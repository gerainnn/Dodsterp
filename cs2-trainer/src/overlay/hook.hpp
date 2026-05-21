#pragma once

namespace overlay {

// Хукает IDXGISwapChain::Present у CS2. После успешной установки наш код
// вызывается каждый кадр — там и рисуем меню/ESP.
bool install_hook();
void remove_hook();

} // namespace overlay
