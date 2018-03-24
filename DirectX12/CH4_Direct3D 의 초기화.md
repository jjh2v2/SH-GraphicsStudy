# 4 :: Direct3D 의 초기화

## 목표

1. Direct3D 의 역할을 개괄적으로 이해하기
2. ***COM*** 의 역할을 이해하기
3. 기초 그래피 개념을 DirectX 관점에서 배우기
4. **성능 카운터 함수**를 사용해서 고해상도 타이머 얻ㄱ
5. Direct3D 초기화하는 방법을 배우기
6. **전반적인 구조를 숙지하기**

## 4.1 기본 지식 

### 4.1.1 Direct3D 12란?

* *Direct3D* 는 *OpenGL* 과 마찬가지로 응용 프로그램에서 GPU 를 제어하고 프로그래밍 하는데 쓰이는 **저수준 그래픽 API** 이다. 이를 사용해서 응용 프로그램은 3차원 공간을 렌더링할 수 있게 된다.
  * 예를 들어서, 렌더 대상을 **지우는** 명령을 그래픽 하드웨어에 **요청**하기 위해서는, 
    Direct3D 의 경우 `ID3D12CommandList::ClearRenderTargetView()` 라는 메서드를 콜하면 된다.
    * *"지운다"* 라는 것은, 배열, 버퍼 등 여러 개의 원소로 이루어진 컨테이너를 *특정한 하나의 값* 으로 초기화하는 것을 뜻한다.
  * 응용 프로그램 어플리케이션 레이어와 그래픽 하드웨어 하드웨어 계층 사이에 라이브러리 계층으로 *Direct3D* 라는 간접층이 끼어들어, API를 사용해 특정 기능을 수행하도록 요청하면 *DIrect3D* 는 이를 하드웨어가 알아들을 수 있도록 번역해준다. 
* *Direct 3D 12* 는 기존의 *11* 보다 CPU 부담을 크게 줄이고 다중 스레드 지원을 하기 위해서 설계를 완전히 다시 했다는 점에서, 추상화가 작은 API 가 되었다. 따라서 저수준 레이어의 지식이 풍부하게 필요하며, 현세대 GPU 구조를 좀 더 밀접하게 알고 있어야 보다 효율적인 렌더링이 가능하다.

### 4.1.2 COM (Component Object Model)

#### A. Component Object Model & WRL

> https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms680573(v=vs.85).aspx

* ***COM*** 은 DirectX 의 프로그래밍 언어 독립성 (말 그대로 언어에 독립적) 과 하위 호환성 (구 버전에서도 가능하게 하는) 을 가능하게 하는 기술이다. **COM 객체**는 대개 ***COM Interface*** 라고 부른다.

  * *COM* 을 이용해서 프로그래밍을 할 때, 프로그래머가 대체적으로 알아야 할 것은 구조도 구조지만 COM 인터페이스를 가리키는 포인터를 특별한 함수를 사용해서 얻는 것이다. C API 혹은 구시대의 C++ 의 `new` 및 `delete` 연산자 없이, 그것을 대신 해주는 메서드를 호출한다.
    * 예를 들면, delete 대신에 `Release()` 라는 멤버 함수가 존재한다.
  * *COM* 인터페이스는 참조 횟수를 기록하고 있어 $ 0 $ 이 되면 자동으로 해제된다. (`shared_ptr` 과 비슷할려면 비슷할 것 같다) 이런 *COM 인터페이스*의 수명 관리를 위해서 ***Windows Runtime Library (WRL)*** 는 ***`Microsoft::WRL::ComPtr`*** 이라는 클래스를 제공한다.
    * 사용할려면 `<wrl.h>` 가 필요하다.
    * 또한 `Comptr` 객체는 자동으로 범위를 벗어나면 소멸자가 호출되면서 실제 인터페이스에 `.Release()` 을 호출하기 때문에 프로그래머가 이 소멸 함수를 호출할 필요가 없다.

* *`Windows::WRL::ComPtr`* 의 여러 멤버 함수 중, 이 책에서 사용하는 멤버 함수는 다음과 같다고 한다.

  1. ***`.Get()`***
     바탕이 되는 (underlying) *COM Interface* 의 포인터를 돌려준다. 해당 *COM Interface* 의 포인터 형식의 인수를 받는 함수를 호출할 때 흔히 쓰인다.

  ``` c++
  ComPtr<ID3D12RootSignature> m_root_signature;
  /*! ... */
  m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
  ```

  2. ***`.GetAddressOf()`***
     가지고 있는 *COM Interface* 의 포인터의 **주소**를 돌려준다. 함수 패러미터를 통해 *COM Interface* 의 포인터를 돌려 받을 때 쓰인다.

  ``` c++
  ComPtr<ID3D12CommandAllocator> m_direct_cmd_list_alloc;
  /*! ... */
  ThrowIfFailed(m_d3d_device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, m_direct_cmd_list_alloc.GetAddressOf()
  ));
  ```

  3. ***`.Reset()`*** 
     ComPtr 인터페이스를 *nullptr* 로 설저하고, 바탕이 되는 *COM Interface* 의 참조 횟수를 $ -1 $ 한다. 또는 *ComPtr* 객체에 *nullptr* 을 설정해도 같은 메커니즘을 지닌다.

> ***COM Interface*** 의 이름은 *prefix* `I` 로 시작한다.

### 4.1.3 텍스쳐 형식

* 텍스쳐는 단순한 자료 배열인 것만은 아니다. 텍스쳐는 *OpenGL* 과 같이 단순히 이미지의 정보만을 저장하지 않고 여러 형태의 자료를 저장할 수 있다. 
  그렇지만, 텍스쳐는 *Direct 12* 가 지정한 ***특정 형식 (format)*** 의 자료 원소만을 담을 수 있다.
  * 특정 형식은 **`DXGI_FORMAT_`** 이라는 열거형으로 지정되어 있다.

> `DXGI_FORMAT_` 은 다음 주소에서 자세하게 볼 수 있다.
> https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx

* `DXGI_FORMAT_` 타입을 이용한 텍스쳐에서, ***Typeless texture*** 라는게 있는데 이런 텍스쳐는 메모리만 확보하고  구체적인 해석 방식을 나중에 텍스쳐를 *DirectX 의 Pipeline* 에 지정하는 용도로 쓰인다.
  * 예를 들면, `DXGI_FORMAT_R16G16B16A16_TYPELESS` 는 각 채널에 대해 2바이트를 사용하되,
    **각 16비트의 구체적인 자료 형식 (정수인가, 부동소수점, 등) 은 지정하지 않는다.**
  * 그리고 `DXGI_FORMAT_` 타입은 정점 (*Vertex*) 자료 형식과 색인 (*Index*) 자료 형식을 할당할 때도 쓰인다.

### 4.1.4 Swap chain & Presenting

단일 프레임 버퍼를 사용해서 렌더링을 하게 되면, 렌더링 도중에 테어링 (혹은 지터링) 현상이 일어날 수 있다. 이를 막기 위해서, 즉 애니메이션의 깜빡이는 현상을 피하기 위해 **Double Buffering** 이라는 방법을 사용할 수 있다. 이렇게 하면 화면을 보는 사용자에게는 프레임이 그려지는 현상이 일어나지 않게 된다.

위 *Double buffering* 이라는 기법에서는 전면 버퍼와 후면 버퍼가 있는데 이 버퍼를 교환해서 페이지가 전환하게 하는 것을 *Direct3D* 에서는 ***Presenting*** 이라고 한다. 이 *Presenting* 은 아주 효율적이다. 버퍼의 내용이 아닌, 버퍼를 가리키는 포인트의 주소 값을 바꾸면 되는 것이기 때문이다. 

*Double buffering* 은 이런 *Presenting* 외에도 ***Swap chain***을 형성한다. *Direct3D* 에서 교환 사슬을 대표하는 COM 인터페이스는 I***DXGISwapChain*** 이다. 

> *IDXGISwapChain* 에 대해서 자세히 알려면 다음 주소를 참고하라.
> https://msdn.microsoft.com/ko-kr/library/windows/desktop/bb174569(v=vs.85)

### 4.1.5 Depth buffering

* ***Depth buffer*** 는 이미지 자료를 담고 있지는 않지만, 각 픽셀의 $ z $ 축등의 깊이 정보를 담는다. 이 깊이 정보는 $ [0, 1] $ 의 값을 가지며 클립 영역의 좌표를 가진다. 

  * $ 0 $ 은 ***view frustum*** 에서의 관찰자에 가까운 물체에 해당하고, $ 1 $ 은 *view frustum* 안에서 관찰자와 최대한 먼 물체에 해당된다. 그리고 깊이 버퍼의 원소와 후면 버퍼의 픽셀은 **일대일 대응**이 된다.
  * *Direct3D* 는 이런 *Depth buffer* 을 활용하기 위해 ***z-buffering*** 이라고 하는 기법을 사용한다. 일반 방법으로는 $ z $ 값이 가장 작은 ($0$ 에 근접한) 픽셀을 프레임 버퍼의 컬러 버퍼와 깊이 버퍼에 갱신해서 최종적으로 카메라에 보이는 것만 렌더링을 하게끔 하는 방법이다. 

* *Depth buffer* 역시 하나의 텍스쳐 (위에서 말했듯이 텍스쳐는 여러 용도로 사용될 수 있다고 했다) 이므로, 별도로 생성을 할 때, 특정한 자료 원소 형식을 지정할 필요가 있다. 깊이 버퍼링을 위한 텍스쳐 자료 원소 형식으로는 다음과 같은 것이 있다.

  1. ***`DXGI_FORMAT_D32_FLOAT_S8X24_UNIT`***
     각 텍셀은 32비트 부동소수점 깊이 값과, $ [0, 255] $ 구간으로 맵핑되는 부호가 없는 $ 8 $ 비트 양수 정수 스텐실 값, 및 패딩용 24 비트로 구성된다.
  2. ***`DXGI_FORMAT_D32_FLOAT`***
     각 텍셀은 하나의 32비트 부동소수점 깊이값 만을 갖는다.
  3. ***`DXGI_FORMAT_D24_UNORM_S8_UNIT`*** 
     각 텍셀은 $ [0, 1] $ 구간의 부호 없는 24비트 부동소수점 깊이와, $ 8 $ 비트 정수 스텐실로 구성이 된다.
  4. ***`DXGI_FORMAT_D16_UNORM`***
     각 텍셀은 부호 없는 16비트 부동소수점 깊이 값으로 구성된다

  * 만약에 **Stencil buffer** 을 사용하고자 하면 깊이 버퍼에 추가할 수 있다. 위 형식은 그것을 지원하고자 한다.

### 4.1.6 Resource & Descriptor

#### A. Descriptor

렌더링 과정에서, *GPU* 는 자원에게 값을 기록하거나 값을 읽어들일 수 있다. 이런 값에 접근할 수 있는 것은 버퍼 혹은 *OpenGL* 에서 말하던 *Uniform 변수*와 같은 것들이다. 아무튼 그리기 명령을 제출하기 전에 해당 그리기 호출이 참조할 자원들을 **현재 쉐이더 렌더링 파이프라인에 묶어야** 된다. 이를 ***Binding*** 이라고 한다. 

* 그런데 *GPU* 자원이 *Pipeline* 에 직접 묶이지는 않는다. **실제로 파이프라인에 묶이는 것은, 해당 자원을 참조하는** ***Descriptor*** 객체이다. *Descripor object* 는 **바인딩된 자원을 GPU 에 실제로 서술하는 경량의 자료구조** 라고 할 수 있다. 즉, *OpenGL* 의 바인딩 포인트 처럼 추상층이 하나 생긴 것이다.

  * 따라서, *GPU* 는 *Descriptor* 을 통해서 자원의 실제 자료에 접근하고자 한다. 그리고 자료를 사용하는데 필요한 정보 역시 자원 서술자로부터 얻는다. 쉐이더에서 참조할 서술자들을 명시하면, 해당 서술자가 쉐이더에 쓰일 파이프라인에 묶이게 된다.

* *Descriptor* 을 둬서 간접층을 만드는 이유는 *Descriptor* 에 묶인 데이터는 여러 군데에서 사용할 수 있으며 또한 동일한 렌더링 파이프라인의 여러 단계에서 독립적으로 참조되어 사용될 수 있기 때문이다.

  * 흔한 예로 텍스쳐를 렌더링 대상으로도 사용하고 셰이더에서 입력을 하는 자료로도 활용될 수 있다. 또한 자원 자체는 자신이 어떤 형식이어야 하는지 어떻게 사용되어야 하는지 모른다.
    그래서 자원만으로는 *GPU* 가 이 자원을 어떻게 다뤄야 하는지 알 수 없다.
  * *Desctiptor* 는 **자원 자료의 형식을 지정**할 뿐만 아니라 자원을 **GPU에 서술**한다. 서술자는 *Direct3D* 에게 자원의 **사용법**을 말해준다. 그리고 자원 전체가 아닌 자원의 부분 영역을 서술자에 지정하게 할 수도 있다. 만약 *Typeless* 인 자원에 대해서는 서술자가 형식을 지정하게 할 수도 있다.

* *Descriptor* 는 자원의 사용법에 따라 여러 가지 형식으로 나뉜다. 이 책에서는 다음을 쓴다.

  1. ***CBV / SRV / UAV***
     상수 버퍼 뷰 (Constant buffer view)
     쉐이더 자원 뷰 (Shader resource view)
     순서 없는 접근 뷰 (Unordered access view) 을 서술한다.
  2. ***Sampler***
     텍스쳐 또는 기타 리소스로부터 입력 값을 읽는 서술자이다.
  3. ***RTV*** (Rendering Target View)
     렌더링 대상을 사용하면 화면에 렌더링될 백 버퍼가 아닌 임시 중간 버퍼로 장면이 렌더링되게 할 수 있습니다. 이 기능을 사용하면 그래픽 파이프라인 내의 반사 텍스처 또는 다른 목적으로 렌더링될 수 있는 복잡한 장면을 사용하거나 렌더링하기 전에 화면에 추가 픽셀 셰이더 효과를 추가할 수 있습니다.
  4. ***DSV*** (Depth Stencil View)
     깊이 및 스텐실 정보를 보유하는 형식 및 버퍼를 서술한다. 렌더링 영역을 정의하는 것을 넘어 스텐실 버퍼를 더 고급 방식으로 사용할 수 있습니다. 스텐실 버퍼 값은 페이드, 실루엣, 디스케일링, 디졸브, 아웃라인, 그림자 볼륨 등에 등의 효과를 위해 조작할 수 있습니다.

  > 자세한 것은 밑의 주소를 참고하라.
  > https://docs.microsoft.com/ko-kr/windows/uwp/graphics-concepts/views
  > https://msdn.microsoft.com/en-us/library/windows/desktop/mt709127(v=vs.85).aspx


#### B. Descriptor Heap

> https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn899110(v=vs.85).aspx

* *Descriptor* 들을 **저장하는 메모리 공간이며** 각각의 *Descriptor* 의 종류마다 같은 종류의 *Descriptor Heap* 에 저장된다. 또한 한 종류의 *Descriptor* 에 대해 여러 개의 힙을 둘 수도 있다.
* 하나의 자원을 렌더링 파이프라인의 여러 단계에 묶을 수 있는데, **단계마다 개별적인 서술자가 필요하다**.
  * 예를 들어서, 하나의 텍스쳐를 렌더 대상이자 쉐이더 자원을 사용하고자 한다면 두 개의 서술자가 필요하다. *RTV* 와 *SRV* 형식을 만들어야 한다. 
  * *Typeless* 로 자원을 만들었을 경우에는 텍스쳐의 원소를 부동소수점 및 정수형으로 사용하고자 한다면 각각의 서술자를 만들어야 한다. 또한 *Typeless (무형식)* 과는 다르게 자원의 형식을 완전히 미리 할당하고 힙에 붙여 쓰게 되면, 최적화가 일어나 보다 좋은 효율을 기대할 수 있다.
  * *서술자(Descriptor)* 들은 응용 프로그램의 초기화 시점에서 생성하는 것이 좋다. 또는 각각의 초기화 시점 단계에서 생성하는 것이 좋은데, 왜냐면 유효성 검증이 일어나기 때문이기도 하다.

### 4.1.7 Multisampling (다중 표본화)

텍스쳐의 해상도, 모니터의 해상도와 같은 한계로 정점에서 정점까지의 임의의 선을 완벽하게 나타내는 것은 불가능하다. 예를 들어서, 렌더링을 했을 때 임의의 선은 일련의 픽셀로 그어져 도중 도중에 두드러지는 현상을 보인다. 이를 ***Aliasing*** 이라고 한다.

모니터 해상도를 키우는 방법으로 해결하기도 할 수 있지만, 그렇지 못할 경우에는 ***Anti-aliasing*** 이라는 기법을 적용할 수 있다. 그런 기법의 하나가 ***Supersampling (SSAA)*** 이다.

* ***SSAA*** 에서는 후면 버퍼와 깊이 버퍼를 대개 화면 해상도보다 $ 2 $ 배씩 크게 잡아서 (메모리를 할당해서) 3차원 장면을 $ 4 $ 배의 크기의 해상도에서 렌더링을 한다. 그리고 나서 이 후면 버퍼에 그려진 값을 *Presenting* 해야할 때가 오면 버퍼를 원래 크기로 ***Resolving*** 한다.

  * ***Downsampling*** 이라고도 불리는 이 기법은 큰 해상도의 **4 픽셀 블록의 네 색상의 평균**을 골라 환원된 버퍼의 임의 픽셀의 최종 색상으로 정한다.
  * 따라서 이 기법은 메모리 사용을 기존 해상도보다 $ 4$ 배 사용한다는 점에서 비용이 높다.

* 대안으로 제시된 ***MSAA (Multisampling anti-aliasing)*** 기법은 일부 계산 결과를 *subpixel* 사이에서 공유하기 때문에 *SSAA* 보다 비용이 낮다.

  * x4 의 경우에는 *MSAA* 역시 화면 해상도가 $ 4$ 배인 후면 버퍼와 깊이 버퍼를 사용한다. 하지만, *MSAA* 는 이미지 색상을 SSAA 처럼이 아니라 **픽셀의 중심에서 한 번만 계산하고 그 픽셀과 부분 픽셀의 깊이, 스텐실 판정 및 다각형이 부분픽셀을 얼마나 덮고 있는가에 따라** 최종 색상이 결정된다.

  > 그림 4.5 에서 볼 수 있다.

### 4.1.8 Direct3D 의 MSAA

#### A. `DXGI_SAMPLE_DESC`

> https://msdn.microsoft.com/en-us/library/windows/desktop/bb173072(v=vs.85).aspx

*MSAA* 을 위해서는 ***`DXGI_SAMPLE_DESC`*** 라는 인스턴스를 값으로 적절히 채워야 한다. 이 구조체는 다음과 같은 형식을 띈다.

``` c++
typedef struct DXGI_SAMPLE_DESC {
  UINT Count;
  UINT Quality;
} DXGI_SAMPLE_DESC;
```

* ***`Count`*** 
  MSAA 에 필요한 픽셀 당 추출할 표본의 개수를 말한다.
* ***`Quality`***
  원하는 품질 수준을 설정한다. 이 값은 $[0, 1]$ 사이의 값을 가지며, 하드웨어 사양에 따라서 $ 1$ 보다 더 낮은 값이 한계치가 될 수도 있다.
  * 한계치 값을 알려면 다음과 같은 함수를 써서 맥스 값을 호출할 수 있다.
    [**ID3D10Device::CheckMultisampleQualityLevels**](https://msdn.microsoft.com/en-us/library/windows/desktop/bb173537(v=vs.85).aspx) for Direct3D 10 or [**ID3D11Device::CheckMultisampleQualityLevels**](https://msdn.microsoft.com/en-us/library/windows/desktop/ff476499(v=vs.85).aspx) for Direct3D 11.
  * 기본 MSAA 가 활성화되지 않은 시점에선 `Count` 는 $ 1 $, `Quality` 는 $ 0 $ 의 값을 가진다.

#### B. `ID3D12Device::CheckFeatureSupport`

* 주어진 텍스쳐 형식 및 표본 개수 조합에 따른 품질 수준의 개수는, 현재 그래픽 드라이버에서 지원하는 기능들의 정보를 얻고자 할 때 쓰는 ***`ID3D12Device::CheckFeatureSupport`*** 을 쓴다.

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788653(v=vs.85).aspx

  * 해당 함수의 인자는 다음과 같다.

    ``` c++
    HRESULT CheckFeatureSupport(
                D3D12_FEATURE Feature,
      [in, out] void          *pFeatureSupportData,
                UINT          FeatureSupportDataSize
    );
    ```

이를 사용해서 다음과 같이 멀티 샘플링이 가능한가를 알아볼 수 있다.

``` c++
typedef struct D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS {
    DXGI_FORMAT Format;
    UINT SampleCount;
    D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG Flags;
    UINT NumQualityLevels;
} D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS;
/*! Code... */
D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
msQualityLevels.Format = mBackBufferFormat;
msQualityLevels.SampleCount = 4;
msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
msQualityLevels.NumQUalityLevels = 0;
ThrowIfFailed(md3dDevice->CheckFeatureSupport(
	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
    &msQualityLevels,
    sizeof(msQualityLevels)
));
```

위 메서드는 둘째 매개변수가 입력과 출력으로 쓰이는데, 넘겨준 구조체에서 *텍스쳐 형식* 과 *표본 개수*를 읽어서 해당하는 적절한 품질 수준 개수를 구조체의 `NumQualityLevels` 에 설정한다. 

> `DXGI_SAMPLE_DESC` 구조체는 *Swap chaning* 및 *Depth buffer* 에 모두 필요하다. 그리고 후면 버퍼 및 깊이 버퍼를 생성할 때, **동일한 MSAA** 설정을 적용해야 한다.

### 4.1.9 Feature Level

> https://msdn.microsoft.com/en-us/library/windows/desktop/ff476876%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
>
> https://msdn.microsoft.com/en-us/library/windows/desktop/ff476329(v=vs.85).aspx

* ***Feature level*** 이라는 것은 *Direct3D 11* 에서 도입된 기능으로, 코드에서는 `D3D_FEATURE_LEVEL_` 의 형태의 열거형으로 제공되고 있다. *Feature Level* 은 여러 *GPU* 가 지원하는 기능들의 엄격한 집합을 정의한다.

  * 예를 들면 *Feature level* 11을 지원하는 *GPU* 는 반드시 *Direct3D 11* 의 기능 전체를 지원해야 한다.
    다만 지원하는 것들 중, 사양으로 각 *GPU* 가 지원하는 정도를 확인해야 할 때도 있다. (MSAA 의 표본 개수 등)

  따라서, feature level 만 알면 구체적으로 어떤 기능을 사용하면 되고 어떤 기능을 사용해서는 안되는 가를 알 수 있다. 그래서 사용자의 하드웨어가 특정 기능 수준을 지원하지 않을 때는 **응용 프로그램이 실행을 포기하는 대신에 낮은 기능 수준으로 후퇴**하는 전략을 쓰게 할 수도 있다.

### 4.1.10 DXGI (DirectX Graphics Infrastructure)

> DXGI Overview
> https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
>
> DXGI 1.6 Improvements
> https://msdn.microsoft.com/en-us/library/windows/desktop/mt845648(v=vs.85).aspx

* ***DXGI*** 는 *Direct3D* 와 함께 쓰이는 API 이다. 이 API 가 만들어진 기본 착안은, 여러 그래픽 API 들에 공통인 그래픽 관련 작업이 존재한다는 점이다. 예를 들어서 2차원 애니메이션을 2차원 렌더링 API 로 할 경우에도 3차원 렌더링 API 처럼 *Swap chaining* 이 필요하다. 따라서, 교환 사슬의 역할을 맡은 인터페이스인 `IDXGISwapChain` 은 *Direct3D* 가 아닌 *DXGI* 의 일부이다.
  * ***DXGI*** 는 이외에도 해상도, 어댑터, 모니터, 그래픽 시스템의 열거 및 모든 렌더링에 있어 공통된 기능을 API 로 제공한다.

### 4.1.11 Checking Feature support

* ***`ID3D12Device::CheckFeatureSupport`*** 는 위에서도 봤지만 여러가지 기능을 점검하도록 도와준다. 이 함수의 인자는 다음과 같다.

``` c++
HRESULT CheckFeatureSupport(
            D3D12_FEATURE Feature,
  [in, out] void          *pFeatureSupportData,
            UINT          FeatureSupportDataSize
);
```

1. **`Feature`** 
   이 매개변수는 지원 여부를 **점검할 기능의 종류**를 입력한다. 해당 인자는 `D3D12_FEATURE` 열거형을 받는데, 해당 열거형의 자세한 정보는 밑의 주소를 참고한다.
   https://msdn.microsoft.com/en-us/library/windows/desktop/dn770363%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396


2. **`pFeatrueSupportData`** 
   Feature support 정보가 설정될 구조체를 가리키는 포인터이다. 구조체의 구체적인 형식은 *Feature* 매개변수에 지정한 값에 따라 다르다.
3. **`FeatureSupportDataSize`** 매개변수로 전달한 구조체의 크기

다음은 그래픽 하드웨어가 지원하는 기능 수준들을 점검하는 방법을 보여주는 코드이다.

``` c++
typedef struct D3D12_FEATURE_DATA_FEATURE_LEVELS {
    UINT NumFeatureLevels;
    const D3D_FEATURE_LEVEL* pFeatureLevelsRequested;
    D3D_FEATURE_LEVEL MaxSupportedFeatureLevel;
} D3D12_FEATURE_DATA_FEATURE_LEVELS;
/*! Code */
D3D_FEATURE_LEVEL feature_levels[3] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3
};
D3D12_FEATURE_DATA_FEATURE_LEVELS feature_levels_info;
feature_levels_info.NumFeatureLevels = 3;
feature_levels_info.pFeatureLevelsRequested = feature_levels;
md3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_levels_info,
                               sizeof(feature_levels_info));
```

### 4.1.12 Residency (상주성)

> Residency
> https://msdn.microsoft.com/ko-kr/library/windows/desktop/mt186622(v=vs.85).aspx
>
> Memory Management in Direct3D 12
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn899198%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

* *Direct3D 12* 이 지원되는 플랫폼에서 어플리케이션은 텍스쳐 등과 같은 자원을 *VRAM* 에서 내리고, 필요하면 다시 올림으로써 ***Residency*** 을 관리한다. 이렇게 함으로써, *VRAM* 이 사용하는 메모리의 양을 최소화시켜서 보다 원활한 성능을 유지하고자 함에 있다.
  * 하지만 이 내렸다 올렸다하는 것 역시 **오버헤드**가 있기 때문에 짧은 시간에 *Residency* 을 바꾸는 등의 행동은 해서는 안된다. **이상적**으로는, 한동안 사용하지 않을 자원들만 **VRAM** 에서 내려야 한다.
* **기본적으로는** 자원을 생성하면 해당 자원이 자동으로 VRAM 에 들어가며, 파괴될 때 VRAM 에서 빠져 나온다. 하지만 다음 메서드를 사용해서 *Application* 이 상주성을 직접 제어하게 할 수 있다.

## 4.2 CPU 와 GPU 의 상호작용

그래픽 프로그래밍에서는 두 가지 처리 장치, CPU 와 GPU 가 작동한다는 점을 이해해야 한다. 이 둘은 병렬로 작동하지만 때로는 어토믹 연산 등을 위해서 동기화가 필요한 때가 있다. 하지만 최대한의 성능을 내기 위해서는 동기화는 되도록이면 해서는 안되며 병렬성을 망쳐서는 안된다.

### 4.2.1 Command queue & Command list

*GPU* 에는 ***Command queue*** 가 하나 있다. *CPU* 는 그리기 명령 목록 (***Command list***) 을 *Direct3D API* 을 통해 해당 *GPU* 의 대기열에 제출하게 된다. 이 때 큐에 아직 남은 명령들이 있으면 제출된 해당 명령 리스트들은 바로 실행되지 않는다. 명령 대기열이 비면, *GPU* 는 할 일이 없으므로 그냥 놀 게 되지만, 반대로 대기열이 꽉 차면 대기열에 자리가 생길 때 까지 *CPU* 는 명령 리스트를 보류하고 놀 게 된다.

따라서 가장 이상적인 상황은, CPU 도 놀지 않으며 GPU 도 놀지 않도록 하는 것이다. 

#### A. Implementation

*Direct 3D* 에서 Command queue 을 대표하는 인터페이스는 ***ID3D12CommandQueue*** 이다. 이 인터페이스를 생성하려면 대기열을 서술 (Descript) 하는 ***`D3D12_COMMAND_QUEUE_DESC`*** 구조체를 채운 후에, ***`ID3D12Device::CreateCommandQueue`*** 을 호출해야 한다.

다음과 같이 명령 대기열을 채울 수 있다.

``` c++
Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
```

이 코드에 쓰인 보조 매크로, `IID_PPV_ARGS` 의 정의는 다음과 같다.

``` c++
#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
```

1. `__uuidof(**(ppType))` 은 `**(ppType)` 의 *COM Interface* 의 ID 로 평가된다. 여기서는 `mCommandQueue` 의 ID 를 반환한다. 그리고 `IID_PPV_ARGS_Helper` 는 `ppType` 을 `void**` 로 캐스팅한다. 이 매크로들은 이 책 전반에 쓰인다.

* ***`void ID3D12CommandQueue::ExecuteCommandLists(UINT Count, ID3D12CommandList_p const_p ppCommandList)`***
  **`ID3D12CommandQueue`** 인터페이스의 주요 메서드 중 하나로, 명령 목록에 있는 명령들을 **VRAM 대기열에 추가**하는 명령이다.
  * 명령 목록을 대표하는 COM 인터페이스는 *ID3D12CommandList* 이다.
    하지만 **실제 그래픽 작업을 위한 명령 목록**은 이 인터페이스를 **상속**하는 ***ID3D12GraphicsCommandList*** 이다. 이 인터페이스에서는 명령들을 명령 목록에 추가하는 여러 메서드가 존재한다.

다음 코드는 ***Viewport*** 을 설정하고, 렌더 대상 뷰를 가져오고, 그리기 호출을 실행하는 명령을 추가하는 코드이다.

``` c++
// mCommandList is pointer of ID3D12CommandList
mCommandList->RSSetViewports(1, &mScreenViewport);
mCommandList->ClearRenderTargetView(mBackBufferView, Colors::LightSteelBlue, 0, nullptr);
mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
```

> **ID3D12CommandList** 의 자세한 설명은 여기서 참고한다.
> https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn770465(v=vs.85).aspx?f=255&MSPPError=-2147217396
>
> **ID3D12GraphicsCommandList** 의 자세한 설명은 여기서 참고한다.
> https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn903537(v=vs.85).aspx

명령을 추가했다고 해서 이 명령들이 바로 처리되는 것은 아니다. 나중에 ***`ExecuteCommandLists`*** 를 호출해야 비로소 명령들이 ***대기열에 추가***되며, *GPU* 가 여유시간에 해당 명령들을 뽑아서 처리한다. 명령들을 *Command List* 에 추가했으면 다음 메서드를 호출한다.

* ***`ID3D12GraphicsCommandList::Close()`***
  명령들의 기록이 끝났음을 *Direct3D* 에 알려준다. 이 명령은 `ExecuteCommandLists` 이전에 **반드시** 호출되어야 한다.

*Command list* 에는 ***`ID3D12CommandAllocator`*** 형식의 메모리 할당자가 하나 연관된다. *Command List* 의 모든 명령들은 이 할당자의 **메모리에 저장**된다. 따라서 `ExecuteCommandLists` 로 명령 목록을 실행하면, ***Queue*** 는 그 할당자에 담긴 명령을 참조한다.

*CommandAllocator* 은 다음과 같은 함수를 이용해서 생성한다.

* ```c++
  HRESULT ID3D12Device::CreateCommandAllocator(
    [in]  D3D12_COMMAND_LIST_TYPE type,
          REFIID                  riid,
    [out] void                    **ppCommandAllocator
  );
  ```

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788655(v=vs.85).aspx

  1. `type` 
     이 *Allocator* 와 연관시킬 수 있는 명령 목록의 종류 (**D3D12_COMMAND_LIST_TYPE**)
     자세한 사항은 다음 주소를 참고한다.
     https://msdn.microsoft.com/en-us/library/windows/desktop/dn770348(v=vs.85).aspx
     * 이 책에서는 두가지를 쓴다.
       `D3D12_COMMAND_LIST_TYPE_DIRECT`
       GPU가 직접 실행하는 명령 목록. (지금까지 실행한 명령 목록에 해당)
     * `D3D12_COMMAND_LIST_TYPE_BUNDLE`
       *Bundle* 을 나타내는 명령 목록.
       Command List 을 만드는 데는 *CPU* 의 부담이 어느정도 따르기 때문에, *Direct3D 12* 에서는 일련의 명령을 "묶음"으로 기록해서 **최적화할 수 있는 수단**을 제공한다. 번들을 추가하면, GPU 드라이버는 **렌더링 도중에 실행이 최적화**되도록 명령을 **전처리 (preprocessing)** 한다. 
       명령 묶음은 성능상의 이득이 있을 때에만 사용해야 하며, 무조건 써서는 안된다.
  2. `riid`
     생성하고자 하는 ID3D12CommandAllocator COM 인터페이스의 **ID**.
  3. `ppCommandAllocator`
     생성될 명령 할당자를 가리키는 포인터 (배열이기 때문에 `**` 이다)

명령 목록 (Command List) 역시 `ID3D12Device` 로 생성한다.

* ``` c++
  HRESULT ID3D12Device::CreateCommandList(
    [in]           UINT                    nodeMask,
    [in]           D3D12_COMMAND_LIST_TYPE type,
    [in]           ID3D12CommandAllocator  *pCommandAllocator,
    [in, optional] ID3D12PipelineState     *pInitialState,
                   REFIID                  riid,
    [out]          void                    **ppCommandList
  );
  ```

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788656%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  * `nodeMask`
    **For single GPU operation, set this to zero.** If there are multiple GPU 
    nodes, set a bit to identify the node (the  device's physical adapter) 
    for which to create the command list. Each bit in the mask corresponds to a single node. Only 1 bit must be set. Refer to [Multi-Adapter](https://msdn.microsoft.com/en-us/library/windows/desktop/dn933253(v=vs.85).aspx).

  * `type` 
    명령 목록의 종류로, `D3D12_COMMAND_` 의 열거형을 가진다.

  * `pCommandAllocator`
    생성된 *Command list* 에 연관시킬 할당자로, 할당자의 종류는 명령 목록의 종류와 **일치**해야 한다.

  * `pInitialState`
    명령 목록의 **초기 파이프라인 상태**를 지정한다. BUNDLE 및 실제 그리기 명령은 없는 list 에 대해서는 ***nullptr*** 을 지정해도 된다.

    > **ID3D12PipelineState** 는 밑의 주소에서 자세히 설명한다.
    > https://msdn.microsoft.com/en-us/library/windows/desktop/dn788705(v=vs.85).aspx

  * `riid`
    생성하고자 하는 *Command list* 에 해당되는 **ID3D12CommandList** COM 인터페이스의 ID

  * `ppCommandList` 
    생성된 명령 목록을 가리키는 포인터.

  > `ID3D12Device::GetNodeCount()` 을 통해서 현재 시스템의 GPU 어댑터 노드 개수를 알아낼 수 있다.

* 주의해야 할 점은, *Allocator* 을 여러 *Command list* 에 할당해도 되지만, *Command* 을 여러 *Command list* 에 동시에 기록할 수는 없다. 즉, **현재 명령을 추가하는 명령 목록을 제외한 모든 명령 목록은 닫혀있어야 한다.**

  * 이렇게 하지 안흥면 한 명령 목록의 모든 명령이 *Allocator* 안에 저장될 수가 없다.

* 또 주의해야할 점은, **명령 목록을 생성하거나 재설정하면, "열린"상태가 된다**.
  따라서 동일한 *Allocator* 로 두 명령 목록을 **연달아 생성**하면, 하나가 이미 열린 상태이기 때문에 오류가 발생한다.

* `ExecuteCommandList(Command list)` 을 호출한 후에, ***`ID3D12CommandList::Reset`*** 을 호출하면, 해당 *Command list* 의 내부 메모리를 **새로운 명령을 기록할 수 있게 된다.** (이전 명령은 VRAM 의 큐에 `mov`된다)
  해당 함수는 다음과 같다.

  ``` c++
  HRESULT Reset(
    [in]           ID3D12CommandAllocator *pAllocator,
    [in, optional] ID3D12PipelineState    *pInitialState
  );
  ```

  > https://msdn.microsoft.com/en-us/library/windows/desktop/dn903895(v=vs.85).aspx

  이 메서드는 주어진 명령 목록을 마치 처음 생성했을 때와 같은 상태로 만든다. (열린다) 그래서, 이 메서드를 사용해서 명령 목록을 힙 해제하고, 새롭게 명령 목록을 할당하는 번거로움 없이 **내부 메모리를 재사용**할 수 있도록 해준다. 명령 대기열이 참조하는 명령들은 *Allocator* 에 남아있기 때문에 영향이 없다.

* 이렇게 *Allocator* 에 *list* 을 넘긴 뒤에는, Allocator 가 명령을 GPU 에 넘긴 후에, 할당자의 메모리를 **다음 프레임을 위해서** 재사용해야한다. 이 경우

  ``` c++
  HRESULT ID3D12CommandAllocator::Reset(void);
  ```

  을 사용한다. 하지만 문제가 있는데, GPU 가 **명령 할당자에 담긴 모든 명령을 실행했음이 확실해지기 전 까지는, 명령 할당자를 Reset 하지 말아야 한다**. 그래서 확인 한 후에야 Reset 을 사용할 수 있다.

### 4.2.2 CPU/GPU 동기화

일반 시스템에서, CPU 와 GPU 는 독립성을 가지며 병렬로 실행된다. 따라서 여러 가지 동기화 문제가 발생할 수 있다. 만약 어떤 지오메트리 구조의 위치를 $ R $ 이라는 자원에 담고, 그 구조를 위치 $ p_1$ 에 그린다고 하면, *CPU* 는 $ p_1 $ 을 $ R $ 에 추가하고, $ R $ 을 참조하는 렌더 명령 $ C $ 을 *Command list* 에 추가할 것이다. 그리고 이것을 GPU 에 넘긴다.

문제는, *GPU* 가 렌더 명령 $ C $ 을 실행하기 전에, *CPU* 가 새 위치 $ p_2 $ 을 $ R $ 에 추가해서 $ p_1 $ 을 덮어씌우면 프로그래머가 의도하던 위치에 지오메트리가 그려지지 않는다.

* 위의 문제의 해결법은, **GPU** 가 명령 대기열의 명령들 중, **특정 지점까지의 명령을 다 처리할 때까지 CPU를 기다리게 하는 것** 이다. 이를 가리켜 **Flush** 라고 한다. 그리고 이를 위해서 필요한 것이 ***Fence (울타리)*** 이다. 

#### A. `ID3D12Fence`

> https://msdn.microsoft.com/en-us/library/windows/desktop/dn899188%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

* **CPU 와 GPU 의 동기화를 위한 수단** 으로 쓰인다.
  * 다음은 울타리 객체를 생성하는 메소드이다.

    ``` c++
    HRESULT ID3D12Device::CreateFence(
            UINT64            InitialValue,
            D3D12_FENCE_FLAGS Flags,
            REFIID            riid,
      [out] void              **ppFence
    );
    ```

    ``` c++
    ThrowIfFailed(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue++;
    ```

* 울타리 객체는 **UINT64** 값 하나를 관리한다. 이 값은 시간 상의 특정 울타리 지점을 식별하는 정수이다.
  해당 책의 예제에서는, 처음 울타리가 하나도 없는 상태에서는 이 값을 $ 0 $ 으로 두고, 새 울타리 지점을 만들 때마다 $ 1 $ 씩 증가시키는 방식으로 구현한다.

다음은 `ID3D12Fence` 을 사용해서 명령 대기열을 *동기화*로 비우는 방법을 보여준다.

``` c++
UINT64 mCurrentFence = 0;
void D3DApp::FlushCommandQueue() {
    // 현재 울타리 지점까지의 명령들을 표시하도록 값을 전진한다.
    mCurrentFence++;
    // 새 울타리 지점을 설정하는 명령을 대기열에 추가한다.
    // GPU 가 이 명령을 실행하기 전까지는, GPU 에서의 울타리 지점은 바뀌지 않는다.
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
    // GPU 가 울타리 지점의 명령을 완료할 때 까지 기다린다.
    if (mFence->GetCompletedValue() < mCurrentFence) {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, false, EVENT_ALL_ACCESS);
        // GPU 가 현재 울타리 지점 (동기화) 에 도달하면, 이벤트를 발동한다.
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        // GPU 가 현재 울타리 지점에 도달했음을 뜻하는 이벤트 발송할 때까지 기다린다.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}
```

사실 이는 이상적인 해결책은 아닌데, 왜냐면 *GPU* 의 작업이 끝날 때 까지 *CPU* 가 기다리지 않으면 안되기 때문이다. 사실 다른 방법이 있는데 이는 Chapter 7 에서 설명한다.

### 4.2.3 Resource status transition

쓰이는 렌더링 효과 중에서는 한 단계에서 *GPU* 가 자원 $ R $ 에 자료를 기록하고, 이후의 단계에서는 자원 $ R $ 의 자료를 읽어서 사용하는 식으로 구현이 되는 경우가 많은데, 이 때 문제는 파이프라이닝 등이 이루어지고 있기 때문에 *GPU* 가 이전 단계에서 자원에 자료를 다 기록하지 않았는데도 불구하고, 자원의 자료를 읽을려고 하면 문제가 발생한다. 이를 ***Resource Hazard*** 라고 한다.

* 위 *Resource harard* 문제를 해결하기 위해서 *Direct3D* 는 자원에 상태를 부여한다. 
  새로 생성된 자원은 ***기본 상태***로 시작한다. 그리고 텍스쳐 자원에 자료를 기록해야 할 때는, 그 텍스쳐 상태를 **Render target state**로 설정한다. 이후에 텍스쳐의 자료를 읽어야 한다면, 상태를 **Shader Resource State**로 변경한다. 이런 GPU에서의 응용 프로그램의 상태 전이를 *Direct3D* 에게도 보고할 수 있는데, 이렇게 함으로써 *Race condition* 을 막을 수 있다.
  * 다만 이 상태 전이의 보고는 전적으로 옵션이며 (성능때문에) 전혀 보고를 안해도 상관은 없다.
* 자원 상태의 변경은 ***Transition Resource Barrier*** 들의 배열을 설정해서 지정한다. 배열을 사용하기 때문에 API 호출을 한번 함으로써 여러개의 자원의 상태를 전이시킬 수 있다.

> Using Resource Barriers to Synchronize Resource States in Direct3D 12
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn899226%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
>
> D3D12_RESOURCE_BARRIER structure
> https://msdn.microsoft.com/en-us/library/windows/desktop/dn986740(v=vs.85).aspx
>
> CD3DX12_RESOURCE_BARRIER structure
> https://msdn.microsoft.com/en-us/library/windows/desktop/mt604704(v=vs.85).aspx

``` c++
typedef struct D3D12_RESOURCE_BARRIER {
  D3D12_RESOURCE_BARRIER_TYPE  Type;
  D3D12_RESOURCE_BARRIER_FLAGS Flags;
  union {
    D3D12_RESOURCE_TRANSITION_BARRIER Transition;
    D3D12_RESOURCE_ALIASING_BARRIER   Aliasing;
    D3D12_RESOURCE_UAV_BARRIER        UAV;
  };
} D3D12_RESOURCE_BARRIER;
```

> **CD3DX12_** 로 시작하는 확장 버전들은 모두 *d3dx12.h* 에 정의되어 있다. 이 헤더 파일은 공식 DirectX 12 SDK 의 일부가 아니라서, 따로 받아야한다.

이번 장에서 보게 될 (한참 멀었지만) 응용 프로그램은 다음과 같은 함수를 사용해서 이를 이용한다.

``` c++
mCommandList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(
                             CurrentBackBuffer(),
                             D3D12_RESOURCE_STATE_PRESENT,
                             D3D12_RESOURCE_STATE_REDNER_TARGET));
```

* ``` c++
  void ID3D12GraphicsCommandList::ResourceBarrier(
    [in]       UINT                   NumBarriers,
    [in] const D3D12_RESOURCE_BARRIER *pBarriers
  );
  ```

  Notifies the driver that it needs to synchronize multiple accesses to resources. 

  이 코드는 화면에 표시할 이미지를 나타내는 텍스쳐 자원을 **Pesentation state** 에서 **Render target state**로 변경한다. 즉, 자원 장벽이 명령 목록에 추가된다.

한마디로 말하면, **Transition Resource Barrier** 는 *GPU* 에게 자원의 상태가 전이됨을 알려주는 하나의 명령이라고 생각하면 좋다. 이를 통해서 *GPU* 는 해당 자원의 상태를 알게 되고, Resource Hazard 를 피할 수 있도록 한다.

> 자원 상태 전이에 대한 장벽 이외의 많은 장벽들이 존재한다.

### 4.2.4 Command list 을 활용한 다중 스레드의 활용

*Direct3D 12* 는 다중 스레드를 효율적으로 활용하게 되있어서, 물체가 많은 큰 장면을 다룰 때 장면 전체를 하나의 명령 리스트에 입력하지 말고, 여러 개의 리스트에 렌더링 명령을 넣어서 스레드를 $ 4$ 개로 돌리면, 전체적인 시간이 줄어들 것이다. (25% 로 줄어든다고는 장담을 못한다)

다음은 명령 목록 구축에 대해서 *Multithreading*을 할 때 주의해야 할 사항이다.

1. 명령 List은 *Free-threaded* 모델을 따르지 않는다. 즉 명령 데이터가 독립적이다.
2. 명령 Allocator 역시 위 모델을 따르지 않는다. 보통의 경우 여러 스레드가 같은 명령 Allocator 을 공유하지는 않는다.
3. 명령 Queue은 *Free threaded* 모델을 따른다. 즉, 여러 스레드가 같은 명령 대기열에 접근할 수 있다. 
4. 성능 상 이유로, 어플리케이션은 동시에 기록할 수 있는 명령 List 의 최대 개수를 반드시 초기화 시점에서 설정해야한다.

> 이 책에서는 멀티 스레딩을 제공하지는 않는다.