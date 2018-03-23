# 7 :: Vertex Processing & Drawing Command

## 7.1 Vertex Processing

*OpenGL* 에서 첫번째로 프로그래밍이 가능한 스테이지는 ***Vertex shader*** 이다. (그 이전의 스테이지는 Vertex fetching 으로 쉐이더에 필요한 정보 등을 가져오는 스테이지이다) 버텍스 쉐이더의 기본 목적은 ***클립 영역의 버텍스 위치***를 *pipeline* 의 다음 스테이지로 가져가는 것이다. 물론 이 과정 역시 생략될 수도 있는데 대개는 그렇지 않다.

### 7.1.1 Vertex Shader input

*OpenGL* 은 많은 종류의 버텍스 속성을 가질 수 있으며, 각 속성은 속성마다 자체 포맷을 가질 수도 있고, 데이터 타입, 컴포넌트 등이 다를 수 있다. 또한 *OpenGL* 은 다른 버퍼 객체로부터 각 속성에 대한 데이터를 읽을 수도 있다.

사실 일반적인 작업은 `glVetexAttribPointer` 로 해결이 가능하지만, 좀 더 저수준의 함수도 지원하고 있다. (`glVertexAttribPointer` 는 밑의 3 함수가 하는 것을 알아서 처리해준다.)

* ***`glVertexAttribFormat(attrib_index, size, type, normalized, relative_offset)`***
  정점 배열 (Vertex 배열) 의 데이터를 구성한다.
* ***`glVertexAttribBinding(attrib_index, binding_index)`***
* ***`glBindVertexBuffer(binding_index, buffer, offset, stride)`***

이 함수들이 어떻게 동작하는 지를 알기 위해서 다음과 같은 코드를 사용해본다.

``` c++
#version 430 core
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;
layout (location = 4) in vec4 color;
layout (location = 5) in int material_id;
```

그리고 다음과 같은 C 언어 구조체를 사용하기로 한다.

``` c
typedef struct VERTEX_t {
    float position[4];
    float normal[3];
    float tex_coord[2];
    unsigned byte color[3];
    int material_id;
} VERTEX;
```

#### A. glVertexAttribFormat

*location* 이 0, 1, 2 인 버텍스 속성에 대해서 `glVertexAttribFormat()` 을 사용할 때는, `size` 을 4, 3, 2 로 하고 `type` 을 `GL_FLOAT` 으로 하면 될 것이다. 하지만 *location* 이 4 인 color 에 대해서는 형이 다르기 때문에 살짝 까다롭다. *vec4* 는 4 바이트를 가지지만, C 구조체에서의 color 는 3 바이트를 가진다. 요소의 개수와 데이터 타입이 다른데, *OpenGL* 은 **데이터를 변환해서 페칭으로 넘겨준다**

``` c++
glVertexAttribFormat(4, 3, GL_UNSIGNED_BYTE, true, offsetof(VERTEX, color));
```

이렇게 하면, unsigned byte 인 color 의 값들은 255 로 나눠져 $ [0, 1] $ 의 값을 가지도록 **정규화**되고, 나머지 하나는 기본 값을 가진채로 버텍스 속성에 넘긴다.

> 이 함수에 대한 자세한 설명은 밑의 주소에서 볼 수 있다.
> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribFormat.xhtml

만약에 `type` 을 지정할 때, 합쳐진 데이터 타입 (`GL_UNSIGNED_INT_2_10_10_10_REV` , `GL_INT_2_10_10_10_REV`) 을 사용하게 된다면, size 는 4 혹은 특별한 값인 **`GL_BGRA`** 로 지정해야 한다. 이렇게 함으로써 자동으로 입력 데이터를 ***Swizzling*** 하여 입력 벡터의 요소를  $ r, g, b, a $ 순으로 만들 수 있다. 

그리고 마지막으로 쉐이더에 데이터를 넘겨줄 때, **정수형**으로 꼭 넘겨줘야 한다면 (일반 함수는 죄다 부동소수점 형으로 넘긴다) 변형 함수인 `glVertexAttribIFormat` 으로 넘겨줄 수 있다.

``` c++
glVertexAttribIFormat(5, 1, GL_INT, offsetof(VERTEX, material_id));
```

마지막 인자인 `relative_offset` is the offset, **measured in basic machine units of the first element relative to the start of the vertex buffer** binding this attribute fetches from.

#### B. glVertexAttribBinding

이렇게 Format 함수를 사용해서 각 속성 인덱스에 어떤 버퍼를 사용해서 데이터를 패칭하겠다고 말했으면, 다음으로 *OpenGL* 에 **어떤 버퍼를 사용해서 데이터를 읽을 지 말해주어야 한다.** *OpenGL* 은 하드웨어에서 제한하는 개수까지의 버퍼로부터 읽어서 그 데이터를 제공할 수 있으며, 일부 버텍스 속성은 버퍼 내의 공간을 공유하도록 할 수 있다. 또한 다른 속성은 다른 버퍼 객체에 있을 수도 있다. 또는 *UBO* 와 같이, **바인딩 포인트 추상 계층을 하나 더 집어넣어 독립적으로 쓸 수 있도록 할 수 있다.**

여기서는 바인딩 포인트에 연관시켜서 *OpenGL* 에 매핑을 시킬 것이다. 

``` c++
glVertexAttribBinding(0, 0);
glVertexAttribBinding(1, 0);
/*! location 2, 4, 5 에 대해서도 똑같이... */
```

> 이 함수에 대한 자세한 설명은 다음 주소를 참고하라.
> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribBinding.xhtml

하지만 더 복잡하게 속성 인덱스 4 를 버퍼 1에, 그리고 5 를 버퍼 2에 바인딩 시킬려고 한다면 다음과 같이 쓸 수도 있다.

``` c++
glVertexAttribBinding(4, 1);
glVertexAttribBinding(5, 2);
```

#### C. glBindVertexBuffer

* `binding_index` 는 버퍼를 바인딩하고자 하는 버퍼 바인딩 포인트의 인덱스이다.
* `buffer` 은 바인딩할 버퍼 객체의 이름이다.
* `offset` 은 정점 데이터가 시작하는 버퍼 객체 상의 오프셋 (바이트 단위) 이다.
* `stride` 는 각 버텍스 데이터의 시작 위치간의 바이트 단위 거리이다. 만약 촘촘하게 버텍스에 비어진 공간이 전혀 없다면 `sizeof` 로도 충분하다. 하지만 위에서는 공간이 빌 수도 있기 때문에 그냥 `0` 으로 하면 *OpenGL* 이 공간을 알아낸다.

> 이 함수에 대해 자세한 정보는 다음 주소에서 알 수 있다.
> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexBuffer.xhtml

### 7.1.2 버텍스 쉐이더 출력

버텍스 쉐이더의 주요한 내장 변수는 다음과 같다.

``` c++
out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
```

`gl_Position` 은 각 정점에 대해 **클립 공간에 위치할 값**을 나타낸다.
그리고 `gl_PointSize` 는 렌더링 될 (수도 있는) **점들의 크기**를 제어한다. (최종 프리미티브가 GL_POINT 가 아니라면 반영되지 않을 수도 있다)
마지막으로 `gl_ClipDistance[]` 는 **클리핑**을 위해 사용된다.

> 자세한 사항은 Chapter 7.4 에서 설명한다.

#### 가변 점 크기

* ***`glPointSize(value)`*** 위치에 상관없는 렌더링 점 고정 크기를 갱신한다.

하지만 위 함수와는 다르게 점 크기를 프로그래밍으로 **가변적으로** 설정할 수 있는 방법이 존재한다. 이를 위해서는 쉐이더 프로그램에서 원하는 점 지름값을 내장 변수 `gl_PointSize` 에 쓰면 된다. 그렇게 하기 위해서는 *OpenGL* 에게 **`gl_PointSize`** 을 뜯어 고치도록 요청하는 과정이 필요하다.

* ***`glEnable(GL_PROGRAM_POINT_SIZE)`***
  을 사용해서 *VS* 에서 `gl_PointSize` 에 임의 값을 (하드웨어가 지원하는 선까지) 설정할 수 있다.

## 7.2 드로잉 커맨드

