

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

Within your ``IDXGISwapChain::Present`` hook, you should call ``Renderer::init`` as fast as possible.
If the function call succeeds, you can call ``Renderer::begin`` and begin rendering.
```cpp
HRESULT __stdcall hooks::hk_present(IDXGISwapChain* _this, UINT interval, UINT flags)
{
	if (renderer::begin(_this))
	{
		// Begin drawing
		D3DXVECTOR2 center{ renderer->get_viewport() };
		D3DCOLOR color{ 255, 255, 255, 1 };
		center.x /= 2;
		center.y /= 2;
		
		renderer::draw_box({center.x - 100, center.y - 100}, {100, 100}, color, 1.0f);
	}

	return hooks::o_present(_this, interval, flags);
}
```

