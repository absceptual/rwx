#pragma once
#include <iostream>
#include <memory>
#include <utility>

#include <Windows.h>

// Direct3D 11 header files
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")


/* Purpose of this renderer class :
	- Serve as a wrapper for internal Direct3D 11 functions


*/
class Renderer
{
public:
	using vtables_t = std::pair<void**, void**>;
	using present_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);

	struct Vertex
	{
		FLOAT x, y, z; D3DXCOLOR color;
	};

private:
	ID3D11Device* m_device{ };
	IDXGISwapChain* m_swapchain{ };
	ID3D11DeviceContext* m_ctx{ };
	ID3D11RenderTargetView* m_renderbuffer{ }; // Target for rendering our primtiives
	ID3D11VertexShader* m_vshader{ }; // Shader responsible for vertices
	ID3D11PixelShader* m_pshader{ }; // Shader responsible for pixels
	ID3D11Buffer* m_buffer{ }; // Buffer for vertices
	ID3D11InputLayout* m_layout{ }; // Defines vertex layout in memory
	Renderer::vtables_t m_vtables{};

	const wchar_t* m_shader{ L"shader.sco" };
	bool m_initalized{ false };

public:
	Renderer() = default;

	// Meant to be called within a present hook (add compiling shaders)
	auto init(IDXGISwapChain* device) -> bool;
	auto clean() -> void;

	// Drawing setup
	auto begin() -> bool;


	// Called within our beginning setup function
	auto get_target() -> bool;
	auto get_viewport() -> bool;
	auto setup_shaders() -> bool;

	// Helper function to setup VMT hooks
	auto dump_virtual_tables() -> bool;
	auto get_virtual_tables() -> vtables_t;

	// Drawing functions
	auto draw_line(D3DXVECTOR2 from, D3DXVECTOR2 to, D3DXCOLOR color, float thickness = 1.0f) -> void;
	auto draw_box(D3DXVECTOR2 position, D3DXVECTOR2 size, D3DXCOLOR color, float thickness, bool filled = false) -> void;
};
