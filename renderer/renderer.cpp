#include "renderer.h"

/*
How do we render internally with Direct3D 11?
1. Dynamically hook IDXGISwapChain::Present with a VMT hook (varies between architetures) 
2. Call our begin function within the present hook
	a. Set our renderer pointers (device, swapchain, context and render target)
	b. Setup our shaders so we can draw like a chad
	c. Grab your viewpo
*/



// Installs a trampoline hook on IDXGISwapChain::Present
auto Renderer::Renderer(const wchar_t* process, present_t hk_present)
{
	IDXGISwapChain* swapchain{ nullptr };
	ID3D11Device* device{ nullptr };

	DXGI_SWAP_CHAIN_DESC desc{ };
 	RtlZeroMemory(&desc, 0x0);

	// Attempt to create a D3D11 device and throw an exception if we fail
        if (FAILED(D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		nullptr,
		NULL,
		D3D11_SDK_VERSION,
		&desc,
		&swapchain,
		&device,
		nullptr,
		nullptr;
	 )))
	 {
		m_initalized = false;
		throw std::runtime_error("Failed to initalize D3D11 device");
	 }

	 // Retrieve the virtual table from our swapchain
	auto vtable  = *reinterpret_cast<void***>(swapchain);
	auto present = vtable[present_index];
	if (!present)
	    return;

	this->o_present = present;

    	// Prepare to install our trampoline hook by allocating memory for our gateway then redirecting control flow
	auto gateway = reinterpret_cast<uintptr_t*>(VirtualAlloc(NULL, NULL, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	if (!gateway)
	    return;

	// Override original protection (12 bytes hooked, 9 bytes apart of bytecode)
	DWORD o_protection{ 0x0 };
	VirtualProtect(target, 21, PAGE_EXECUTE_READWRITE, &o_protection);


    	// Setup gateway by copying bytes from target function to gateway
    	memcpy(present_gateway, o_present, size);

#ifdef _WIN64
	const uint8_t bytecode[] = {
		0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // jmp rax (absolute)
	};
#else
    	const uint8_t bytecode[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00 // jmp rax (relative)
	};
#endif

	memcpy(reinterpret_cast<void*>((uintptr_t(gateway) + 12)), bytecode, sizeof(bytecode));
	*reinterpret_cast<uintptr_t*>((uintptr_t(gateway) + 12) + 0x1) = (uintptr_t(target) + size);

    	// Overwrite target function (present)
	memset(o_present, 0x90, size);
	memcpy(o_present, bytecode, sizeof(bytecode));
	*reinterpret_cast<uintptr_t*>(uintptr_t(target) + 0x1) = uintptr_t(hk_present);

	VirtualProtect(target, size, o_protection, &o_protection);
	this->m_initalized = true;
}

// Grab our device, swapchain, context and render target pointers
auto Renderer::init(IDXGISwapChain* swapchain) -> bool
{
	// Don't do anything if we haven't installed a hook or our swapchain pointer is invalid
	if (!this->m_initalized || !swapchain)
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
		
	ID3D11RenderTargetView* render_target{ nullptr };
	ctx->OMGetRenderTargets(1, &this->m_rendertarget, nullptr);

	if (this->m_rendertarget) // Create a render target if we dont have one
		return false;
		
	this->m_ctx = ctx;
	this->m_swapchain = swapchain;
	return true;
}

auto Renderer::begin() -> bool
{
	return (init() && setup_shaders() && get_viewport() && setup_projection());
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

/* Retrieves virtual tables of both ID3D11Device and its swapchain
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
*/

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
	this->m_device->CreateBuffer(&desc, NULL, this->m_buffer0;

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


