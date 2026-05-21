// Manual mapper для x64 PE.
//
// Пайплайн:
//   1. читаем DLL с диска, валидируем PE
//   2. в target — VirtualAllocEx под SizeOfImage (RWX, потом фиксим)
//   3. копируем headers + секции по их VA
//   4. в target — VirtualAllocEx под (LoaderData + код Loader)
//   5. CreateRemoteThread на копию Loader, параметр = указатель на LoaderData
//   6. Loader (исполняется в target):
//        - применяет relocations (delta = remote_base - PE.ImageBase)
//        - резолвит imports (LoadLibraryA + GetProcAddress)
//        - проходит TLS callbacks
//        - вызывает DllMain(hinst=remote_base, DLL_PROCESS_ATTACH, 0)
//   7. чистим стаб, затираем PE-header в remote image (анти-сигскан)
//
// Ограничения этой версии:
//   - x64 only (CS2 == x64, не парим)
//   - PAGE_EXECUTE_READWRITE на всём image — не делим обратно по секциям
//     (секционные protections можно добавить позже, на функционал не влияет)
//   - exception directory / SEH unwind info не регистрируем через
//     RtlAddFunctionTable — если внутри DLL вылетит исключение и нужна
//     раскрутка, оно крашнет. Внутри dodsterp.dll исключений быть не должно
//     (явно not-throw кодом + /EHa-).

#include "manual_map.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace inject {

namespace {

char g_err[256] = "";

void set_err(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    std::vsnprintf(g_err, sizeof(g_err), fmt, ap);
    va_end(ap);
}

// ---------------------------------------------------------------------------
// LoaderData: то что мы кладём в target. Все колбеки winapi — заранее
// разрезолвлены в нашем процессе. У target и у нас kernel32/ntdll лежат
// по одинаковым адресам (ASLR per-boot, не per-process), так что валидно.

struct LoaderData {
    BYTE*                       image_base;
    decltype(&LoadLibraryA)     p_LoadLibraryA;
    decltype(&GetProcAddress)   p_GetProcAddress;
};

using DllEntryFn = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);

// ---------------------------------------------------------------------------
// Loader — выполняется в target. Должен быть position-independent:
//   - никаких внешних вызовов кроме указателей из LoaderData
//   - никаких глобалов / стат-переменных с инициализацией
//   - никаких string literals (они живут в нашей .rdata)
//   - никаких runtime checks / GS cookies
//
// Соблюдение этих правил обеспечивается прагмами ниже.

#pragma runtime_checks("", off)
#pragma optimize("", off)
#pragma strict_gs_check(push, off)
#pragma check_stack(off)

static DWORD WINAPI Loader(LoaderData* d) {
    if (!d || !d->image_base) return 0xDEAD0001;

    BYTE* base = d->image_base;
    auto* dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt   = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);

    // ---- 1. relocations -------------------------------------------------
    {
        const auto& dir = nt->OptionalHeader.DataDirectory[
            IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (dir.Size && dir.VirtualAddress) {
            ULONG_PTR delta = reinterpret_cast<ULONG_PTR>(base) -
                              static_cast<ULONG_PTR>(nt->OptionalHeader.ImageBase);
            if (delta != 0) {
                auto* reloc = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
                    base + dir.VirtualAddress);
                while (reloc->VirtualAddress) {
                    DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))
                                  / sizeof(WORD);
                    auto* list = reinterpret_cast<WORD*>(reloc + 1);
                    for (DWORD i = 0; i < count; ++i) {
                        WORD type   = list[i] >> 12;
                        WORD offset = list[i] & 0x0FFF;
                        if (type == IMAGE_REL_BASED_DIR64) {
                            auto* p = reinterpret_cast<ULONG_PTR*>(
                                base + reloc->VirtualAddress + offset);
                            *p += delta;
                        }
                    }
                    reloc = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
                        reinterpret_cast<BYTE*>(reloc) + reloc->SizeOfBlock);
                }
            }
        }
    }

    // ---- 2. imports -----------------------------------------------------
    {
        const auto& dir = nt->OptionalHeader.DataDirectory[
            IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (dir.Size && dir.VirtualAddress) {
            auto* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
                base + dir.VirtualAddress);
            while (desc->Name) {
                const char* dll_name = reinterpret_cast<const char*>(
                    base + desc->Name);
                HMODULE mod = d->p_LoadLibraryA(dll_name);
                if (!mod) return 0xDEAD0002;

                DWORD orig_rva = desc->OriginalFirstThunk
                               ? desc->OriginalFirstThunk
                               : desc->FirstThunk;
                auto* orig = reinterpret_cast<ULONG_PTR*>(base + orig_rva);
                auto* iat  = reinterpret_cast<ULONG_PTR*>(
                                 base + desc->FirstThunk);
                while (*orig) {
                    FARPROC fn;
                    if (*orig & IMAGE_ORDINAL_FLAG64) {
                        fn = d->p_GetProcAddress(
                            mod,
                            reinterpret_cast<LPCSTR>(*orig & 0xFFFF));
                    } else {
                        auto* by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                            base + *orig);
                        fn = d->p_GetProcAddress(mod, by_name->Name);
                    }
                    if (!fn) return 0xDEAD0003;
                    *iat = reinterpret_cast<ULONG_PTR>(fn);
                    ++orig;
                    ++iat;
                }
                ++desc;
            }
        }
    }

    // ---- 3. TLS callbacks ----------------------------------------------
    {
        const auto& dir = nt->OptionalHeader.DataDirectory[
            IMAGE_DIRECTORY_ENTRY_TLS];
        if (dir.Size && dir.VirtualAddress) {
            auto* tls = reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(
                base + dir.VirtualAddress);
            if (tls->AddressOfCallBacks) {
                auto* cb = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(
                    tls->AddressOfCallBacks);
                while (*cb) {
                    (*cb)(base, DLL_PROCESS_ATTACH, nullptr);
                    ++cb;
                }
            }
        }
    }

    // ---- 4. DllMain ----------------------------------------------------
    auto entry = reinterpret_cast<DllEntryFn>(
        base + nt->OptionalHeader.AddressOfEntryPoint);
    entry(reinterpret_cast<HINSTANCE>(base), DLL_PROCESS_ATTACH, nullptr);

    return 0;
}

// маркер для расчёта размера Loader в байтах
static void LoaderEnd() {}

#pragma check_stack()
#pragma strict_gs_check(pop)
#pragma optimize("", on)
#pragma runtime_checks("", restore)

// ---------------------------------------------------------------------------

bool read_file(const std::wstring& path, std::vector<BYTE>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { set_err("cannot open dll: %ls", path.c_str()); return false; }
    auto sz = f.tellg();
    f.seekg(0);
    out.resize(static_cast<size_t>(sz));
    if (!f.read(reinterpret_cast<char*>(out.data()), sz)) {
        set_err("read dll failed");
        return false;
    }
    return true;
}

} // namespace

const char* last_error() { return g_err; }

bool manual_map(DWORD pid, const std::wstring& dll_path) {
    g_err[0] = 0;

    std::vector<BYTE> file;
    if (!read_file(dll_path, file)) return false;

    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(file.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        set_err("not a PE (bad MZ)"); return false;
    }
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(file.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        set_err("not a PE (bad NT sig)"); return false;
    }
    if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        set_err("dll is not 64-bit"); return false;
    }
    if (!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
        set_err("PE is not a DLL"); return false;
    }

    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) {
        set_err("OpenProcess failed: %lu", GetLastError());
        return false;
    }

    SIZE_T image_size = nt->OptionalHeader.SizeOfImage;
    BYTE*  remote_image = static_cast<BYTE*>(VirtualAllocEx(
        proc, nullptr, image_size,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!remote_image) {
        set_err("VirtualAllocEx image failed: %lu", GetLastError());
        CloseHandle(proc);
        return false;
    }

    SIZE_T written = 0;

    // headers
    if (!WriteProcessMemory(proc, remote_image, file.data(),
                            nt->OptionalHeader.SizeOfHeaders, &written)) {
        set_err("write headers failed: %lu", GetLastError());
        VirtualFreeEx(proc, remote_image, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    // sections
    auto* sect = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (!sect[i].SizeOfRawData) continue;
        if (!WriteProcessMemory(proc,
                                remote_image + sect[i].VirtualAddress,
                                file.data() + sect[i].PointerToRawData,
                                sect[i].SizeOfRawData,
                                &written)) {
            set_err("write section %hs failed: %lu",
                    sect[i].Name, GetLastError());
            VirtualFreeEx(proc, remote_image, 0, MEM_RELEASE);
            CloseHandle(proc);
            return false;
        }
    }

    // LoaderData + Loader code в отдельной аллокации
    LoaderData data{};
    data.image_base       = remote_image;
    data.p_LoadLibraryA   = LoadLibraryA;
    data.p_GetProcAddress = GetProcAddress;

    SIZE_T loader_size = static_cast<SIZE_T>(
        reinterpret_cast<BYTE*>(LoaderEnd) - reinterpret_cast<BYTE*>(Loader));
    if (loader_size == 0 || loader_size > 0x4000) {
        // если линкер пере-сортировал функции (ICF/REF), валимся в безопасный
        // размер. /OPT:NOICF /OPT:NOREF в линкере должны это предотвращать,
        // но 0x1000 байт хватит на любой реалистичный Loader.
        loader_size = 0x1000;
    }

    SIZE_T stub_total = sizeof(LoaderData) + loader_size;
    BYTE*  remote_stub = static_cast<BYTE*>(VirtualAllocEx(
        proc, nullptr, stub_total,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!remote_stub) {
        set_err("VirtualAllocEx stub failed: %lu", GetLastError());
        VirtualFreeEx(proc, remote_image, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    if (!WriteProcessMemory(proc, remote_stub, &data, sizeof(data), &written) ||
        !WriteProcessMemory(proc, remote_stub + sizeof(LoaderData),
                            reinterpret_cast<LPCVOID>(Loader),
                            loader_size, &written)) {
        set_err("write stub failed: %lu", GetLastError());
        VirtualFreeEx(proc, remote_stub,  0, MEM_RELEASE);
        VirtualFreeEx(proc, remote_image, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    BYTE*  remote_loader = remote_stub + sizeof(LoaderData);
    HANDLE thr = CreateRemoteThread(
        proc, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(remote_loader),
        remote_stub, 0, nullptr);
    if (!thr) {
        set_err("CreateRemoteThread failed: %lu", GetLastError());
        VirtualFreeEx(proc, remote_stub,  0, MEM_RELEASE);
        VirtualFreeEx(proc, remote_image, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    WaitForSingleObject(thr, INFINITE);
    DWORD code = 0;
    GetExitCodeThread(thr, &code);
    CloseHandle(thr);

    // стаб всегда чистим
    VirtualFreeEx(proc, remote_stub, 0, MEM_RELEASE);

    if (code != 0) {
        set_err("loader returned 0x%X (1=null base, 2=LoadLibrary, 3=GetProcAddr)",
                code);
        VirtualFreeEx(proc, remote_image, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    // затираем PE-header у image — простая защита от наивного сигскана
    // (DOS+NT signature). Не идеал, но дёшево.
    BYTE zeros[0x1000] = {};
    DWORD wipe = (image_size < sizeof(zeros)) ? (DWORD)image_size
                                              : (DWORD)sizeof(zeros);
    WriteProcessMemory(proc, remote_image, zeros, wipe, &written);

    CloseHandle(proc);
    return true;
}

} // namespace inject
