#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct ObjectConstants {
    DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class Application final : public D3DApp {
public:
    Application(HINSTANCE h_instance);
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize() override final;

private:
    void OnResize() override final;
    void Update(const GameTimer& dt) override final;
    void Draw(const GameTimer& dt) override final;

    void OnMouseDown(WPARAM btn_state, int x, int y) override final;
    void OnMouseUp(WPARAM btn_state, int x, int y) override final;
    void OnMouseMove(WPARAM btn_state, int x, int y) override final;

    void BuildDescriptorHeaps();
	void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    DirectX::XMFLOAT4X4 m_world = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 m_view  = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 m_proj  = MathHelper::Identity4x4();
    POINT m_last_mouse_pos{};
    float m_theta   = 1.5f * DirectX::XM_PI;
    float m_phi     = DirectX::XM_PIDIV4;
    float m_radius  = 5.0f;
    std::unique_ptr<UploadBuffer<ObjectConstants>> m_object_cb{ nullptr };
    std::unique_ptr<MeshGeometry> m_box_geometry{ nullptr };

    Microsoft::WRL::ComPtr<ID3DBlob> m_vs_byte_code{ nullptr };
    Microsoft::WRL::ComPtr<ID3DBlob> m_ps_byte_code{ nullptr };
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso{ nullptr };

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_heap{ nullptr };
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature{ nullptr };
};

Application::Application(HINSTANCE h_instance) : D3DApp{ h_instance } {}

bool Application::Initialize() {
    if (!D3DApp::Initialize()) return false;

    /*! Reset command list and allocator stores list memory and set it to nullptr as dummy state */
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildDescriptorHeaps();
	BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    /*! Close command lost to get GPU to use it. */
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmds_list[] = { mCommandList.Get() };
    /*! Insert command queue of GPU, and execute asynchronically */
    mCommandQueue->ExecuteCommandLists(_countof(cmds_list), cmds_list);
    /*! Wait until command queue is over. */
    FlushCommandQueue();
    return true;
}

void Application::OnResize() {
	D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    auto P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&m_proj, P);
}

void Application::Update(const GameTimer& dt) {
    using namespace DirectX;
    // Convert Spherical to Cartesian coordinates.
    float x = m_radius*sinf(m_phi)*cosf(m_theta);
    float z = m_radius*sinf(m_phi)*sinf(m_theta);
    float y = m_radius*cosf(m_phi);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_view, view);
    XMMATRIX world = XMLoadFloat4x4(&m_world);
    XMMATRIX proj = XMLoadFloat4x4(&m_proj);
    XMMATRIX worldViewProj = world*view*proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    m_object_cb->CopyData(0, objConstants);
}

void Application::Draw(const GameTimer& dt) {
    /*! Reset allocator memery to write new commands */
    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    /*! Reset command list with m_pso also afterward ExecuteCommandList called last frame. */
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), m_pso.Get()));
    /*! Insert boilerplate commands */
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    /*! Change state for synchronization */
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    /*! Clear RTV, DSV overall region */
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(),
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                        1.f, 0, 0, nullptr);

    /*! Set target buffers to which we want to render */
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    /*! Set CBV heap */
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbv_heap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    /*! Set root signature */
    mCommandList->SetGraphicsRootSignature(m_root_signature.Get());
    /*! Set data */
    mCommandList->IASetVertexBuffers(0, 1, &m_box_geometry->VertexBufferView());
    mCommandList->IASetIndexBuffer(&m_box_geometry->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    /*! Set Root signature descriptor table */
    mCommandList->SetGraphicsRootDescriptorTable(0, m_cbv_heap->GetGPUDescriptorHandleForHeapStart());

    /*! Insert Draw call */
    mCommandList->DrawIndexedInstanced(m_box_geometry->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    /*! After those, return state to initial state */
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    /*! Done recording commands */
    ThrowIfFailed(mCommandList->Close());

    /*! Add command list to the queue */
    ID3D12CommandList* cmds_lists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmds_lists), cmds_lists);
    /*! Swap the back and front buffers */
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
    /*! Wait */
	FlushCommandQueue();
};

void Application::OnMouseDown(WPARAM btn_state, int x, int y) {
    m_last_mouse_pos.x = x;
    m_last_mouse_pos.y = y;

    SetCapture(mhMainWnd);
}

void Application::OnMouseUp(WPARAM btn_state, int x, int y) {
    ReleaseCapture();
}

void Application::OnMouseMove(WPARAM btn_state, int x, int y) {
    if ((btn_state & MK_LBUTTON)) {
        // Make each pixel correspond to a quarter of a degree.
        auto dx = DirectX::XMConvertToRadians(0.25f*static_cast<float>(x - m_last_mouse_pos.x));
        auto dy = DirectX::XMConvertToRadians(0.25f*static_cast<float>(y - m_last_mouse_pos.y));
        // Update angles based on input to orbit camera around box.
        m_theta += dx;
        m_phi += dy;
        // Restrict the angle mPhi.
        m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi - 0.1f);

    }
    else if ((btn_state & MK_RBUTTON)) {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f*static_cast<float>(x - m_last_mouse_pos.x);
        float dy = 0.005f*static_cast<float>(y - m_last_mouse_pos.y);
        // Update the camera radius based on input.
        m_radius += dx - dy;
        // Restrict the radius.
        m_radius = MathHelper::Clamp(m_radius, 3.0f, 15.0f);
    }

    m_last_mouse_pos.x = x;
    m_last_mouse_pos.y = y;
}

void Application::BuildDescriptorHeaps() {
    /*! Make descriptor heap for constant buffer */
    D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc;
    cbv_heap_desc.NumDescriptors = 1; /*! Just one descriptor */
    cbv_heap_desc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; /*! DescHeap type is CBV_SRV_UAV */
    /*! 쉐이더 프로그램에서 이 서술자에 직저 접근. */
    cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    /*!
     * The flag D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE can optionally be set on a descriptor
     * heap to indicate it is be bound on a command list for reference by shaders.
     * Descriptor heaps created without this flag allow applications the option to stage descriptors
     * in CPU memory before copying them to a shader visible descriptor heap, as a convenience.
     * But it is also fine for applications to directly create descriptors into shader visible
     * descriptor heaps with no requirement to stage anything on the CPU.
     */
    cbv_heap_desc.NodeMask = 0;
    /*!
     * For single-adapter operation, set this to zero.
     * If there are multiple adapter nodes, set a bit to identify the node */
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&m_cbv_heap)));
}

void Application::BuildConstantBuffers() {
    /*! Make constant buffer for upload heap */
    m_object_cb = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);
    /*! Set constant buffer view */
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    cbv_desc.BufferLocation = m_object_cb->Resource()->GetGPUVirtualAddress();
    cbv_desc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    /*! Create constant buffer view */
    md3dDevice->CreateConstantBufferView(&cbv_desc, m_cbv_heap->GetCPUDescriptorHandleForHeapStart());
}

void Application::BuildRootSignature() {
    /*! Set root parameter for root signature */
    CD3DX12_ROOT_PARAMETER slot_root_parameter[1];
    CD3DX12_DESCRIPTOR_RANGE cbv_table;
    cbv_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); /*! constant buffer for b0, just one */
    slot_root_parameter[0].InitAsDescriptorTable(1, &cbv_table);
    /*! Set root signature, Root signature is an array of root parameters */
    CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc{ 1, slot_root_parameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
    /*! Serialize root signature with root signature descriptor */
    Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_sig{ nullptr };
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob{ nullptr };
    auto result = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              serialized_root_sig.GetAddressOf(),
                                              error_blob.GetAddressOf());
    if (error_blob) {
        ::OutputDebugStringA((char*)error_blob->GetBufferPointer());
    }
    ThrowIfFailed(result);
    /*! Create root signature */
    md3dDevice->CreateRootSignature(0,
                                    serialized_root_sig->GetBufferPointer(),
                                    serialized_root_sig->GetBufferSize(),
                                    IID_PPV_ARGS(&m_root_signature));
}

void Application::BuildShadersAndInputLayout() {
    HRESULT hr = S_OK;
    /*! Compile and get bytecodes */
    m_vs_byte_code = d3dUtil::CompileShader(L"Chapter6/color.hlsl", nullptr, "VS", "vs_5_0");
    m_ps_byte_code = d3dUtil::CompileShader(L"Chapter6/color.hlsl", nullptr, "PS", "ps_5_0");
    /*! Set input layout */
    /*! semantic name, semantic index, format, input socket, byte offset, interval, interval offset */
    m_input_layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void Application::BuildBoxGeometry() {
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4;
    std::array<Vertex, 8> vertices = {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(DirectX::Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Magenta) })
    };

    std::array<std::uint16_t, 36> indices = {
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

    m_box_geometry = std::make_unique<MeshGeometry>();
    m_box_geometry->Name = "boxGeo";

    const auto vb_byte_size = vertices.size() * sizeof(Vertex);
    const auto ib_byte_size = indices.size() * sizeof(std::uint16_t);
    /*! Make vertex, index buffer in Cpu */
    ThrowIfFailed(D3DCreateBlob(vb_byte_size, &m_box_geometry->VertexBufferCPU));
    CopyMemory(m_box_geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vb_byte_size);
    ThrowIfFailed(D3DCreateBlob(ib_byte_size, &m_box_geometry->IndexBufferCPU));
    CopyMemory(m_box_geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ib_byte_size);
    /*! Create vertex, index buffer on GPU Default heap, and send a data using Intermediate buffer on Upload heap. */
    m_box_geometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), mCommandList.Get(), vertices.data(), vb_byte_size, m_box_geometry->VertexBufferUploader);
    m_box_geometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), mCommandList.Get(), indices.data(), ib_byte_size, m_box_geometry->IndexBufferUploader);
    /*! Set other settings */
    m_box_geometry->VertexByteStride = sizeof(Vertex);
    m_box_geometry->VertexBufferByteSize = vb_byte_size;
    m_box_geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
    m_box_geometry->IndexBufferByteSize = ib_byte_size;
    /*! Set Submesh Geometry */
    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    m_box_geometry->DrawArgs["box"] = submesh;
}

void Application::BuildPSO() {
    /*! Set settings of pipeline state object */
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
    ZeroMemory(&pso_desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    pso_desc.InputLayout    = { m_input_layout.data(), (UINT)m_input_layout.size() };
    pso_desc.pRootSignature = m_root_signature.Get();
    pso_desc.VS = {
		reinterpret_cast<BYTE*>(m_vs_byte_code->GetBufferPointer()), m_vs_byte_code->GetBufferSize()
	};
    pso_desc.PS = {
		reinterpret_cast<BYTE*>(m_ps_byte_code->GetBufferPointer()), m_ps_byte_code->GetBufferSize()
	};
    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = mBackBufferFormat;
    pso_desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    pso_desc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    pso_desc.DSVFormat = mDepthStencilFormat;
    /*! Create PSO */
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&m_pso)));
}

int WINAPI WinMain(HINSTANCE h_instance, HINSTANCE prev_instance, PSTR cmd_line, int show_cmd) {
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    try {
        Application app(h_instance);
        if (!app.Initialize()) return 0;
        return app.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

