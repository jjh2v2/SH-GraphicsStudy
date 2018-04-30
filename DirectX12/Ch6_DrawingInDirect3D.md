# 6 :: Drawing in Direct3D

#### 목표

1. Geometry Data 의 정의와 저장, 그리기 (Drawing) 을 위한 D3D COM interface 와 제공하는 멤버 함수를 파악한다.
2. 기본적인 Vertex, Pixel Shader (Fragment shader) 을 작성하는 법을 배운다.
3. 파이프라인 상태 객체들을 이용해 렌더링 파이프라인을 구성하는 방법을 파악한다.
4. Constant buffer 자료를 생성해서 파이프라인에 묶는 방법을 배우고, Root signature 에 익숙해진다.

## 6.1 Vertices and Input layouts

* *Direct3D* 의 정점에 공간 위치 이외의 **추가적인 자료**를 부여할 수 있는데, 이 원하는 자료를 가진 ***Custom vertex format*** 을 만들려면 그러한 자료를 담을 **구조체**를 정의해야 한다.

다음은 서로 다른 두 가지 정점 형식의 예이다.

``` c++
struct Vertex1 { XMFLOAT3 pos; XMFLOAT4 color; };
struct Vertex2 { XMFLOAT3 pos; XMFLOAT4 normal; XMFLOAT2 tex_0; XMFLOAT2 tex_1; };
```

**정점 구조체**를 정의한 다음에는 정점 구조체의 각 데이터로 무엇을 해야하는가를 *Direct3D* 에 **알려주어야 한다**. 이런 정보를 *Direct3D* 에게 알려주는 수단으로, ***Input layout description*** 이 쓰일 수 있다. 이는 밑의 `D3D12_INPUT_LAYOUT_DESC` 로 쓸 수 있다.

* ***D3D12_INPUT_LAYOUT_DESC***

  > https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn770378(v=vs.85).aspx

  ``` c++
  typedef struct D3D12_INPUT_LAYOUT_DESC {
    const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs;
    UINT                           NumElements;
  } D3D12_INPUT_LAYOUT_DESC;
  ```

  이는 단지 `D3D12_INPUT_ELEMENT_DESC` 의 배열의 포인터와 그 배열이 갯수를 담은 구조체이다.

D3D12_INPUT_ELEMENT_DESC 을 설명하기에 앞서서, 해당 정점 구조체를 미리 바인딩할 배열을 선언하는 것을 HLSL 와 같이 붙여서 코드로 첨부한다.

``` c++
struct Vertex {
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 tex0;
    XMFLOAT2 tex1;
};
D3D12_INPUT_ELEMENT_DESC vertexDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D12_INPUT_PER_VERTEX_DATA, 0}
};
/*! HLSL */
VertexOut VS(float3 iPos : POSITION,
             float3 iNormal : NORMAL,
             float2 iTex0 : TEXCOORD0,
             float2 iTex1 : TEXCOORD1)
```

* ***D3D12_INPUT_ELEMENT_DESC***

  > https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn770377(v=vs.85).aspx

  Describes a single element for the input-assembler stage of the graphics pipeline.

  ``` c++
  typedef struct D3D12_INPUT_ELEMENT_DESC {
    LPCSTR                     SemanticName;
    UINT                       SemanticIndex;
    DXGI_FORMAT                Format;
    UINT                       InputSlot;
    UINT                       AlignedByteOffset;
    D3D12_INPUT_CLASSIFICATION InputSlotClass;
    UINT                       InstanceDataStepRate;
  } D3D12_INPUT_ELEMENT_DESC;
  ```

  해당 원소 서술자는, 그래픽 파이프라인의 첫 스테이지인 *Input-assembler* 에서의 각 원소의 형식을 서술한다. 이 배열의 원소들과 정점 구조체의 성분들은 **일대일 대응**이 되어야 한다. 예를 들어, 정점 구조체의 성분이 $ 2 $ 개이면, 그에 해당되는 **D3D12_INPUT_ELEMENT_DESC** 의 원소 역시 $ 2 $ 개가 되어야 한다.

  1. `SemanticName`
     ***HLSL*** 에서 쓰일 성분에 부여된 문자열 이름. 이것을 통해서 정점 쉐이더 등에서 성분을 찾을 수 있기 때문에 반드시 유효한 이름이 적혀져야 한다. 이를 *semantic* 이라고도 하는데, 구조체의 성분을 *Vertex shader* 입력 서명과 대응 시키는 역할을 한다.

  2. `SemanticIndex`
     해당 *semantic* 에 부여된 *index*. 이 색인은 Name 에 붙어서 $ 0...N $ 가 되는데, 이 색인이 필요한 이유는 D3D12_INPUT_ELEMENT_DESC 을 정의할 때 각 텍스쳐 좌표에 고유한 *semantic* 이름을 부여하는 대신에 그냥 색인을 통해서 구별하기 위함이다.

  3. `Format` **DXGI_FORMAT** 열거형의 멤버를 가지며, 성분의 비트 타입을 추정한다.

  4. `InputSlot`
     이 성분의 자료를 가져올 **Input slot** 의 *index* 이다. *Direct3D* 에서는 총 $ 16$ 개의 Input slot ($ 0 ~ 15$) 을 지원하는데, 현재는 입력 슬롯을 하나만 사용하기로 한다.

     > 자세한 것은 다음 주소를 참고한다.
     > https://msdn.microsoft.com/ko-kr/library/windows/desktop/bb205117(v=vs.85).aspx#Input_Slots

  5. `AlignedByteOffset`
     지정된 입력 슬롯에서, *C++ 정점 구조체*의 각 성분의 **시작 위치를 나타내는 offset**이다. 이 offset 은 공간이 꽉 차 있을 경우에는 안 써도 무방하다. 
     Optional. Offset, in bytes, between each element. Use D3D12_APPEND_ALIGNED_ELEMENT (0xffffffff) for convenience to define the current element directly after the previous one, including any packing if necessary.

  6. `InputSlotClass` 
     A value that identifies the input data class for a single input ``slot. 인스턴싱 등의 고급 기법에 쓰인다. 지금은 D3D12_INPUT_PER_VERTEX_DATA 을 넣어서, 한 정점마다 성분을 가져올 수 있도록 한다.

  7. `InstanceDataStepRate`
     The number of instances to draw using the same per-instance data before advancing in the buffer by one element. This value must be 0 for an element that contains per-vertex data.

## 6.2 Vertex Buffers

#### A. Vertex buffer 소개

*GPU* 가 정점들의 배열에 접근하려면, 해당 배열을 *buffer* 라고 부르는 *GPU* 자원, **`ID3D12Resource`** 에 넣어야 한다. *Vertices* 을 저장하는 버퍼를 *Vertex buffer* 이라고 한다. 아무튼 응용 프로그램에서 일련의 데이터를 전송할 때는 *Buffer* 을 사용해서 *GPU* 에 넘겨줘야 한다.

* ***Vertex buffer*** 을 생성하려면, *buffer resource* 을 서술하는 `D3D12_RESOURCE_DESC` 에 정보를 채우고, 그 뒤에 `ID3D12Device::CreateCommittedResource` 멤버 함수를 호출해서 ID3D12Resource 객체를 생성한다.

> ID3D12Resource interface
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn788709%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
>
> ID3D12Device::CreateCommittedResource method
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn899178(v=vs.85).aspx

* **CD3D12_RESOURCE_DESC** 는 D3D12_RESOURCE_DESC 을 상속해서 편의용 생성자 및 메서드를 추가했기 때문에 기초적인 동작을 원하면 이를 써도 된다.

  * `Buffer` 메서드 및 `Tex1D` `Tex2D` `Tex3D` 등이 있어서 간단하게 D3D12_RESOURCE_DESC 을 가져올 수 있다. 

  > 자세한 것은 다음 주소를 참고한다.
  > https://msdn.microsoft.com/en-us/library/windows/desktop/mt186577(v=vs.85).aspx

* 사실, **ID3D12Resource** 인터페이스는 *Direct3D 11* 까지와는 다르게 모든 *12* 의 자원을 대표하는 형식이다. *Direct3D 11* 까지만 해도 Texture2D, Buffer, 등과 같이 인터페이스가 달랐다. *Direct3D 12* 에서 ID3D12Resource 을 사용할 때, **자원의 구체적인 종류** 는 **D3D12_RESOURCE_DESC::D3D12_RESOURCE_DIMENSION** 필드로 지정된다. 

  > D3D12_RESOURCE_DIMENSION enumeration
  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770396%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

STATIC 한 기하 구조를 그릴 때는 최적의 성능을 위해서 정점 버퍼의 정보를 VRAM 의 기본 힙 (**D3D12_HEAP_TYPE_DEFAULT**) 에 넣는다. 정적 기하구조의 경우에는 정점 버퍼를 특정 값으로 초기화한 후에는 *GPU* 만 정점의 버퍼를 읽으므로 기본 힙에 넣는 것이 타당하다. *CPU* 는 초기화된 *VRAM* 의 기본 힙의 버퍼를 수정하지 못한다. 

* 맨 처음 정적 기하구조를 위해 값을 초기화할 때는, D3D12_RESOURCE_DESC 의 정점 버퍼 자원을 생성하는 것과 더불어서 **D3D12_HEAP_TYPE_UPLOAD** 형식의 힙에 **임시 업로드용 버퍼 자원**을 생성해야 한다. 업로드 버퍼를 생성한 다음에는 일반 RAM 의 정점 자료들을 업로드 버퍼로 복사하고, 그 다음에 업로드 버퍼의 정점 자료를 실제 VRAM 의 정점 버퍼에 복사하도록 명령 리스트로 요청한다.

  > 자세한 것은 p148 혹은 4.3.8 을 참고한다.

  * 버퍼 자원을 생성할려면 `ID3D12Device::CreateCommittedResource` 을 사용한다.

  > 임시 버퍼 자원을 이용해서 실제 버퍼 자원의 공간에 할당하는 방법은 d3dUtil::CreateDefaultBuffer() 을 참고한다. 

다만 함수에서 업로드 버퍼에서 실제 버퍼로 값을 복사할 때 쓸 구조체가 있는데 이것은 `<d3dx12.h>` 에 정의된 `UpdateSubresources` 와 `D3D12_SUBRESOURCE_DATA` 이다.

* **D3D12_SUBRESOURCE_DATA** structure

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn879485%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  typedef struct D3D12_SUBRESOURCE_DATA {
    const void *pData;
    LONG_PTR   RowPitch;
    LONG_PTR   SlicePitch;
  } D3D12_SUBRESOURCE_DATA;
  ```

  1. `pData`  A pointer to a memory block that contains the subresource data.
  2.  `RowPitch` 버퍼의 경우 복사할 자료의 크기 (바이트 수) The row pitch, or width, or physical size, in bytes, of the subresource data
  3. `SlicePitch` The depth pitch, or width, or physical size, in bytes, of the subresource data.

#### B. 예시

``` c++
Vertex vertices[] = {
    { XMFLOAT3(-1.f, -1.f, -1.f), XMFLOAT4(Colors::White) },
    { XMFLOAT3(-1.f, +1.f, -1.f), XMFLOAT4(Colors::Black) },
    { XMFLOAT3(+1.f, +1.f, -1.f), XMFLOAT4(Colors::Red) },
    /*! ... 총 8개가 초기화됨 */
};
const UINT64 vb_byte_size = 8 * sizeof Vertex;
ComPtr<ID3D12Resource> VertexBufferGPU { nullptr };
ComPtr<ID3D12Resource> VertexBufferUploader { nullptr };
VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
                                              mCommandList.Get(), vertices, vb_byte_size,
                                              VertexBufferUploader);
```

여기서 업로드 버퍼는 업로드 큐에서 값 복사가 실행되기 전 까지는 할당해제 되서는 안된다.

#### C. Vertex buffer view

* 정점 버퍼를 파이프라인에 묶으려면, **정점 버퍼 자원을 서술하는 VBV (Vertex Buffer View)** 가 필요하다. 하지만 *RTV* 와는 다르게 정점 버퍼 뷰에는 **Allocator heap** 이 필요하지 않다고 한다.
  *VBV* 을 대표하는 형식은 **D3D12_VERTEX_BUFFER_VIEW_DESC** 이다.

* **`D3D12_VERTEX_BUFFER_VIEW`** structure

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn903819(v=vs.85).aspx

  ``` c++
  typedef struct D3D12_VERTEX_BUFFER_VIEW {
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT                      SizeInBytes;
    UINT                      StrideInBytes;
  } D3D12_VERTEX_BUFFER_VIEW;
  ```

여기까지, **정점 배열의 값, 정점 버퍼, 그리고 VBV**까지 생성했다. 그러면 정점 버퍼를 파이프라인의 한 **Input slot**에 묶을 수 있다. 인풋 슬롯에 묶기 위해서는 다음을 사용한다.

* ***`ID3D12GraphicsCommandList::IASetVertexBuffers`***

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn986883%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  void IASetVertexBuffers(
    [in]                 UINT                     StartSlot,
    [in]                 UINT                     NumViews,
    [in, optional] const D3D12_VERTEX_BUFFER_VIEW *pViews
  );
  ```

#### D. VBV, Input slot 적용

``` c++
D3D12_VERTEX_BUFFER_VIEW vbv;
vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
vbv.StrideInBytes = sizeof(Vertex);
vbv.SizeInBytes = 8 * sizeof(Vertex);

D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
mCommandList->IASetVertexBuffers(0, 1, vertexBuffers);
```

일단 입력 슬롯에 묶은 정점 버퍼는, **다시 변경하지 않는 한 계속 그 입력 슬롯에 묶여 있다**. 만약에 정점 버퍼를 여러개 사용한다고 하면 다음과 같이 쓸 수 있을 것이다.

``` c++
ComPtr<ID3D12Resource> mVb1{nullptr};
ComPtr<ID3D12Resource> mVb2{nullptr};
D3D12_VERTEX_BUFFER_VIEW_DESC mVbView1;
D3D12_VERTEX_BUFFER_VIEW_DESC mVbView2;
/*! 정점 버퍼와 뷰를 생성... */
mCommandList->IASetVertexBuffers(0, 1, &VbView1);
/*! 정점 버퍼 1 을 사용해서 물체들을 그리도록 명령어를 추가 할당한다 */
mCommandList->IASetVertexBuffers(0, 1, &VbView2);
/*! 정점 버퍼 2 를 사용해서 물체들을 그리도록 명령어를 추가 할당한다 */
/*! 모든 렌더링이 끝나면 ID3DResource 및 버퍼가 메모리 누출이 되지 않도록 조심해서 끝낸다 */
```

여기서 주의해야 할 것은, **정점 버퍼를 입력 슬롯에 설정한다고 해서 버퍼의 정점들이 그려지는 것이 아니며, 단지 그 정점들을 파이프라인의 Input assembly 에서 공급할 준비가 되도록 할 뿐이다.**

#### E. DrawInstanced

따라서 정점을 실제로 그릴려면 **`ID3D12GraphicsCommandList::DrawInstanced`** 을 호출해야 한다.

* **`ID3D12GraphicsCommandList::DrawInstanced`**

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn903877%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  void DrawInstanced(
    [in] UINT VertexCountPerInstance,
    [in] UINT InstanceCount,
    [in] UINT StartVertexLocation,
    [in] UINT StartInstanceLocation
  );
  ```

  1. `VertexCountPerInstance` 인스턴스 당 그릴 정점의 개수
  2. `InstanceCount` 인스턴싱할 인스턴스의 개수, 하나만 그릴거면 $ 1 $ 로 족하다.
  3. `StartVetexLocation` *OpenGL* 의 *basevertex* 와 같으며, 불러올 정점의 추가 인덱스.
  4. `StartInstanceLocation` *OpenGL* 의 *baseInstance* 와 같으며, 불러올 인스턴스 정보의 인덱스의 추가 인덱스.

그런데 문제는, 정점 버퍼를 읽게 해서 그리는 건 그렇다 치더라도, **Primitive topology** 가 인자에 없다는 것이 우려스럽다. 하지만 이 경우에는 *DrawInstanced* 을 호출하기 전에 **`ID3D12GraphicsCommandList::IASetPrimitiveTopology`** 을 사용해서 어느 프리미티브를 사용하게 할 것인지를 결정해야 한다.

``` c++
cmdList->IASetPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
```

## 6.3 Index 및 Index buffer

#### A. Index buffer

*OpenGL* 에서 말하는 Element 가 *Direct3D* 에서는 Index 인데, *GPU* 가 index 의 배열에 접근할 수 있으려면 이 역시 위처럼 버퍼를 만들고, 버퍼의 뷰를 만들고, 그리고 버퍼 뷰를 파이프라인에 묶어야 한다.

버퍼의 뷰를 만드는 것은 그냥 `d3dUtil::CreateDefaultBuffer` 등을 사용해서 임시 버퍼 객체를 만들어 그것을 실제 *VRAM* 의 기본 힙에 위치한 *Index buffer* with ID3D12Resource 에 묶는다.

그 후에는 과정이 약간 다른데, 일반 정점 버퍼와는 다르게 색인 버퍼는 **따로 Index Buffer View** 가 존재한다. 이 뷰 역시 *VBV* 처럼 서술자 힙이 필요하지 않다.

* **`D3D12_INDEX_BUFFER_VIEW`**

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn891445%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  typedef struct D3D12_INDEX_BUFFER_VIEW {
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT                      SizeInBytes;
    DXGI_FORMAT               Format;
  } D3D12_INDEX_BUFFER_VIEW;
  ```

* **`ID3D12CommandList::SetIndexBuffer`** 

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn986882%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  void IASetIndexBuffer(
    [in, optional] const D3D12_INDEX_BUFFER_VIEW *pView
  );
  ```

  *GPU* 단으로 생성한 인덱스 버퍼를 파이프 입력 어셈블리에 묶도록 한다.

#### B. Example

``` c++
std::uint16_t indices[] { 0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
                        3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7 };
const UINT ib_byte_size = 36 * sizeof(std::uint16_t);
ComPtr<ID3D12Resource> IndexBufferGPU { nullptr };
ComPtr<ID3D12Resource> IndexBufferUploader { nullptr };
IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
                                             mCommandList.Get(), indices, ibByteSize,
                                              IndexBufferUploader);
D3D12_INDEX_BUFFER_VIEW ibv;
ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
ibv.Format = DXGI_FORMAT_R16_UINT;
ibv.SizeInBytes = ibByteSize;

mCommandList->IASetIndexBuffer(&ibv);
```

마지막으로 Index buffer 을 사용해서 렌더링을 할 경우에는 다음 멤버 함수를 사용해야 한다.

* **`ID3D12GraphicsCommandList::DrawIndexedInstanced`**

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn903874(v=vs.85).aspx


## 6.4 Vertex Shader 의 예제

#### A. Code

다음은 간단한 정점 쉐이더의 구현이다.

``` c++
cbuffer cbPerObject : register(b0) {
    float4x4 gWorldViewProj;
};

void VS(float3 iPosL : POSITION, float4 iColor : COLOR,
        out float4 oPosH : SV_POSITION, out float4 oColor : COLOR) {
    oPosH = mul(float4(iPosL, 1.f), gWorldViewProj);
    oColor = iColor;
}
```

*Direct3D 12* 의 쉐이더는, *OpenGL* 이 GLSL 이라는 언어로 그래왔던 것 처럼 HLSL (High level shading language) 라는 언어로 작성이 가능하다.

> Appendix B 에서 HLSL 의 문법에 대해 볼 수 있다.

1. 본질적으로 **각각의 쉐이더는 하나의 메인 함수**를 가진다. 지금 예에서는 *VS* 라는 이름을 사용했으나, **유효한 함수 이름이면 어떤 것이든 쉐이더의 이름으로 가져올 수 있다.** 또한 정점 쉐이더는 매개변수가 $ 4 $ 개로 되어있는데, 처음 둘은 입력이고, 나머지 둘 그러니까 **`out`** 으로 지정된 매개변수는 다음 스테이지에 넘길 출력 변수이다.
2. HLSL 은 glsl 와 같이 포인터나 참조가 없다. 따라서 함수가 여러 개의 값을 돌려주려면 다음과 같은 짓을 해야한다. 
   1. ***구조체***를 사용한다.
   2. ***`out`*** specifier 을 사용해서 함수 밖으로 넘긴다.
3. HLS 에서 함수는 항상 인라인화 된다. 

#### B. Analysis

* 처음의 두 입력 매개 변수, `POSITION` 과 `COLOR` 은 *Vertex shader* 의 ***Input signature*** 을 형성한다. 이들은 현재의 그리기 작업에 **쓰이는 Custom buffer structure 의 멤버**와 대응이 된다.
  * 매개변수 *semantic* `:POSITION` 및 `:COLOR` 은 Resource 버퍼에 있는 정점 구조체의 멤버가 참조될 메모리 위치의 값을 정점 쉐이더 입력 매개변수에 대응시키는 역할을 한다.

``` c++
struct Vertex { XMFLOAT3 Pos; XMFLOAT4 Color; };
D3D12_INPUT_ELEMENT_DESC vertexDesc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0}
};
```

* **`out`** 지정자의 출력 변수에도 *semantic* 이 있다. 이들은 정점 쉐이더의 출력을 **파이프라인의 다음 단계 임의 프로그램된 쉐이더 스테이지**에 대응시키는 역할을 한다.

  * ***`SV_POSITION`*** 의 **SV** 는 *system value* 의 약어로, 해당 의미소가 쉐이더에 미리 내장된 의미소임을 말한다. 해당 *SV_POSITION* 은 출력 성분이 **Clip space** 에서의 정점의 위치를 알리는 의미소이다.
  * *GPU* 는 Clipping, Depth test, Rasterization 등등 다른 특성들에 적용되지 않는 고유한 의미소를 각 쉐이더에서 내포하고 있기 때문에, 이처럼 *SV_* 가 앞에 붙은 의미소를 잘 활용해야 한다.
  * 다만, `out` 의 `COLOR` 은 위의 *SV_* 와는 약간 다른 것이, 그냥 어플리케이션이 **D3D12_INPUT_ELEMENT_DESC** 배열을 통해서 지정한 이름이다. 시스템 값 의미소가 아닌 출력 매개변수 의미소에는 HLSL 에서 식별할 수 있기만 하면 어떤 이름도 사용이 가능하다.

* *Vertex shader* 에서 함수 본문의 첫 줄을 보면, 다음과 같다.

  ``` c++
  oPosH = mul(float4(iPosL, 1.f), gWorldViewProj);
  ```

  `mul(x, y)` 는 `x` 벡터 혹은 매트릭스 및 `y` 벡터 혹은 매트릭스를 곱해서 돌려준다. 이 함수는 HLSL 의 내장 함수인데, 여기서 다양한 내장 함수를 확인할 수 있다.

  > https://msdn.microsoft.com/ko-kr/library/windows/desktop/ff471376(v=vs.85).aspx

  * Matrix 변수 *gWorldViewProj* 는 **Constant Buffer (상수 버퍼)**에 들어있는 정보이다. 

또한 다음과 같이, 구조체를 사용해서 깔끔하게 전달할 수도 있다.

``` c++
cbuffer cbPerObject : register(b0) { float4x4 gWorldViewProj; };
struct VertexIn { float3 PosL : POSITION; float4 Color : COLOR; };
struct VertexOut { float4 PosH : SV_POSITION; float4 Color : COLOR; };
VertexOut VS(VertexIn vin) {
    VertexOut vout;
    vout.PosH = mul(float4(vin.PosL, 1.f), gWorldViewProj);
    vout.Color = vin.Color;
    return vout;
}
```

* *Geometry shader* 을 사용하지 않을 때는, 반드시 정점의 최종 출력 위치를 `SV_POSITION` *semantic* 을 가진 변수에 전달을 해야한다. 만약 쓸 경우에는 최종 정점 위치 출력을 *Geometry shader* 까지 미룰 수 있다.
* *OpenGL* 과 마찬가지로, *Direct3D* 에서도 원근 나누기 ($w$ 로 나누기) 는 하지 말아야 한다. 이는 렌더링 파이프라인에서 자동으로 해준다.

### 6.4.1 입력 배치 서술과 Input signature 연결

* 렌더링 파이프라인에 공급된 정점들이 정점 쉐이더가 기대하는 (Input assembly) 에서 받아올 모든 입력을 제공하지 못하면, 해당 쉐이더 프로그램은 실패하며 오류가 일어난다.
* 다만, **정점 쉐이더가 기대하는 것보다 더 많은 정보가 들어오면** 그것은 오류가 아니다. 따라서 다음 밑의 코드는 호환이 된다.

``` c++
struct Vertex { XMFLOAT3 POs; XMFLOAT4 Color; XMFLOAT3 Normal; };
D3D12_INPUT_ELEMENT_DESC desc[] = {
    {"POSITION", 0, DXGI_FORMAT_R32B32G32_FLOAT, /*! ... */},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, /*! ... */},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, /*! ... */}
};

/*! Vertex shader is below */
struct VertexIn {
    float3 PosL : POSITION; float4 Color : COLOR;
};
/*! ... */
```

* 또한, *HLSL* 은 **reinterpret_cast** 을 허용하고 있기 때문에, 자료 형식이 달라도 오류를 일으키지 않는다. 다만 경 디버그 환경에서 경고 메시지를 낸다.

## 6.5 Pixel Shader 의 예제

*Vertex shader* 혹은 *Geometry shader* 가 출력한 정점 특성들은, 래스터화 단계에서 삼각형의 픽셀을 따라서 보간이  된다. 그 보간된 값은 무수한 값이 되서 *Pixel shader* 에 입력된다.

* *Pixel shader* 역시 *Vertex shader* 와 같이 하나의 함수이지만, 차이점은 정점 단위가 아닌, 픽셀 (Fragment) 단편마다 실행이 되기 때문에 연산량이 많다. *Pixel shader* 의 임무는 **주어진 입력으로부터 픽셀 단편의 색상을 결정하는 것이다**.
  * 하지만, PS 도중에 `clip()` 등의 함수로 픽셀 쉐이더에서 절단되는 케이스도 존재한다. (*glsl* 에서도 지원한다) 그리고 **Depth test** 에 의해 깊이 값이 크면, 깊이 값이 작은 단편에 의해 렌더링되지 않는 경우도 존재한다. 또한 스텐실 역시 그렇다.
  * 따라서, 픽셀 자체가 렌더링이 되서 최종적으로 스크린 버퍼에 보여지지 않고, 스크린 버퍼에 특정 위치에 여러가지 픽셀 후보군이 존재할 수 있기 때문에, 실질적으로 *픽셀* 이 아닌, *픽셀 단편 (Fragment)* 라고 부르는 것이 좋을 것 같다.

---

하드웨어 최적화의 하나로써, 렌더링 파이프라인은 **특정 Fragment 을 Pixel shader 에 도달하기 전에 폐기할 수 있다.** 이것을 **Early Z(depth) rejection** 이라고 한다. 이 최적화 방법을 수행하려면, 파이프라인은 깊이 판정을 먼저 수행해서 후면 버퍼의 픽셀이 **명백하게** 가려질 경우에는 픽셀 쉐이더를 실행하지 않도록 한다.

이렇게 함으로써 무수한 픽셀 중 일부만 쉐이더를 통과하기 때문에 상당한 최적화 효능이 있다. 하지만, *Early Z rejection* 이 무시되는 경우가 있는데, 예를 들어서 픽셀 쉐이더가 픽셀의 깊이를 수정한다면, 픽셀 쉐이더가 깊이를 어떻게 변경할 지 모르기 때문에 전체 프래그먼트가 쉐이더를 통과하지 않으면 안된다.

---

``` c++
cbuffer cbPerObject : register(b0) { float4x4 gWorldViewProj; };
void VS(float3 iPos : POSITION, float4 iColor : COLOR,
        out float4 oPosH : SV_POSITION, out float4 oColor : COLOR) {
    oPosH = mul(floa4(iPos, 1.f), gWorldViewProj);
    oColor = iColor;
}

float4 PS(float4 posH : SV_POSITION, float4 color : COLOR) : SV_TARGET {
    return color;
}
```

* 픽셀 쉐이더의 입력 *semantic* 이 정점 혹은 지오메트리 쉐이더의 출력 *semantic* 과 일치해야 한다.
* 또한, 픽쉘 쉐이더의 경우에는 return 으로 값을 돌려주는데 이는 **float4** 여야 한다. 그리고 해당 값의 *semantic* 은 RTV 의 어느 한 버퍼 위치에 서술될 예정이기 때문에, `SV_TARGET` *semantic* 을 써야한다.

## 6.6 Constant buffer

### 6.6.1 상수 버퍼의 생성

#### A. Constant buffer

*Constant buffer* 는 쉐이더 프로그램에서 참조하는 **상수 자료**를 담는 GPU 자원 (ID3D12Resource) 의 한 예이다.

``` c++
cbuffer cbPerObject : register(b0) { float4x4 gWorldViewProj; };
```

HLSL 쉐이더 프로그램에서 볼 수 있는 이 코드는 cbPerObject 라는 ***`cbuffer`*** (constant buffer) 를 참조한다. 이 버퍼는 $ 4 x 4 $ 행렬 하나만 저장하는데, *local space* 을 *clip space* 로 변환하는 역할을 한다.

* 정점 버퍼, 색인 버퍼와는 다르게 *Constant buffer* 는 **CPU 가 프레임 당 한번만 갱신하는 것이 일반적**이다. 예를 들어서, 카메라가 매 프레임 작동한다면 프레임마다 상수 버퍼를 새 시야로 갈아끼워야 한다.

  * 따라서 상수 버퍼는 *기본 힙* 이 아니라, ***업로드 힙 (Upload heap)*** 에 만들어야 한다. 그래야 CPU 가 버퍼의 내용을 갱신할 수 있기 때문이다. 

    > DEFAULT, UPLOAD 등에 관한건 다음 주소를 참고한다.
    > https://msdn.microsoft.com/en-us/library/windows/desktop/dn770374(v=vs.85).aspx

* **상수 버퍼를 만들 때는 크기가 반드시 최소 하드웨어 (256Bytes) 할당 크기의 배수**여야 한다. 

또한, 같은 종류의 상수 버퍼를 $ N $ 개 만들어서 써야할 경우에는 다음과 같이 ID3D12Resource 에 한꺼번에 $ N $ 개의 요소를 담을 수 있도록 할 수 있다.

``` c++
struct ObjectConstants { DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Idntity4x4(); };
/*! Return a multiply of 256 bytes more than ObjectConstants size bytes */
UINT elementByteSize = d3dUtil::CalcConstantBufferByte(sizeof(ObjectConstants));
ComPtr<ID3D12Resource> mUploadCBuffer {nullptr};
d3dDevice->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * NumElements),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&mUploadCBuffer)
);
```

그리고 나서 Command list 등으로 상수 버퍼 뷰를 파이프라인에 묶어야 한다.

* **`ID3D12Device::CreateConstantBufferView`**

  ``` c++
  void CreateConstantBufferView(
    [in, optional] const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
    [in]                 D3D12_CPU_DESCRIPTOR_HANDLE     DestDescriptor
  );
  ```

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788659(v=vs.85).aspx

#### B. Shader model 5.1

> https://msdn.microsoft.com/en-us/library/windows/desktop/dn933277(v=vs.85).aspx

### 6.6.2 Constant buffer 의 갱신

앞에서 상수 버퍼를 `D3D12_HEAP_TYPE_UPLOAD` 힙에 생성했기 때문에, CPU 에서 상수 버퍼 (Constant buffer) 자원을 자료에 올릴 수 있다. 근데 자료를 올리려면, **자원 자료를 가리키는 포인터를 얻어야 한다.** (*OpenGL 에서의 glMapBuffer?*)

다음과 같이 `ID3D12Resource::Map` 을 써서 자원 자료의 위치 포인터를 가져올 수 있다.

``` c++
ComPtr<ID3D12Resource> mUploadBuffer;
BYTE* mMappedData{ nullptr };
mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
/*! Copy... */
memcpy(mMappedData, &data, dataSizeInBytes);
/*! If all processing are end, must Unmap */
if (mUploadBuffer) 
    mUploadBuffer->Unmap(0, nullptr);
mMappedData = nullptr;
```

* **`ID3D12Resource::Map`**

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788712%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  HRESULT Map(
                          UINT        Subresource,
    [in, optional]  const D3D12_RANGE *pReadRange,
    [out, optional]       void        **ppData
  );
  ```

  1. `Subresource` CPU 메모리에 대응 시키려는 부분 자원의 색인을 말한다.
  2. **`pReadRange`** 대응시킬 메모리의 범위를 `D3D12_RANGE` 구조체의 포인터로 입력한다. 만약에 현재 상수 버퍼의 자원 전체를 범위로 하고 싶으면 `nullptr` 로도 충분하다.
  3. **`ppData`** 버퍼 자원에 대응될 데이터의 포인터를 가리킨다. 이것이 일종의 대행자 역할을 한다.

* **`ID3D12Resource::Unmap`**

  ``` c++
  void Unmap(
                         UINT        Subresource,
    [in, optional] const D3D12_RANGE *pWrittenRange
  );
  ```

  Unmap 함수는 보기와는 다르게 시작 위치와 위치로부터의 범위를 D3D12_RANGE 로 지정해서 해당되는 부분만 할당해제 하게 할 수도 있다. 하지만 그렇지 않고 전체를 해제할 경우에는 0, nullptr 을 입력한다.

