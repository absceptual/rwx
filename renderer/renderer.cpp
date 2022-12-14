#include "renderer.h"

auto Renderer::init(IDXGISwapChain* swapchain) -> bool
{
	// No need for re-initalization if our data is already set
	if (this->m_initalized || !swapchain || swapchain == this->m_swapchain)
		return false;

	// Retrieve current device from swapchain
	ID3D11Device* device{ nullptr };
	swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device));

	if (!device)
		return false;

	this->m_device = device;

	// Get our device context while we're at it
	ID3D11DeviceContext* ctx{ nullptr };
	this->m_device->GetImmediateContext(&ctx);

	if (!ctx)
		return false;

	this->m_ctx = ctx;
	this->m_swapchain = swapchain;
	this->m_initalized = true;
	return true;
}

auto Renderer::begin() -> bool
{
	return (this->m_initalized && !get_target() && !setup_shaders() && !get_viewport());
}

auto Renderer::get_target() -> bool
{
	if (!this->m_initalized)
		return false;

	this->m_ctx->OMGetRenderTargets(1, &this->m_renderbuffer, NULL);
	return true;
}

auto Renderer::get_viewport() -> bool
{
	return false;
}

auto Renderer::setup_shaders() -> bool
{
	if (!this->m_initalized)
		return false;

	// Initalize our pixel shader first
	ID3D10Blob* vshader{ nullptr }, * pshader{ nullptr };
	auto result = D3DX11CompileFromFile(
		this->m_shader,
		NULL,
		NULL,
		"VShader",
		"vs_4_0",
		NULL,
		NULL,
		NULL,
		&vshader,
		NULL,
		NULL
	);

	if (result != S_OK)
		return false;

	result = D3DX11CompileFromFile(
		this->m_shader,
		NULL,
		NULL,
		"PShader",
		"ps_4_0",
		NULL,
		NULL,
		NULL,
		&vshader,
		NULL,
		NULL
	);

	if (result != S_OK)
		return false;

	this->m_device->CreateVertexShader(vshader->GetBufferPointer(), vshader->GetBufferSize(), NULL, &this->m_vshader);
	this->m_device->CreatePixelShader(pshader->GetBufferPointer(), pshader->GetBufferSize(), NULL, &this->m_pshader);

	this->m_ctx->VSSetShader(this->m_vshader, NULL, NULL);
	this->m_ctx->PSSetShader(this->m_pshader, NULL, NULL);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	this->m_device->CreateInputLayout(layout, 2, vshader->GetBufferPointer(), vshader->GetBufferSize(), &this->m_layout);
	this->m_ctx->IASetInputLayout(this->m_layout);
	return true;
}

// Retrieves virtual tables of both ID3D11Device and its swapchain
auto Renderer::dump_virtual_tables() -> bool
{
	// Create a temporary device to retrieve its virtual table
	ID3D11Device* p_device{ };
	ID3D11DeviceContext* p_ctx{ };
	IDXGISwapChain* p_chain{ };

	DXGI_SWAP_CHAIN_DESC desc{ };

	// Bullshit values for width/height
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1;
	desc.OutputWindow = GetForegroundWindow();
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.SampleDesc.Count = 1;

	auto result = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&desc,
		&p_chain,
		&p_device,
		NULL,
		NULL // No context needed, we only care about the dummy device
	);

	// Return no virtual table if we couldn't create the device
	if (result != S_OK)
		return false;

	memcpy(&m_vtables.first, reinterpret_cast<void***>(p_device), sizeof(void*));
	memcpy(&m_vtables.second, reinterpret_cast<void***>(p_chain), sizeof(void*));
	p_device->Release();
	p_chain->Release();

	return true;
}

auto Renderer::get_virtual_tables() -> vtables_t
{
	return this->m_vtables;
}

auto Renderer::draw_line(D3DXVECTOR2 from, D3DXVECTOR2 to, D3DXCOLOR color, float thickness) -> void
{
	D3D11_BUFFER_DESC desc{ };
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(Renderer::Vertex) * 2; // to & from
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Renderer::Vertex vertices[]{
		{ from.x, from.y, 0.0f, color },
		{ to.x, to.y, 0.0f, color }
	};

	// Map our vertices to the vertex buffer
	D3D11_MAPPED_SUBRESOURCE ms{ NULL };
	this->m_ctx->Map(this->m_buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, &vertices, sizeof(vertices));
	this->m_ctx->Unmap(this->m_buffer, NULL);

	UINT stride = sizeof(Renderer::Vertex);
	UINT offset = 0;
	this->m_ctx->IASetVertexBuffers(0, 2, &this->m_buffer, &stride, &offset);
	this->m_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP); // Drawing a line strip
	this->m_ctx->Draw(2, 0);
}

auto Renderer::draw_box(D3DXVECTOR2 position, D3DXVECTOR2 size, D3DXCOLOR color, float thickness, bool filled) -> void
{
	if (!this->m_initalized)
		return;

	this->draw_line(position, { position.x + size.x, position.y }, color, thickness); // Top left to top right
	this->draw_line({ position.x + size.x, position.y }, { position.x + size.x, position.y + size.y }, color, thickness); // Top right to bottom right
	this->draw_line({ position.x + size.x, position.y + size.y }, { position.x, position.y + size.y }, color, thickness); // Bottom right to bottom left
	this->draw_line({ position.x, position.y + size.y }, position, color, thickness); // Bottom left to top left
}



