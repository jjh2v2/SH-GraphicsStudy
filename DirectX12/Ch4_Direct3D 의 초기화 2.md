# 4 :: Direct3D 의 초기화 2

## 4.3 Direct3D 의 초기화

*Direct3D* 의 초기화 설정은 다음과 같은 단계로 구성된다.

1. D3D12CreateDevice 함수를 사용해서 ID3D12Device *COM interface* 을 생성한다.
2. ID3D12Fence 객체를 생성하고 *Descriptor* 의 크기를 얻는다.
3. 4X MSAA (Multisampling anti-aliasing) 품질 수준 지원 여부를 결정한다.
4. *Command List, Allocator* 을 생성하고 명령 목록을 할당한다
5. *Swap chain* 에 대한 *Descriptor* 과 버퍼 자체를 생성한다.
6. Application 에 필요한 *Descriptor heap* 을 생성한다.
7. *Back buffer* 의 크기를 설정하고, *RTV* 을 생성한다.
8. *DSV* 와 *DSV*에 대한 버퍼를 생성한다.
9. *Viewport* 및 가위 판정용 사각형들을 생성한다.

### 4.3.1 `ID3D12Device` 생성

> https://msdn.microsoft.com/en-us/library/windows/desktop/dn788650(v=vs.85).aspx
> Represents a virtual adapter; it is used to create command allocators, command lists, command queues, fences, resources, pipeline state objects, heaps, root signatures, samplers, and many resource views.

*Device* 는 **Display adaptor 을 나타내는 객체**이다. 보통 디스플레이 어댑터는 물리적인 3차원 그래픽 하드웨어 장치이다. 하지만 하드웨어 그래픽 기능을 흉내내는 소프트웨어 디스플레이 어댑터 (**WRAP adaptor**) 역시 존재한다.

* ***`ID3D12Device`*** 는 기능 지원 점검에 쓰이며, 자원 뷰, 명령 목록 등의 다른 모든 D3D COM 인터페이스 객체의 생성에도 쓰인다. 그리고 윈도우 10 에서는 해당 Device 인터페이스만을 써야한다.

*`ID3D12Device`* 을 생성할 때는 다음과 같은 함수를 사용한다.

#### A. D3D12CreateDevice

* ``` c++
  HRESULT WINAPI D3D12CreateDevice(
    _In_opt_  IUnknown          *pAdapter,
              D3D_FEATURE_LEVEL MinimumFeatureLevel,
    _In_      REFIID            riid,
    _Out_opt_ void              **ppDevice
  );
  ```

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770336(v=vs.85).aspx

  1. `pAdapter`
     장치가 나타내는 **디스플레이 어댑터**를 말한다. 만약 이 매개변수가 `nullptr` 이라면 시스템의 기본 디스플레이 어댑터가 자동으로 쓰인다. 
     * 디스플레이 어댑터를 얻는 방법은, ***DXGI*** 을 사용해서 얻을 수 있다. 자세한 것은 
       https://msdn.microsoft.com/en-us/library/windows/desktop/ff476878(v=vs.85).aspx
       을 참고할 것.
  2. `MinimumFeatureLevel`
     응용 프로그램이 요구하는 **최소 Feature level.** 만일 디스플레이 어댑터 (그래픽 카드, 등) 이 Level 을 지원하지 않으면 장치 생성이 실패한다.
     * `D3D_FEATURE_LEVEL_ENUMRATION`
       https://msdn.microsoft.com/en-us/library/windows/desktop/ff476329(v=vs.85).aspx
  3. `riid`
     생성하고자 하는 ID3D12Device 인터페이스의 COM Interface ID.
  4. ``ppDevice`
     생성된 장치가 설정될 출력 매개변수.

#### B. Example

다음은 `bool D3DApp::InitDirect3D()` 에서 ID3D12Device 을 사용한 호출 예이다.

``` c++
#if defined(DEBUG) || define(_DEBUG) {
/*! Activate D3D12 Debug layer */
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
}
#endif
ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
/*! Create device represents hardware adapter */
HRESULT hardwareResult = 
    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
/*! If failed, Generate WRAP Adapter support features as the way of software */
if (FAILED(hardwareResult)) {
    ComPtr<IDXGIAdapter> pWarpAdapter;
    ThrowIfFailed(mdxgiFactory->EnumWrapAdapter(IID_PPV_ARGS(&pWarpAdapter)));
    ThrowIfFailed(D3D12CreateDevice(
        pWrapAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)));
}
```

우선 이 코드에서는 디버그 모드일 때 디버그 레이어를 활성화한다. 여기서 `ID3D12Debug` 는 디버그 세팅을 컨트롤하는 디버그 인터페이스이다. 이 층이 활성화 되어 있으면, *Direct3D* 는 추가적인 디버깅을 활용해서, *VC++* 의 출력 창에 디버그 메시지를 출력한다.

> `ID3D12Debug` 에 대해 자세한 것은 다음을 참고하라.
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn950153%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

그리고 DXGI 1.1 의 팩터리를 생성하고, **첫 D3D12CreateDevice 호출**을 해서 하드웨어 어댑터를 받도록 한다. 하지만 실패하게 되면, 소프트웨어 어댑터인 *WRAP* 을 나타내는 장치를 생성한다.

* `CreateDXGIFactory1()`

  >  https://msdn.microsoft.com/en-us/library/windows/desktop/ff471318(v=vs.85).aspx

  Creates a DXGI 1.1 factory that you can use to generate other DXGI objects.

* ***WARP (Windows Advanced Rasterization Platform) (Windows 고급 래스터화 플랫폼)***

  >  https://msdn.microsoft.com/en-us/library/windows/desktop/gg615082(v=vs.85).aspx

  소프트웨어 렌더링 어댑터로 DirectX11 에서 소개되었으며, 이를 통해 하드웨어 기능이 지원되지 않을 때 소프트웨어적으로 렌더링을 대신하게 하는 어댑터이다. 자세한 것은 위 주소를 참고한다.

  * Windows 7 및 그 이전에서는 D3D_FEATURE_LEVEL_10_1 까지만 지원하고, Windows 8 의 경우에는 D3D_FEATURE_LEVEL_11_1 까지 지원한다.
  * *WRAP* 을 생성할려면, **`IDXGIFactory4` **의 `EnumWrapAdapter` 메서드를 호출하지 않으면 안된다. 따라서 위에서 어댑터를 생성하기 전에 팩터리 *COM Interface* 을 생성했다. 이렇게 해서 디스플레이 어댑터 나열 시에 *WRAP* 어댑터가 같이 나타난다.

mdxgiFactory 객체는 나중에도 쓰이는데, 왜냐면 *Swap chain* 이 *DXGI* 의 일부이기 때문이다.

### 4.3.2 `ID3D12Fence` 생성 및 Descriptor 크기 얻기

장치를 생성한 다음에는 *CPU* 와 *GPU* 의 동기화를 위한 **`ID3D12Fence`** 객체를 생성한다. 이후에는 필요한 서술자들의 크기도 미리 조회해서 설정해 둔다. 왜냐면 **서술자의 크기는 GPU 마다 다를 수 있기 때문에** 실행 시점에서 미리 적절하게 크기를 얻어야 좋다. 그리고 이 크기를 적절한 멤버 변수에 저장해둔다.

``` c++
ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
mRtvDescriptorSize = 
    md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
mDsvDescriptorSize = 
    md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
mCbvSrvUavDescriptorSize = 
    md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
```

위 코드에서는 만들어 둔 Device COM *interface*을 통해  `ID3D12Fence` 을 만들고 그 뒤에 ***RTV, DSV, CBV, SRV, UAV*** 의 *Descriptor* 핸들 사이즈를 반환한다. 

### 4.3.3 4X MSAA 품질 수준 지원 점검

*Direct3D 11* 을 지원하는 모든 장치는 4X MSAA (멀티샘플링 안티에일리어싱)을 지원한다. 그래서 만약에 지원 레벨이 11 이상이면 이 기능을 명시적으로 지원 안해도 상관은 없지만, 명시적으로 점검할 경우에는 다음과 같은 코드를 사용한다.

``` c++
D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
msQualityLevels.Format = mBackBufferFormat;
msQualityLevels.SampleCount = 4;
msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
msQualityLevels.NumQualityLevels = 0;

HRESULT res = md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                              &msQualityLevels,
                                              sizeof(msQualityLevels)));
ThrowIfFailed(res);

m4xMsaaQuality = msQualityLevels.NumQualityLevels;
/*! IF MORE THAN D3D_FEATURE_LEVEL_11_0, assert */
assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level");
```

> D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS 구조체에 대해서는 다음을 참고한다.
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn859387(v=vs.85).aspx

### 4.3.4 Command Queue 및 List 생성

#### A. Introduction

* ***`ID3D12CommandQueue`***

  >https://msdn.microsoft.com/en-us/library/windows/desktop/dn788627%28v=vs.85%29.aspx

  Command Queue 을 대표하는 COM interface 이다.

* ***`ID3D12CommandAllocator`***

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770463(v=vs.85).aspx

  Command Allocator 을 대표하는 COM Interface 이다.

* ***`ID3D12GraphicsCommandList`***

  >https://msdn.microsoft.com/en-us/library/windows/desktop/dn903537%28v=vs.85%29.aspx

  Command List 을 대표하는 COM Interface 이다. 이는 ID3D12CommandList 을 상속한다.

#### B. Example

다음 예제는 Queue, Allocator 그리고 List 을 생성하는 것을 보여준다.

``` c++
ComPtr<ID3D12CommandQueue> mCommandQueue;
ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
ComPtr<ID3D12GraphicsCommandList> mCommandList;

void D3DApp::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}
```

CreateCommandList 에서, ID3D12PipelineState 에 `nullptr` 을 제출했다. 왜냐면 이번 장에서는 어떤 그리기 명령도 제출하지 않을 예정이기 때문에 상태를 설정할 필요가 없다.

> 명령 목록의 **초기 파이프라인 상태**를 지정한다. BUNDLE 및 실제 그리기 명령은 없는 list 에 대해서는 ***nullptr*** 을 지정해도 된다.

> `ID3D12PipelineState` 에 자세히 대해서는 제 6장에서 논의한다.

### 4.3.5 Swap chain 과 Descriptor 의 생성

#### A. `DXGI_SWAP_CHAIN_DESC`

Queue, Allocator, List 을 생성한 뒤에는, *Swap chain (교환 사슬)* 을 생성해야 한다.
그럴려면 우선 `DXGI_SWAP_CHAIN_DESC` 구조체의 인스턴스 멤버들을 지금 생성하고자 하는 Swap chain 에 맞게 설정해야 한다.

> DXGI_SWAP_CHAIN_DESC structure
> https://msdn.microsoft.com/en-us/library/windows/desktop/bb173075(v=vs.85).aspx

``` c++
typedef struct DXGI_SWAP_CHAIN_DESC {
  DXGI_MODE_DESC   BufferDesc;
  DXGI_SAMPLE_DESC SampleDesc;
  DXGI_USAGE       BufferUsage;
  UINT             BufferCount;
  HWND             OutputWindow;
  BOOL             Windowed;
  DXGI_SWAP_EFFECT SwapEffect;
  UINT             Flags;
} DXGI_SWAP_CHAIN_DESC;
```

이 서술자 구조체에 있는 `DXGI_MODE_DESC` 서술자 구조체는 다음과 같다.

> DXGI_MODE_DESC structure
> https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

``` c++
typedef struct DXGI_MODE_DESC {
  UINT                     Width;
  UINT                     Height;
  DXGI_RATIONAL            RefreshRate;
  DXGI_FORMAT              Format;
  DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
  DXGI_MODE_SCALING        Scaling;
} DXGI_MODE_DESC;
```

#### B. 설명

1. ***`BufferDesc`***
   생성하고자 하는 **Back buffer** 의 속성들을 서술한다. 여기서 Width, Height, 및 Format 을 서술한다.
2. ***`SampleDesc`***
   MultiSampling 표본 개수와 품질 수준을 설정한다. 만일 MSAA 을 끄고 싶다면 표본 개수를 1, 품질 수준을 0 으로 돌린다.
3. ***`BufferUsage`***
   여기서는 *back-buffer* 에 렌더링할 예정이기 때문에, `DXGI_USAGE_RENDER_TARGET_OUTPUT` 을 지정한다.
4. ***`BufferCount`*** 여기서는 교환 사슬을 사용해서 더블 버퍼링을 구현할 것이기 때문에, $ 2 $ 로 지정한다.
5. ***`OutputWindow`*** 렌더링 결과가 표시될 창의 핸들이다.
6. ***`Windowed`*** 윈도우 모드를 설정할지 안할지를 설정한다.
7. ***`SwapEffect`*** A member of the DXGI_SWAP_EFFECT enumerated type that describes options for handling the contents of the presentation buffer after presenting a surface.
8. ***`Flags`*** describes options for swap-chain behavior. 추가적인 플래그들을 나타낸다.

`DXGI_SWAP_CHAIN_DESC` 의 정보를 다 채웠으면 CreateSwapChain() 메서드를 호출해서 교환 사슬을 생성한다.

> IDXGIFactory::CreateSwapChain method
> https://msdn.microsoft.com/en-us/library/windows/desktop/bb174537%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
>
> [Starting with Direct3D 11.1, we recommend not to use CreateSwapChain anymore to create a swap chain. Instead, use CreateSwapChainForHwnd, CreateSwapChainForCoreWindow, or CreateSwapChainForComposition depending on how you want to create the swap chain.]

#### C. 적용

``` c++
void D3DApp::CreateSwapChain() {
    // Release the previous swapchain we will be recreating.
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
        mCommandQueue.Get(), &sd, mSwapChain.GetAddressOf()));
}
```

이렇게 하면 응용 프로그램은 런타임 시간에 이전과는 다른 설정으로 *Swap chain* 을 다시 재생성할 수 있게 된다.

### 4.3.6 Descriptor Heap 생성

이제 서술자들을 담을 서술자 힙을 만들어야 한다. 이 서술자 힙은 ***`ID3D12DescriptorHeap`*** COM 인터페이스로 대표된다.

> ID3D12DescriptorHeap 의 설명
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn788648%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

여기서는 Swap chain 에 대한 **RTV** 와 **DSV** 가 2 개, 1 개 필요하다. 왜냐면 RTV 는 전면 버퍼와 후면 버퍼의 역할을 해야하며 DSV 는 현재 렌더링되는 버퍼의 깊이 및 스텐실 정보만 필요하기 때문이다. 서술자 힙은 종류마다 따로 만들어야 한다.

다음은 힙을 생성하는 코드이다.

``` c++
ComPtr<ID3D12DescriptorHeap> mRtvHeap;
ComPtr<ID3D12DescriptorHeap> mDsvHeap;

void D3DApp::CreateRtvAndDsvDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}
```

그리고 더블 버퍼링을 위한 버퍼 인덱스의 참조는 수동으로 하지 않으면 안된다.

힙을 생성하고 나면, *Descriptor heap* 에 저장된 서술자들에 접근이 가능하다. 또는 핸들을 통해서 서술자들을 참조 할 수 있는데, **힙의 첫 서술자에 대한 핸들은** ***`ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart`*** 로 얻을 수 있다. 

> ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn899174%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

### 4.3.7 RTV (Render Target View) 생성

*Direct3D 12* 에서는 자원 자체를 파이프단계에 묶지는 않는다. 대신에 자원에 대한 ***View*** 을 생성해서 이 뷰를 파이프라인에 묶어야 한다. 예를 들어서 후면 버퍼를 파이프라인의 **output merger** 에 연결시키려고 하면, 후변 버퍼에 대한 **RTV**을 생성해야 한다. 그래야 제대로 후면 버퍼에 렌더링이 된다. 

우선 해야할 것은, *Swap chain* 에 저장된 버퍼 자원을 얻는다.

> IDXGISwapChain::GetBuffer method
> https://msdn.microsoft.com/en-us/library/windows/desktop/bb174570(v=vs.85).aspx

``` c++
HRESULT GetBuffer(
        UINT   Buffer,
  [in]  REFIID riid,
  [out] void   **ppSurface
);
```

* `GetBuffer` 을 호출하면 해당 후면 버퍼의 COM 참조 횟수가 증가하기 때문에, ComPtr 등을 사용해서 자동으로 할당 해제 할 수 있도록 해야한다.

**RTV** 을 생성할 때는 ***`ID3D12Device::CreateRenderTargetVIew`*** 메서드를 사용한다.

> ID3D12Device::CreateRenderTargetView method
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn788668(v=vs.85).aspx

``` c++
void CreateRenderTargetView(
  [in, optional]       ID3D12Resource                *pResource,
  [in, optional] const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
  [in]                 D3D12_CPU_DESCRIPTOR_HANDLE   DestDescriptor
);
```

* `pResource` 
  렌더 대상으로 사용할 ComPtr 자원을 가리키는 포인터이다.
* `pDesc`
  A pointer to a **D3D12_RENDER_TARGET_VIEW_DESC** structure that describes the render-target view. 만약 무형식 자원이 아니라 구체적인 자료 형식을 지정해서 Resource 을 생성했다면 `nullptr` 로도 지정해도 된다. 그러면 메서드는 자원을 생성할 때 *mipmap* 수준을 생성한다.
  * 여기서는 후면 버퍼는 한 장의 밉맵만을 생성한다.
* `DescDescriptor`
  생성된 렌더 대상 뷰가 저장될 **서술자의 핸들**이다.

다음은 `void D3DApp::OnResize()` 에서 예시로 제시된 교환 사슬을 사용해 RTV 을 생성하는 코드이다.

``` c++
ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount]; (2)

CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
for (UINT i = 0; i < SwapChainBufferCount; i++)
{
	ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
	md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
	rtvHeapHandle.Offset(1, mRtvDescriptorSize);
}
```

### 4.3.8 DSV (Depth Stencil View) 생성

그 후에 깊이 및 스텐실 버퍼를 생성한다. 버퍼 자체는 텍스쳐이고, 텍스쳐는 GPU 자원의 하나이기 때문에 텍스쳐 자원을 서술하는 `D3D12_RESOURCE_DESC` 구조체를 채운 후, `ID3D12Device::CreateCommittedResource` 을 호출하면, DS 버퍼를 생성할 수 있다.

#### A. D3D12_RESOURCE_DESC

>  *D3D12_RESOURCE_DESC* 의 구조체는 다음과 같다.
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn903813%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

1. `Dimension` 텍스쳐 자원의 차원으로, ***`D3D12_RESOURCE_DIMENSION`*** 을 사용해 지정한다.

   > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770396(v=vs.85).aspx

2. `Width` `Height` 
   텍스쳐의 **너비 와 높이를** 지정한다. 만약 버퍼 자원의 경우에는 버퍼의 **바이트 개수**를 지정한다.

3. `DepthOrArraySize`
   3차원 텍스쳐의 **텍셀 단위 깊이** 또는 1차원 및 2차원 텍스쳐 배열의 크기를 지정한다.
   *OpenGL* 과 같이 3차원 텍스쳐들의 배열은 지원하지 않는다.

4. `MipLevels`
   밉맵 레벨의 수를 정한다. 후면 버퍼의 경우에는 $ 1 $ 로 해서 수준을 1 개만 생성할 수 있도록 한다.

5. `Format`
   각 텍셀의 자료 형식을 지정한다. DXGI_FORMAT 열거형을 지정한다.

   > https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx

6. `SampleDesc` 
   MSAA 의 표본 개수와 품질 수준을 지정한다. 다만, DSV 의 버퍼에 적용할 MSAA 설정들은 **렌더 대상에 쓰인 설정들과 부합**해야 한다. 예를 들어, 4x MSAA 의 경우에는 픽셀 당 네 개의 서브 픽셀에 대한 색상, 깊이, 스텐실 값이 있어야 하기 때문에 DSV 의 버퍼가 화면 해상도의 4배가 되어야 한다.

7. `Layout` 
   텍스쳐의 배치로, D3D12_TEXTURE_LAYOUT 의 열거형을 지정한다.

   > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770442(v=vs.85).aspx

8. `MiscFlags` 
   기타 자원 플래그들을 지정한다. 깊이 스텐실 버퍼 자원의 경우에는 D3D12_RESOURCE_MISC_DEPTH_STENCIL 을 지원한다. 그 외에는 다음을 참고하라.

   > https://msdn.microsoft.com/en-us/library/windows/desktop/dn986742(v=vs.85).aspx

#### B. ID3D12Device::CreateCommittedResource

GPU 자원들은 GPU 의 메모리 영역 중 힙에 존재한다. 이 GPU 힙은 GPU 메모리의 블록이면서, 특정한 속성들을가지고 있다. 이 메서드는 **자원을 GPU 힙에 생성하고**, **속성들에 부합하는 힙에 자원을 맡긴다**.

> ID3D12Device::CreateCommittedResource method
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn899178%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

``` c++
HRESULT CreateCommittedResource(
  [in]            const D3D12_HEAP_PROPERTIES *pHeapProperties,
                        D3D12_HEAP_FLAGS      HeapFlags,
  [in]            const D3D12_RESOURCE_DESC   *pDesc,
                        D3D12_RESOURCE_STATES InitialResourceState,
  [in, optional]  const D3D12_CLEAR_VALUE     *pOptimizedClearValue,
                        REFIID                riidResource,
  [out, optional]       void                  **ppvResource
);
```

1. `pHeapProperties`
   자원을 맡길 메모리 힙의 **속성**들을 담은 구조체를 가리키는 포인터이다. 
   이 패러미터의 타입인 D3D12_HEAP_PROPERTIES 는 다음 주소에서 확인할 수 있다.

   > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770373(v=vs.85).aspx
   > D3D12_HEAP_TYPE
   > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770374%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

2. `HeapMiscFlags` 
   자원을 맡길 힙이 가졌으면 하는 추가적인 속성을 지정한다. 없을 경우에는 `D3D12_HEAP_MISC_NONE` 을 지정한다.

3. `pResourceDesc`
   생성하고자 하는 자원을 서술하는 D3D12_RESOURCE_DESC 의 객체 포인ㅌ.

4. `InitialResourceState`
   쉐이더의 파이프라인에서 사용될 자원의 초기 상태를 지정한다. 이는 각자 다 다르며, 
   **DSV **로 사용할 자원은 `D3D12_RESOURCE_USAGE_INITIAL` 을 초기 상태로 지정한다. 생성 후에는 `D3D12_RESOURCE_USAGE_DEPTH` 으로 변경해서 DSV 버퍼로 파이프라인에 **묶어야 한다**.

5. `pOptimizedClearValue`
   자원 Reset 에 최적화된 값을 나타내는 `D3D12_CLEAR_VALUE` 구조체를 가리키는 포인터이다. 지우기 값을 설정하지 않을려면 nullptr 을 호출한다.

6. `riid` 생성하려는 자원에 해당하는 ComPtr 의 COM ID

7. `ppvResource` 새로 생성된 자원을 받는 `ID3D12Resource` 의 포인터.

> 최적의 성능을 위해서는 왠만해서는 자원을 **기본 힙**에 넣어야 한다. 그외 힙은 해당 기능이 필요할 때에만 사용한다.

이렇게 힙에 자원을 만든 후에, 자원을 사용해서 **DSV** Descriptor 을 생성한다. 

``` c++
// Create descriptor to mip level 0 of entire resource using the format of the resource.
D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
dsvDesc.Format = mDepthStencilFormat;
dsvDesc.Texture2D.MipSlice = 0;
md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());
```

그리고 초기 상태의 DSV 에 쓸 자원 상태를, **깊이 버퍼로 사용할 수 있는 상태로 전이한다**.

``` c++
// Transition the resource from its initial state to be used as a depth buffer.
mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
    mDepthStencilBuffer.Get(),
    D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
```

#### C. ID3D12Device::CreateDepthStencilView

> ID3D12Device::CreateDepthStencilView method
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn788661(v=vs.85).aspx

``` c++
void CreateDepthStencilView(
  [in, optional]       ID3D12Resource                *pResource,
  [in, optional] const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
  [in]                 D3D12_CPU_DESCRIPTOR_HANDLE   DestDescriptor
);
```

기본적인 원리는 CreateRenderTargetView 와 같다.

### 4.3.9 Viewport 설정

#### A. D3D12_VIEWPORT

> D3D12_VIEWPORT structure
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn770454(v=vs.85).aspx

장면을 그려 넣고자 하는 버퍼 (여기서는 후면 버퍼) 의 **부분 직사각형 영역**을 ***Viewport*** 라고 한다. 구조의 상세한 설명은 위 주소에서 볼 수 있다.

구조체 멤버에서, `MinDepth` 와 `MaxDepth` 가 있는데 이는 깊이 구간 $ [0, 1]$ 을 깊이 구간 $ [MinDepth, MaxDepth] $ 로 역정규화 하는데 쓰인다. 이를 활용해서 여러가지 효과를 구현할 수 있다. 예를 들면, 두 값이 $ 0 $ 이면 모든 물체보다 항상 앞에 나와있게 된다. 하지만 대개 보통은 $ [0, 1] $ 로 설정한다.

아무튼 `D3D12_VIEWPORT` 구조체를 모두 채우면, **`ID3D12CommandList::RSSetViewports` 메서드**를 사용해서 *Viewport* 을 *Direct3D* 에 설정한다.

#### B. 적용

다음은 후면 버퍼 전체에 장면을 그리는 뷰포트를 설정하도록 하는 커맨드를 삽입하는 예이다.

``` c++
D3D12_VIEWPORT vp;
vp.TopLeftX = 0.f;
vp.TopLeftY = 0.f;
vp.Width = static_cast<float>(mClientWidth);
vp.Height= static_cast<float>(mClientHeight);
vp.MinDepth = 0.f;
vp.MaxDepth = 1.f;

mCommandList->RSSetViewports(1, &vp);
```

* > ID3D12GraphicsCommandList::RSSetViewports method
  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn903900(v=vs.85).aspx

  ``` c++
  void RSSetViewports(
    [in]       UINT           NumViewports,
    [in] const D3D12_VIEWPORT *pViewports
  );
  ```

  Bind an array of viewports to the rasterizer stage of the pipeline. 

> 단순히 하나의 렌더 대상에 여러 개의 뷰포트를 지정할 수는 없다. 다중 뷰포트를 지정하기 위해서는 고급 테크닉이 필요하다.
>
> 또한 *Command List* 을 ***Reset*** 하면, Viewport 역시 재설정을 해야한다.

뷰포트를 활용해서 2인용 화면 모드를 그리게 할 수도 있다. 그럴 때는 각각 화면 절반의 뷰포트를 만들고 3차원 장면을 왼쪽 뷰포트 및 오른쪽 뷰포트에 그리게 하면 된다.

### 4.3.10 Scissor rectangle 설정

#### A. D3D12_RECT

* ***Scissor Rectangle*** 은 특정 픽셀들을 Culling 하는 용도로 쓰인다. 후면 버퍼를 기준으로 해서 *Scissor rectangle* 을 정의하고, 설정하면 렌더링 시에 가위 직사각형 바깥의 픽셀들은 최종적으로 후면 버퍼에 래스터화되지 않는다. 
  * 이는 일종의 **최적화 기법**이다. 

가위 직사각형을 *Direct3D 12* 에 설정할 때는, ***`ID3D12GraphicsCommandList::RSSetScissorRects`*** 라는 메서드를 사용한다.

* > ID3D12GraphicsCommandList::RSSetScissorRects method
  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn903899%28v=vs.85%29.aspx

  ``` c++
  void RSSetScissorRects(
    [in]       UINT       NumRects,
    [in] const D3D12_RECT *pRects
  );
  ```

#### B. 적용

다음은 후면 버퍼의 왼쪽 위 사분면 (제 2 사분면) 을 덮는 가위 삼각형이다.

``` c++
mScissorRect = { 0, 0, mClientWidth / 2, mClientHeight / 2 };
mCommandList->RSSetScissorRects(1, &mScissorRect);
```

물론 이 역시 커맨드 리스트가 Reset 이 되면 다시 설정해줘야 하는 수고가 든다.