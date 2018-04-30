# 6 :: Drawing in Direct 3D pt2

### 6.6.4 Constant buffer descriptor

자원을 렌더링 파이프라인에 묶으려면 **Descriptor** 가 필요하다. 렌더 타겟, 깊이 스텐실에서도 그래왔듯이 **상수 버퍼** 역시 **Descriptor**가 필요하다.

* 상수 버퍼 서술자는 **D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV** 형식의 서술자 힙에 담긴다. 이 힙 타입은, 상수 버퍼 뿐만 아니라 *쉐이더 자원 뷰*, *순서 없는 접근 뷰* 서술자들을 섞어서 담을 수 있다.

``` c++
D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
cbvHeapDesc.NumDescriptors = 1;
cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
cbvHeapdesc.NodeMask = 0;
ComPtr<ID3D12DescriptorHeap> mCbvHeap{ nullptr };
md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));
```

그리고 ID3D12Device::CreateConstantBufferView 을 호출해서 상수 버퍼 뷰를 생성한다. 

* **`ID3D12Device::CreateConstantBufferView`**

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788659(v=vs.85).aspx

  ``` c++
  void CreateConstantBufferView(
    [in, optional] const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
    [in]                 D3D12_CPU_DESCRIPTOR_HANDLE     DestDescriptor
  );
  ```

``` c++
struct ObjectConstants { XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4(); };
auto mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), n, true);
UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
int boxCBufIndex = i;
cbAddress += boxCBufIndex * objCBByteSize;

D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
cbvDesc.BufferLocation = cbAddress;
cbvDesc.SizeInBytes = objCByteSize;

md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
```

### 6.6.5 Root signature 및 Descriptor table

* 쉐이더 프로그램은 **특정 종류의 자원들이 렌더링 파이프라인에 묶인 상태에서 그리기 호출이 실행되었다고 가정한다**.
  * 따라서, 자원들은 **특정 레지스터 슬롯**에 묶이며, 쉐이더 프로그램들은 그 레지스터 슬롯에 접근해서 자원을 가져온다. 위의 쉐이더 프로그램에서 본, `register(b0)` 등이 대표적인 예시다.

#### A. Root signature 

* 루트 서명은 **그리기 호출 전에** 응용 프로그램이 반드시 렌더링 파이프라인에 묶어야 하는 자원이 무엇이고, 자원들이 바인딩될 **쉐이더 입력 레지스터**에 어떻게 대응이 되야 하는지를 정의한다. 


* 따라서, **Root signatrue** 은 그리기 호출 전에 렌더링 파이프라인에 묶였다고 쉐이더들이 기대하는 **모든 자원을 제공해야 한다**. 루트 서명이 제대로 만들어졌는가를 확인하는 것은 **상태 객체**를 생성할 때 검증된다.
  * 그리기 호출마다 서로 다른 쉐이더를 사용할 수 있는데, 이러면 루트 서명도 달라야 한다.

> 조금 더 생각하면, 쉐이더 프로그램은 본질적으로 하나의 함수이고
> 쉐이더에 입력되는 자원들은 함수의 매개변수에 해당하기 때문에, 루트 서명은 **함수의 서명**을 정의하는 수단이라고 할 수 있다.

*Direct3D* 에서 루트 서명을 대표하는 인터페이스는 **`ID3D12RootSignature`** 이다. 루트 서명은, 렌더링 호출에서 쉐이더들이 기대하는 자원들을 서술하는 **루트 매개변수의 배열** 로 이루어져 있다.

* **루트 매개변수 (root paramter)** 는 *root constant*, *root descriptor*, *descriptor table* 들 중 하나일 수 있다.

  > 앞의 두 개는 Chapter 7 에서 설명한다고 한다.

  * ***Descriptor table*** 은 *Descriptor heap* 안에 있는 연속된 서술자'들'의 구간을 지정한다.

다음 코드는 *root parameter* 하나로 된 루트 서명을 생성한다.

``` c++
/*! Root parameter is either table, root descriptor or root constant */
CD3D12_ROOT_PARAMETER slotRootParameter[1];
/*! Create Descriptor table accomodates one CVB */
CD3D12_DESCRIPTOR_RANGE cbvTable;
cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
              1, /*! 테이블의 서술자 개수 */
              0);/*! 이 root parameter에 묶일 쉐이더 인수들의 기준 레지스터 번호(b0) */
/*! 따라서, 해당 root signature 라면 루트 서명은 레지스터 b0 에 CBV 배열 정보를 공급할 것이다 */

slotRootParameter[0].InitAsDescriptorTable(
    1,          /*! The number of range */
	&cbvTable); /*! 구간들의 배열을 가리키는 포인터 */
/*! CBV 하나로 구성된 서술자 구간을 가리키는 루트 서명을 생성한다 */
CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
ComPtr<ID3DBlob> serializedRootSig = nullptr;
ComPtr<ID3DBlob> errorBlob = nullptr;
HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
                                        D3D_ROOT_SIGNATURE_VERSION_1,
                                        serializedRootSig.GetAddressOf(),
                                        errorBlob.GetAddressOf());
ThrowIfFailed(md3dDevice->CreateRootSignature(
    0,
    serializedRootSig->GetBufferPointer(),
    serializedRootSig->GetBufferSize(),
    IID_PPY_ARGS(&mRootSignature)
));
```

> CD3D12_ROOT_PARAMETER 및 CD3DX12_DESCRIPTOR_RANGE 는 다음 장에서 설명하고자 한다.

* 중요한 점은, **Root signature** 은 렌더링 파이프라인에 묶을 자원들을 정의하기만 하지, 이것을 실제로 루트 서명이 묶지는 않는다. 따라서 루트 서명의 정보를 따라 실제로 Input assembly 에서 묶어주는 것이 필요하다.
  이를 위해 필요한 메서드가 `ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable` 이다.

  * ``` c++
    void SetGraphicsRootDescriptorTable(
      [in] UINT                        RootParameterIndex,
      [in] D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor
    );
    ```

``` c++
mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
ID3D12DescriptorHeap* descriptorHeaps[] { mCbvHeap.Get() };
mCommandList->setDescriptorHeap(_countof(descriptorHeaps), descriptorHeaps);

CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
cbv.Offset(cbvIndex, mCbvSrvUavDescriptorSize);
mCommandList->SetGraphicsRootDescriptorTable(0, cbv);
```

이렇게 함으로써 상수 버퍼를 만들고, 거기에 서술자를 만들고 그리고 서술자를 담는 root signature 를 만들어, 패러미터를 담고 테이블을 만들고 정보를 담아서 명령으로 넘겨주고, 마지막으로 정보를 쉐이더에게 줄 수 있도록 명령을 할당했다.

> 성능을 위해서는 *root signature* 을 최대한 작게 만든다. 그리고 루트 서명의 변경을 최소화 해야한다.
> 또한 루트 서명을 바꾸면, 기존의 모든 바인딩이 사라지기 때문에 다시 묶어야 한다.

## 6.7 Shader compile

*Direct3D* 에서 쉐이더 프로그램은 먼저 플랫폼 이식성이 있는 **바이트 코드**로 컴파일이 되어야 한다. 그리고 그래픽 드라이버는 해당 바이트코드를 *GPU*에 맞도록 최적의 네이티브 명령어로 컴파일한다.

> Dark secrets of shader development
> http://mgun.tistory.com/2148

어플리케이션 런타임 시점에서 다음 함수를 사용해서 쉐이더를 컴파일 할 수 있다.

* **`D3DCompileFromFile`**

  > https://msdn.microsoft.com/en-us/library/windows/desktop/hh446872(v=vs.85).aspx

  ``` c++
  HRESULT WINAPI D3DCompileFromFile(
    in      LPCWSTR pFileName,
    in_opt  const D3D_SHADER_MACRO pDefines,
    in_opt  ID3DInclude pInclude,
    in      LPCSTR pEntrypoint,
    in      LPCSTR pTarget,
    in      UINT Flags1,
    in      UINT Flags2,
    out     ID3DBlob ppCode,
    out_opt ID3DBlob ppErrorMsgs
  );
  ```

> 덤, ID3DBlob 인터페이스는 임의 길이의 문자열을 저장하는 인터페이스이다.
> https://msdn.microsoft.com/en-us/library/windows/desktop/ff728743(v=vs.85).aspx

쉐이더를 컴파일 했다고 해서 바로 쉐이더를 쓸 수 있지는 않다. *OpenGL* 처럼 렌더링 파이프라인에 묶어야 한다.

### 6.7.1 Offline compile

*OpenGL* 과는 다르게 쉐이더를 실행 시점에서 컴파일하지 않고, 오프라인에서 개별적으로 컴파일 하게 할 수도 있다. 이렇게 하면 다음과 같은 효과를 기대해볼 수 있다.

1. 복잡한 쉐이더를 런타임에서 컴파일하면 시간이 오래 걸리는 단점을 해결할 수 있다.
2. 쉐이더 컴파일 오류를 빌드 과정에서 일찍 점검할 수 있다.
3. *Windows 8* 스토어 앱들은 오프라인 컴파일을 하지 않으면 빠꾸당한다

* 컴파일된 쉐이더를 담는 확장자로는 **`.cso`** 를 사용한다.


* 쉐이더를 오프라인에서 컴파일할려고 하면, *DirectX* 에 포함된 ***FXC*** 도구를 사용한다. 이것은 명령줄 도구 (콘솔에서 컴파일 해야한다) 예를 들어서 color.hlsl 에 담긴 VS 의 진입점이 VS 이고, PS 의 진입점이 PS 인 쉐이더 프로그램이 존재하면 콘솔 창에서 다음과 같은 명령을 실행해야 해서 *디버그 모드*용 쉐이더를 얻을 수 있다.

  ``` c++
  fxc "color.hlsl" /Od /Zi /T vs_5_0 /E "VS" /Fo "color_vs.cso" /Fc "color_vs.asm"
  ```

  *릴리즈 모드*로 컴파일할려면, 앞의 /Od /Zi 옵션을 빼면 된다.

  > 자세한 것은 다음, Offline compling 란을 참고한다.
  > https://msdn.microsoft.com/en-us/library/windows/desktop/bb509710(v=vs.85).aspx

이렇게 하면 D3DCompilerFromFile 을 호출할 필요가 없다. 하지만 `cso` 의 바이트코드를 응용 프로그램에 적재를 해야하기 때문에, 다음 코드를 사용해서 `ID3DBlob` 에 바이트코드 문자열을 옮겨야 한다.

``` c++
std::ifstream fin(filename, std::ios::binary);
fin.seekg(0, std::ios_base::end);
std::ifstream::pos_type size = (int)fin.tellg();
fin.seekg(0, std::ios_base::beg);

ComPtr<ID3DBlob> blob;
ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

fin.read((char*)blob->GetBufferPointer(), size);
fin.close();
```

이는 현재 쓰고 있는 D3DUtil 의 `LoadBinary` 함수로 구현되어 있다.

## 6.8 Raseterizer state

렌더링 파이프라인의 많은 부분이 프로그래밍 가능한 스테이지로 구성되어 있으나, 일부는 *FIxed function* 으로 설정만 가능하다. 렌더링 파이프라인의 *Rasterization* 상태가 바로 그것이다. 래스터화 단계는 ***Rasterizer state***를 통해서 구성한다.

이 상태를 대표하는 것은 `D3D12_RASTERIZER_DESC` 구조체이다.

* **`D3D12_RASTERIZER_DESC` **

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770387(v=vs.85).aspx

  ``` c++
  typedef struct D3D12_RASTERIZER_DESC {
    D3D12_FILL_MODE                       FillMode;
    D3D12_CULL_MODE                       CullMode;
    BOOL                                  FrontCounterClockwise;
    INT                                   DepthBias;
    FLOAT                                 DepthBiasClamp;
    FLOAT                                 SlopeScaledDepthBias;
    BOOL                                  DepthClipEnable;
    BOOL                                  MultisampleEnable;
    BOOL                                  AntialiasedLineEnable;
    UINT                                  ForcedSampleCount;
    D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
  } D3D12_RASTERIZER_DESC;
  ```

  1. `FillMode` 와이어프레임 렌더링을 할 지, 아니면 속을 채운 기본 렌더링을 할 지 결정한다.

     > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770366(v=vs.85).aspx

  2. `CullMode` BackFaceCulling 을 끄거나, FRONT , BACK 중 하나를 설정해서 해당 면을 컬링할 수 있다.

     > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770354(v=vs.85).aspx

  3. `FrontCounterCloseWise` 정점들이 반시계방향으로 구성될 때 이를 FRONT 면으로 할지, BACK 면으로 할지 결정할 수 있다.

따라서 다음과 같이 임의 *Rasterizaer state* 을 생성할 수 있다.

``` c++
CD3DX12_RASTERIZER_DESC rsDesc(D3D12_DEFAULT);
rsDesc.FillMode = D3D12_FILL_WIREFRAME;
rsDesc.CullMode = D3D12_CULL_NONE;
```

* 여기서 `D3D12_DEFAULT` 는 *Rasterizaer state* 의 값을 기본으로 초기화한다.

## 6.9 파이프라인 상태 객체 (Pipeline state object)

이제 객체들을 실제로 사용하기 전에, **렌더링 파이프라인에 묶어야 한다**. 렌더링 파이프라인의 상태를 제어하는 대부분의 객체는 ***Pipeline state object*** 라고 부르는 집합체에 지정된다.

* *Direct3D 12* 에서 ***PSO*** 을 대표하는 인터페이스는 **`ID3D12PipelineState`** 이다. PSO 을 생성하려면 그에 맞는 서술자인 **D3D12_GRAPHICS_PIPELINE_STATE_DESC** 구조체 정보를 채워야 한다.

> D3D12_GRAPHICS_PIPELINE_STATE_DESC 에 대해서는 다음을 참고한다.
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn770370%28v=vs.85%29.aspx

1. `pRootSignature` *PSO* 와 함께 묶을 루트 서명을 가리키는 포인터. 루트 서명은 *PSO* 에 묶을 쉐이더와 호환이 되어야 한다.

2. `D3D12_SHADER_BYTECODE` 각 정점 쉐이더를 기술하는 쉐이더 바이트코드 구조체. 이 구조체는 컴파일된 바이트 코드 자료를 가리키는 포인터와 바이트 코드의 크기를 구성한다.

   > **D3D12_SHADER_BYTECODE**
   > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770405(v=vs.85).aspx

D3D12_GRAPHICS_PIPELINE_STATE_DESC 인터페이스를 채운 후에는, **`ID3D12Device::CreateGraphicsPipelineState`** 을 사용해서 `ID3D12PipelineState` 객체를 생성한다.

``` c++
ComPtr<ID3D12RootSignature> mRootSignature;
std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
ComPtr<ID3DBlob> mVsByteCode;
ComPtr<ID3DBlob> mPsByteCode;
/*! ... */
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), 
              mvsByteCode->GetBufferSize() };
psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
             mpsByteCode->GetBufferSize() };
psoDesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
psoDesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
psoDesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
psoDesc.SampleMask = UNIT_MAX;
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = mBackBufferFormat;
psoDesc.SampleDesc.Count = m4xMsaaState : 4 : 1;
psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
psoDesc.DSVFormat = mDepthStencilFormat;

ComPtr<ID3D12PipelineState> mPSO;
md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
```

이렇게 상당히 vorbose 하게 설정을 하지 않으면 안되는데, 이유는 **성능**때문이다. 모든 설정을 하나의 집합체로 설정하기 때문에 *Direct3D 12* 는 모든 상태가 잘 호환되는지 미리 검증할 수 있다. *Direct3D 11* 에서는 이런 설정들을 개별적으로 관리를 했는데 따라서 하드웨어에서 프로그래밍을 위한 코드를 생성할 때 설정 객체들을 계속 검증해서 다시 프로그래밍해야 한다. 

> *Direct3D 11* 에서는 설정을 개별 조각으로 설정했기 때문에 파이프라인을 구성하는 도중에 여러 조각을 변경하게 되면 파이프라인을 위한 코드를 생성하는 것이 그리기 호출 시점으로 미뤄질 수 밖에 없다. 그러나 하드웨어 프로그래밍을 그런 식으로 미루면 실행 시점에서 드라이버가 추가적인 관리 작업을 수행해야 한다.
>
> 하지만 *Direct3D 12* 에서는 모든 상태를 하나의 구조체에 다 담아서 파이프라인 설정을 하기 때문에 미리 코드를 만들어 성능을 극대화 할 수 있다.

> **PSO 검증**은 시간이 많이 걸리기 때문에 가급적이면 초기화 시점에서 온전히 정보를 담아서 생성해야 한다. 또는 그렇지 못할 경우에는 해시 테이블처럼 빠르게 조회할 수 있는 컬렉션에 담아두고 필요할 때마다 여러 PSO 을 꺼내 쓰는 방법도 있긴 하다.

* *PSO* 는 모든 렌더 상태를 포함하지는 않는다. 뷰포트, Scissor rectangle 등은 따로 저장한다.
  * 어떤 *PSO* 을 명령 목록에 묶었다면, 다른 *PSO* 가 묶이기 전 까지는 해당 *PSO* 가 계속 렌더링 파이프라인에 적용이 된다.

## 6.11 상자 예제

예제의 모든 코드 줄을 이해할 수 있어야 한다...

> Chapter 6 / _11_boxapp.cc 참고.