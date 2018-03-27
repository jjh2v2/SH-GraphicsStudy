# 5 :: Rendering Pipeline

3차원 장면의 Geometrical 한 Descriptor 와 가상 카메라 (뷰의 시점이 되는 것) 위치 및 방향이 주어졌을 때, 가상 카메라에 비친 3차원 공간을 2차원으로 옮겨 이미지를 생성하는 일련의 단계를 **Rendering Pipeline** 이라고 한다.  (화면 상에 보여주는 것은 렌더링 파이프라인 단계에 속하지는 않는다. 렌더링 후 화면에 보여주지 않고 다른 방향으로 쓸 때도 있다)

**목표**

1. 사람이 2차원 이미지에서 부피와 공간적 깊이감을 실제로 느끼게 만드는 핵심 단서를 살핀다.
2. *Direct3D* 에서의 3차원 물체의 조작법을 알아본다.
3. 가상 카메라를 정의하고, 조작하는 방법을 배운다.
4. *Direct3D* 에서의 *Rendering Pipeline* 을 이해한다.

## 5.2 모형의 표현

일반적으로 *Direct3D* 응용 프로그램에서는 고형의 3차원 물체를 **삼각형 메시(Mesh)** 로 Approximation 해서 표현할 수 있다. 삼각형 프리미티브는 가장 기초적인 도형이면서도 Convex하기 때문에, 거의 모든 사물을 유사하게 표현할 수 있다는 장점이 있다.

## 5.3 컴퓨터 색상의 기본 개념

### 5.3.2 128bit (32x32x32x32) 색상

$ (R, G, B, A) $ 각 성분을 32비트 단정도 부동소수점 값 하나로 표현한다면, 하나의 색상을 표현하는데 128 비트가 소요된다. 수학적으로 하나의 색상은 4차원 벡터이기 때문에, 코드에서도 그냥 `XMVECTOR` 을 사용해서 색상을 표현할 수 있다.

*DirectXMath* 라이브러리의 벡터 함수들로 색상 연산을 수행할 때는 SIMD 의 도움을 받아서 연산을 빨리 수행하게 할 수 있다. 성분별 곱셉은 *DirectXMath* 라이브러리의 다음 함수로 수행할 수 있다.

* ***`XMColorModulate`***

  > https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.color.xmcolormodulate%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

  ``` c++
  XMVECTOR XMColorModulate(
    [in] XMVECTOR C1,
    [in] XMVECTOR C2
  );
  ```

### 5.3.3 32비트 색상 (8x8x8x8)

$ (R, G, B, A) $ 각 성분을 8비트, 0 에서 255 단계까지의 정수형 비트로 표현할 수 있다. *DirectXMath* 라이브러리는 32비트 색상의 저장을 위해서 다음과 같은 구조체를 사용할 수 있다.

다만 주의해야 할 점은, **`DirectXPackedVector.h`** 헤더가 필요하다.

* ***`XMCOLOR`***

  > https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.xmcolor(v=vs.85).aspx

  여기서 알아둬야 할 것은 `XMCOLOR` 는 union 으로 R, G, B, A 따로 저장된 형식을 따를지 아니면 32비트 하나를 받는 변수형을 취할 수 있다는 것이다. 그리고 `XMCOLOR` 는 기본적으로 **ARGB** 배치를 사용한다.

대체로 128비트 색상은 정밀도가 높은 색상 연산이 필요한 곳, 특히 픽셀 쉐이더 (프래그먼트 쉐이더 in *OpenGL*) 에서 쓰인다. 128 비트 색상은 정밀도가 높아서 연산 오차가 적은 장점이 있지만, 메모리를 더 쓴다는 단점이 존재할 수 있다. 그리고 아직도 범용 모니터에서는 10bit 가 지원되지 않기 때문에 후면 버퍼에 32비트 색상으로 저장된다.

## 5.4 Rendering Pipeline Introduction

> 그림 5.11 을 보면서 *Direct3D* 의 렌더링 파이프라인 구조를 참고하라.
>
> 혹은 다음 페이지에서 대략적인 렌더링 파이프라인 구조를 참고할 것.
> https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn899200%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

## 5.5 Input Assembler

***Input Assembler*** 단계는 메모리에서 Geometry data (정점 및 색인) 을 읽어서 프리미티브를 만든다. 이 프리미티브는 간단한 도형으로 더 복잡한 형태를 만드는데 사용된다.

### 5.5.2 Primitive topology

정점들은 **정점 버퍼(vertex buffer)** 라고 하는 특별한 *Direct3D* 자료구조에 담겨서, 렌더링 파이프라인에 묶인다. 정점 버퍼 자체에는 *Primitive* 을 생성하기 위한 일련의 정보들만 들어가있다. 따라서, 정점 버퍼를 활용해서 도형을 생성하기 위해서는 **Primitive topology** 을 설정해야 한다.

다음은 *topology* 을 설정하는 데 쓰이는 메서드와 관련 Enumration 이다.

* **`D3D_PRIMITIVE_TOPOLOGY` ** **enumeration**

  > https://msdn.microsoft.com/ko-kr/library/windows/desktop/ff728726.aspx

* **ID3D12GraphicsCommandList::IASetPrimitiveTopology method**

  > https://msdn.microsoft.com/ko-kr/library/windows/desktop/dn903885.aspx

  ``` c++
  void IASetPrimitiveTopology(
    [in] D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology
  );
  ```

이후에 모든 그리기 호출은 **현재 설정된 Primitive Topology 을 사용해서** 그린다. 이 역시 명령 목록에 집어넣어 GPU 가 알아차리도록 한다.

예를 들면 다음과 같다...

``` c++
mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TRIANGLELIST);
/*! 이 후에 독립 삼각형을 적용해서 물체들을 그린다. */
```

#### A. D3D_PRIMITIVE_TOPOLOGY_POINTLIST

위를 `IASetPrimitiveTopology` 을 사용해서 호출하면, **점 리스트**가 현재 위상 구조로 설정이 된다. 이렇게 되면, 정점 버퍼에 입력한 모든 정점에 대해서, ***점*** 들이 그려진다.

#### B. D3D_PRIMITIVE_TOPOLOGY_LINESTRIP

***선 띠***가 위상구조로 그려진다. 이 경우, 정점들을 렌더링할 때 렌더링 사이로 차례로 연결된 선분들이 형성된다. 만약 $ N $ 개의 정점이 그려져야 하면, $ N - 1 $ 개의 선분이 그려진다.

#### C. D3D_PRIMITIVE_TOPOLOGY_LINELIST

***선 목록*** 이 위상구조로 그려진다. 이 경우, 선 띠랑은 약간 다른 것이, 두 정점 사이에 선이 한번 그어지면, 해당 마지막 정점과 다른 위치에 그려진 정점 사이에는 **선분을 형성하지 않는다.** 따라서 $ 2N $ 개의 정점에서 $ N $ 개의 선분이 그려진다.

이 경우, 잘만 사용하면 각 3D 메쉬의 법선 벡터 등을 그릴 때 유용할지도?

#### D. D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP

위 위상구조를 지정하면, 각 정점에 대해서 ***삼각형 띠***를 만들게 된다. 따라서 $ N $ 개의 정점에서 $ N - 2 $ 개의 삼각형이 맥스치로 만들어질 수 있다. 이 때 삼각형 띠는 짝수 번째 삼각형과 홀수 번째 삼각형의 ***Winding order***가 다를 수도 있다는 것에 주의를 해야한다. 이 경우에, **Back-face culling** 으로 인해 의도치 않게 잘려나갈 수 있다. 하지만 이런 사태를 방지하기 위해서, *GPU* 는 내부적으로 짝수 번째 삼각형의 처음 두 정점의 순서를 맞바꾸어서 **홀수 번째 삼각형과 같은 순서**가 되도록 한다.

#### E. D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST

이 인자는 기본 삼각형 리스트를 출력하는 인자로, $ 3N $ 의 정점이 있으면 $ N $ 개의 삼각형이 만들어질 수 있다. 따라서 삼각형 목록으로는 따로 떨어진 삼각형을 형성할 수 있게 된다.

#### F. Adjecency 정보를 가진 Primitive

삼각형 목록을 만들 때, 각 삼각형에 그에 접한 이웃 삼각형 세 개에 관한 정보를 포함할 수 있는데 이것을 **Adjacency (인접성) 정보를 가진 삼각형 목록** 이라고 하고, 주어진 삼각형에 접한 삼각형들을 ***Adjacent triangle*** 이라고 한다.

이런 삼각형 목록 정보는, 특정한 기하 쉐이딩 알고리즘을 구현해야 하는 Geomerty shader 에서 구현할 때 쓰인다. 기하 쉐이더가 그런 인접 삼각형들에 접근하기 위해서는 **삼각형 자체 뿐만 아니라, 인접 삼각형의 정보를 Vertex Buffer 및 Index Buffer**에 담아서 파이프라인에 제출해야 한다. 

그리고 이 경우, **D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ** 로 위상구조를 지정한다.

#### G. Control point patch list

D3D_PRIMITIVE_TOPOLOGY_N_CONTROL_POINT_PATCHLIST 위상구조는 정점 자료를 $ N $ 개의 제어점 (하나의 그룹을 이루는 정점 단위)으로 이루어진 패치 목록으로 해석해야 함을 뜻한다. 이 경우에는 *OpenGL* 과 같이 ***Tesselation*** 단계에서 유용하게 쓰일 수 있다.

### 5.5.3 Index

* 삼각형을 지정할 때는, 감기 순서 (***Winding order***) 을 잘 지정해서 정점 배열에 넣어야 한다.

정점들의 중복이 바람직하지 않은 이유는 크게 두 가지로 볼 수 있다.

1. 메모리 요구량이 증가한다. (VRAM 에 같은 정점 자료가 $ N $ 번 들어갈 수 있다)
2. 그래픽 하드웨어 처리량이 증가한다. (캐쉬 미스, 등의 우려가 커진다)

따라서 이를 해결하기 위해서는, 다음의 방법으로 해결할 수 있다.

1. *TRIANGLE STRIP* 을 이용한다. (좀 귀찮은 방법이긴 하며, 항상 가능하지는 않다)
2. ***Index***을 사용해서, Triangle list 을 위상구조로 사용하면서도 메모리 및 성능 효율을 최대화 할 수 있다.

색인을 이용할려면, 일반 삼각형을 생성하기 위한 정점의 배열 외에도 **색인 배열을 하나 만든다**. 따라서 삼각형을 만들려면 다음과 같이 쓰면 될 것이다.

``` c++
Vertex quad[4] = { v0, v1, v2, v3 };
UINT indexList[6] = { 0, 1, 2, 0, 2, 3 }; 
```

이렇게 색인을 사용해서 얻을 수 있는 장점으로는 다음과 같다.

1. 많은 바이트를 차지하는 정점이 아닌, 일반 숫자만을 가지는 인덱스를 사용해 렌더링하기 때문에, 메모리 사용량이 적다.
2. 일반 정점보다 인덱스가 캐쉬가 유지되고 히트될 가능성이 높다.

## 5.6 Vertex shader

렌더링 파이프라인의 맨 첫번째 단계인 *Input primitive* 단계는, 해당 정점들을 *Vertex shader* 단계로 넘겨준다. 정점 쉐이더는 기본 단위가 **한 정점**이다. 화면에 그려질 모든 정점은 이 정점 쉐이더를 거치며 따라서 다음과 같이 내부에서는 일어난다고 봐도 된다.

``` c++
for (UINT i = 0; i  < numVertices; ++i) {
    outputVertex[i] = VertexShader(inputVertex[i]);
}
```

### 5.6.1 Local space & World space

