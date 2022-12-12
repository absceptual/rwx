
# rwx!
**rwx!** is a class that wraps the D3D11 API to make drawing internally easier. 
It achieves this level of functionality by:

 - Automatically hooking ``IDXGISwapChain::Present`` through a mixture of signature scanning and/or VMT manipulation
 - Dynamically resolving pointers to important Direct3D 11 interfaces
 
 ## How to use?
 Before using the **rwx!** render class, it must be initalized. Please note that **rwx!** should only be used in dynamic link libraries (DLLs).
 ```cpp
// Renderer::Renderer(const wchar_t* process, void* present_replacement) 
const auto renderer = std::make_unique<Renderer>(L"RustClient.exe", hooks::hk_present);
 ```
`const wchar_t* process` - The name of the executable you wish to hook Direct3D 11 on.
``void* present_replacement`` - The function that will be used in the trampohline hook for IDXGISwapChain::Present.
