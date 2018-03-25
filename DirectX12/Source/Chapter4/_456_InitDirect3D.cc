#include <DirectXColors.h>
#include "../Common/d3dApp.h"

class InitDirect3DApp final : public D3DApp {
public:
    InitDirect3DApp(HINSTANCE hInstance);
    ~InitDirect3DApp();

    bool Initialize() override final;
private:
    void OnResize() override final;
    void Update(const GameTimer& gt) override final;
    void Draw(const GameTimer& gt) override final;
};

int WINAPI WinMain(HINSTANCE h_instance, HINSTANCE prev_instance, PSTR cmd_line, int show_cmd) {
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try {
        InitDirect3DApp the_application(h_instance);
        if (!the_application.Initialize()) return 0;

        return the_application.Run();
    }
    catch (DxException& e) {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE h_instance) : D3DApp{ h_instance } {};

bool InitDirect3DApp::Initialize() {
    if (!D3DApp::Initialize()) return false; else return true;
}

void InitDirect3DApp::OnResize() {
    D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt) {

}

void InitDirect3DApp::Draw(const GameTimer& gt) {
    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(),
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0x00, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmd_lists[] = { mCommandList.Get() };

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}

InitDirect3DApp::~InitDirect3DApp() = default;