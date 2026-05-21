#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace overlay::renderer {

bool init(ID3D11Device* dev, ID3D11DeviceContext* ctx, HWND hwnd);
void shutdown();

} // namespace overlay::renderer
