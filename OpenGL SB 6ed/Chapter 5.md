# Chapter 5 :: Data

## 5.1 Buffer

* *OpenGL* 에서 ***Buffer*** 는 여러 용도로 활용할 수 있는 *연속적인 메모리 공간*이다.
* *OpenGL* 에서 버퍼는 이름으로 구분되며, 따라서 버퍼를 사용하기 전에 *OpenGL* 을 통해서 고유의 이름을 예약하고, 할당하고 값을 저장하거나 불러와야 한다.
  * 버퍼 객체를 위해 할당한 메모리를 ***Data store*** 라고 한다.
* 버퍼를 위한 ID 를 할당하고 메모리 공간을 만든 뒤에는 이름을 **버퍼 바인딩 포인트**에 바인딩 시켜서 *OpenGL* 이 쓸 수 있도록 해야한다.
  * 버퍼 바인딩 포인트는 ***target*** 이라고 부른다. 하지만 차이점이 존재하는데, 타깃은 여러 개의 버퍼 바인딩 포인트를 가질 수 있다.

### 5.1.1 Allocate memory using buffer

* 버퍼 객체를 사용해서 메모리를 할당하기 위해 사용하는 함수는
  ***`glBufferData(GLenum target, GLsizeiptr size, const GLvoid_p data, GLenum usage)`***
  이다. 이를 이용해서 만들어진 버퍼에 어느 정도의 메모리 바이트를 쓰도록 할 것인지를 할당할 수 있다.
  * 물론 이 함수를 쓰기 전에, *`glGenBuffers(GLsize_t i, GLbuffer_p)`* 을 사용해서 버퍼의 이름을 생성해줘야 한다.
  * `target` 매개 변수는 버퍼가 어떤 타깃에 바인딩이 될 것인지를 *OpenGL* 에게 알려준다.
    * 예를 들면, 다른 그래픽스 라이브러리처럼 **Vertex Buffer** 혹은 **Uniform Buffer** 와 같이 버텍스 속성을 담을 데이터를 저장하는 버퍼를 *OpenGL* 의 매체에 바인딩하고자 할 때는 `GL_ARRAY_BUFFER` 타깃을 사용한다.
  * `size` 매개 변수는 해당 버퍼의 메모리 영역을 얼마만큼 할당할 것인지 결정하는데 쓰인다. 이 값은 nullable 할 수 없다.
  * `data` 는 버퍼에 들어갈 데이터의 시작 지점을 포인터로 받는다. 만약 `nullptr` 이라면 해당 버퍼의 메모리 공간의 값들은 미정의되거나 초기화되지 않은 상태로 된다. (쓰레기 값이 들어간다)
  * `usage` 는 버퍼를 어떤 목적으로 사용할 지를 결정한다. (`target` 과는 다르다) 

> 자세한 쓰임법 및 동작 원리는 다음을 참고한다.
> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml

위 함수는 다음과 같이 쓸 수 있다.

``` c++
GLuint buffer;
glGenBuffers(1, &buffer);
glBindBuffer(GL_ARRAY_BUFFER, buffer);
glBufferData(GL_ARRAY_BUFFER, 2^20, nullptr, GL_STATIC_DRAW);
```

주의해야 할 점은, `glBufferData` 는 특정 버퍼의 ID 가 아닌, 타깃에 대해 쓰임과 메모리 할당과, 값을 결정하므로 이 함수를 사용하기 이전에 ***`glBindBuffer`*** 을 사용해서 특정 버퍼를 타깃에 올려야 한다.

그리고 위에서는 `nullptr` 을 사용해서 해당 버퍼의 값을 초기화시키지 않았다. 그러면 추후에 버퍼에 데이터를 전달하기 위해서는 *OpenGL* 의 함수를 사용하면 된다.

1. ***`glBufferSubData(target, offset, size_byte, data)`***
   은 해당 *target* 에 대해 *offset* 위치에서 *size_byte* 만큼 *data* 의 값들을 복사한다.

다음과 같이 쓸 수 있다.

``` c++
static const float data[] = { /*! Data... */ };
glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);
```

2. 버퍼 객체에 해당하는 **메모리 포인터**를 직접 *OpenGL* 에 요청해서 데이터를 복사한다. 이 경우는
   ***`glMapBuffer(target, mode)`*** 을 사용한다. 
   C API 등을 사용해서 메모리를 옮긴 뒤에는 *`glUnmapBuffer(target)`* 을 사용한다. 그 뒤에 다시 위치를 할당한 포인터를 이용해 값을 쓸려고 하면 미정의 행동이 발생한다.

``` c++
static const float data[] = { /*! Data... */ };
void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
memcpy(ptr, data, sizeof(data));
glUnmapBuffer(GL_ARRAY_BUFFER);
```

### 5.1.2 Buffer 에 데이터 채우고 복사하기

만약 버퍼에 저장하고자하는 데이터가 **하나의 상수값**이라면,
***`glClearBufferSubData(target, internal_format, offset_bt, size_bt, format, type, data)`*** 을 사용한다.

* 버퍼 객체를 채울 값을 갖는 변수 포인터 `data` 가 주어지면, 해당 값을 `internal_format` 에 명시된 규격에 맞게 **변환**하고, 변환된 내용을 `offset` 바이트 위치에서 `size` 바이트 만큼 복사한다.
* ***`format`*** 과 ***`type`*** 은 *OpenGL* 에 data 를 통해 그 데이터가 가리키는 포인터에 대한 **정보**를 알려준다. 따라서 넘기고자 하는 데이터의 포맷과 타입이 맞지 않으면, *OpenGL* 은 미정의 행동을 일으킬 수 있다.

> 자세한 설명은 다음 주소에서 볼 수 있다.
> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glClearBufferSubData.xhtml

만약 *OpenGL* 의 문맥이 가지는 버퍼의 데이터를 또 다른 버퍼로 전달하고 싶다면,
***`glCopyBufferSubData(read_target, write_target, read_offset, write_offset, size)`*** 을 사용한다.

* 주의해야 할 점은, 이 함수를 사용할 때는 `glBindBuffer` 을 통해 송신/수신하고자 하는 버퍼들을
  타깃 `GL_COPY_READ_BUFFER` `GL_COPY_WRITE_BUFFER` 로 바인딩하는 것이 매우 좋다.
  * 만약 `size` 가 한계 범위를 넘으면 복사가 실패한다.

> gl 타입형인 `GLsizeiptr` 은 포인터 변수를 담을 만큼 플랫폼에서 큰 정수형으로 정의되어 있다.

### 5.1.3 정점 쉐이더에 입력 전달

우선 버텍스 배열 객체를 하나 만들어서 상태를 저장할 수 있도록 한다.

``` c++
GLuint vao;
glGenVertexArrays(1, &vao);
glBindVertexArray(vao);
```

* ***`glVertexAttribPointer(index, size, type, normalized, stride, GLvoid_p offset)`*** 
  *OpenGL* 에 데이터가 버퍼 객체 (VAO) 의 어디 위치에 있는가를 알려주기 위해 사용한다.
  중요한 것은, 이 `glVertexAttribPointer` 는 버퍼 타깃 `GL_ARRAY_BUFFER` 을 참조하기 때문에 **사전에 읽고자 하는 버퍼를 바인딩**해야 한다.
  * `index` 는 버텍스 속성의 인덱스이다. `uniform (layout = $x$)` 와 맞지 않으면 쉐이더 프로그램이 잘 작동하지 않을 것이다.
  * `size` 는 각 버텍스에 대해 버퍼에 저장된 요소의 개수를 나타낸다.
  * `type` 은 해당 요소의 값 타입을 나타낸다.
  * `normalized` 는 *OpenGL* 에서 데이터를 읽어 올 때, 해당 값들이 정규화 되서 $ [0, 1] $ 사이의 값으로 되어야 하는가를 설정한다. 만약에 정수형이고 정규화되지 않는다고 하더라도 *OpenGL* 에서는 해당 값을 모두 **부동소수점** 으로 변환하여 읽는다.
  * `stride` 는 읽어올 데이터의 **바이트 크기**를 말한다. 이 바이트 크기는 실제 읽어올 값의 크기보다 클 수도 있다. (이 부분 중요)
  * `offset` 은 읽어올 데이터의 **시작 위치**를 말한다. 이 인자는 위의 *stride* 와 엮어서 규격화된 데이터를 유연하게 받아오게 할 수 있다.
* 그리고 ***`glEnableVertexAttribArray(index)`*** 을 사용해서 *OpenGL* 에게 지정한 속성에 대해 ***Vertex fetching***을 하도록 지시한다. 만약에 페칭을 마쳤다면, *`glDisableVertexAttribArray(index)`* 을 사용해서 다시 비활성화 시킬 수 있다.

``` c++
glBindBuffer(GL_ARRAY_BUFFER, buffer);
glVertexAttribPointer(/*! ... */);
glEnableVertexAttribArray(0);
glDisableVertexAttribArray(0);
```

#### 버텍스 쉐이더에 여러 개의 입력 사용하기

하나의 VAO 에 여러 개의 입력을 사용해서 버텍스 쉐이더 혹은 그 외 각종 쉐이더에서 사용하게 할 수 있다.

* 만약 버텍스 쉐이더가 여러 개의 입력을 가진 프로그램 객체를 링크했고, 각 프로그램마다 변수의 지정된 위치가 다를 때, 해당 변수의 **위치**를 다음 함수로 확인할 수 있다.
  ***`GLint glGetAttribLocation(GLuint program, const char_p name);`***
  * 만약 찾을 수 없으면 `-1` 을 반환한다.
  * 또한 **쉐이더 코드에 위치를 명시적으로 설정하지 않았더라도** *OpenGL* 은 이를 설정해주기 때문에 해당 함수로 묵시적인 위치를 얻을 수 있다.

## 5.2 Uniform

### 5.2.2 Uniform block

어플리케이션에서 수 많은 쉐이더를 사용해야 한다면, 모든 쉐이더에 대해서 유니폼을 설정하고, 유니폼에 값을 넣기 위해서 `glUniform*()` 함수를 호출할 수 밖에 없다. 

그리고 어떤 유니폼이 변경되는 지도 관리해야 하며, 결과적으로 *관리하기 복잡*해질 뿐더러, 유연하게 유니폼을 변경할 수 없기 때문에 (OpenGL 에서 값이 변했는지 안 변해는지 알기 어려우므로) 매 프레임마다 값이 변하지 않더라도 갱신해줘야 하는 문제가 생긴다.

* 이런 성능 저하와 유지 보수의 어려움을 막기 위해서 *OpenGL* 은 ***Uniform block*** 이라는 **유니폼을 그룹화**해서 VAO 에 저장할 수 있도록 하는 것을 만들었다. 
  * 버퍼 바인딩을 변경하거나, 바인딩된 버퍼의 내용을 덮어쓰면 빠르게 설정이 가능하다.
  * 쉐이더 프로그램을 변경해도, **버퍼 바인딩을 그대로 둘 수 있다**. 따라서 새로운 프로그램 역시 현재 상태의 유니폼 블록의 값들을 가진다. 이런 기능을 ***Uniform block object*** 라고 부른다.

> 사실 기존의 유니폼 변수 역시 보이지 않는, 기본 유니폼 블록에 위치하고 있었다.

* 쉐이더 프로그램에서 *Uniform block object* 을 사용하려면, ***`uniform`*** 키워드를 사용해서 **이름이 있는** 유니폼 블록을 사용해야 한다.

``` c++
uniform TransformBlock {
    float scale;
    vec3 translation;
    float rotation[3];
    mat4 projection_matrix;
} trnasform;
```

위 코드는 *TransformBlock* 이라는 이름을 가진 *UBO* 를 정의한다. 또한 그 블록에 대한 인스턴스로 *transform* 이라는 객체 역시 정의한다. 쉐이더에서는 인터페이스 블록처럼 *UBO* 의 멤버를 참조할 수 있다. 문제는, 쉐이더에 쓰일 *UBO* 의 멤버의 값을 **어떻게 채우느냐** 이다. 

#### 유니폼 블록 만들기

*UBO* 의 멤버의 값을 채우기 위해서는 다음과 같은 2 가지의 선택지가 존재한다.

1. **표준적인 방법**으로 데이터의 레이아웃에 의존한다. 즉, **어플리케이션이 버퍼로 그냥 데이터를 복사해서 멤버의 블록 내 위치가 그대로 일치한다고 가정한다는 것이다.** 이렇게 할 경우에는, 블록의 멤버 사이 사이에 메모리 참조의 효율성을 올리기 위해 빈 공간을 둘 가능성이 존재하기 때문에 위험할 수 있다. 하지만 대부분의 경우에는 안전하다.
2. 데이터가 어디에 위치하게 할 지 *OpenGL* 이 결정하게 한다. **효율적인 쉐이더**를 만들 수 있지만 *UBO* 의 멤버의 위치를 어플리케이션이 알아내야 한다. 이 방식으로 저장된 데이터들은 ***공유된 레이아웃 형태***로 정렬된다.
   * 이것이 바로 **기본 레이아웃**이고, *OpenGL* 에서는 명시적으로 요구하지 않는 한 이 형태로 받는다.
   * 이렇게 함으로써, 메모리 참조의 효율성이 올라가고 성능이 개선되지만, 어플리케이션에서는 조금 트릭키한 구현이 요구된다.
   * 또한 이렇게 함으로써, **여러 프로그램과 쉐이더가 동일한 유니폼 블록 선언을 공유**하면서, 동일한 버퍼 객체를 사용해서 메모리 요구량을 줄이기도 한다. 다만 공유 레이아웃을 쓰기 위해서는 *UBO* 의 멤버의 위치를 어플리케이션 단에서 설정해야 한다.

##### 공유 레이아웃 

* 공유 레이아웃을 쓰기 위해서는 `uniform` 앞에 `layout(std140)` 이라는 지시어를 사용해야 한다.

``` c++
layout(std140) uniform TransformBlock {
    float scale;
    vec3 translation;
    float rotation[3];
    mat4 projection;
} transform;
```

* 유니폼 블록이 **`std140`** 으로 선언이 되면 각 블록의 멤버는 버퍼에 미리 정의된 양만큼의 메모리 공간을 차지할 것이며, 일련의 **규칙**에 의해서 오프셋만큼 지난 위치에서 시작한다.

> 버퍼에서 $ N $ 바이트를 차지하는 타입은 그 버퍼의 $ N $ 바이트 경계에서 시작한다. 예를 들어 `int` `bool` `float` 과 같은 타입은 실 값이 아닌, 메모리 공간에서는 4 바이트를 차지할 것이므로 표준 GLSL 타입들은 **4 바이트의 배수 위치**에서 시작한다.
>
> 만약 $ 8 $ 바이트의 경우에는 $ 2N$ 바이트 경계에서 시작하며, 그 이상의 3 요소, 4 요소 벡터는 항상 **$ 4N $** 바이트 위치에서 시작한다. 
>
> *UBO* 의 std140 멤버의 배열의 각 멤버는 $ 4N $ 바이트 위치로 정렬되어 시작한다. 행렬은 벡터의 긴 벡터 배열로 간주하기 때문에, 항상 $ 4N $ 바이트 정렬이 된다.

> 자세한 사항은 이 밑의 주소의 중반에서 볼 수 있다.
> https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt

이렇게 `std140` 레이아웃과 실제 *C++* 컴파일러가 메모리를 할당하는 구조는 차이가 있기 때문에 C 배열의 데이터를 그대로 복사할 수가 없다. 그래서 공유 레이아웃을 쓰고 싶으면, *OpenGL* 에서 *UBO* 의 멤버를 어느 지점에 메모리 위치를 시켰는가를 알아내야 한다. 

* ***`glGetUniformIndices(GLuint program, GLsizei uniofrm_count, GLchar_ptr2 name, GLuint indices)`***
  이 함수를 사용해서 한번에 **많은 수의 유니폼 블록 멤버의 인덱스**(메모리 위치가 아니다)를 가져올 수 있다. 심지어 다른 블록의 멤버도 가능하다. 이 함수는 다음과 같이 쓸 수 있다.

``` c++
static const GLchhar* uniformNames[4] = {
    "TransformBlock.scale", "TransformBlock.translation",
    "TransformBlock.rotation", "TransformBlock.projection_matrix"
};
GLuint uniformIndices[4];
glGetUniformIndices(program, 4, uniformNames, uniformIndices);
```

이제 얻어온 인덱스들을 활용해서 해당 멤버의 **여러가지 정보**를 가져올 수 있다.

* ***`glGetActiveUniformsiv(program, count, indices_ptr, option_name, output_params_ptr)`***
  이 함수는 **`option_name`** 을 통해 특정 유니폼 블록 멤버에 대한 많은 정보를 제공하는 함수이다. 오프셋, 배열 스트라이드, 등을 확인할 수 있다. 

``` c++
GLint uniformOffsets[4];
GLint arrayStrides[4];
GLint matrixStrides[4];
glGetActiveUniformsiv(program, 4, uniformIndices, GL_UNIFORM_OFFSET, uniformOffsets);
glGetActiveUniformsiv(program, 4, uniformIndices, GL_UNIFORM_ARRAY_STRIDE, arrayStrides);
glGetActiveUniformsiv(program, 4, uniformIndices, GL_UNIFORM_MATRIX_STRIDE, matrixStrides);
```

>여기서 option_name 의 정보를 얻을 수 있다.
>https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveUniformsiv.xhtml

이제 버퍼 내의 유니폼의 위치를 알았으니 다음과 같은 방법으로 값을 전달할 수 있다.

1. **`glBufferSubData()`** 과 오프셋을 사용해서 적절한 위치에 데이터를 쓴다.
2. 직접 C API 을 사용해 **메모리 상에서 버퍼를 조합**해서 넘긴다.
3. **`glMapBuffer()`**을 사용해서 버퍼 메모리에 대한 포인터를 얻어 직접 데이터를 조작한다.

2번의 방법은 성능이 좋지만, 까다롭다는 단점이 존재한다.

> 쓰기 힘들기 때문에 자세한 사항은 p133 을 참고한다. C malloc 과 free, reinterpret_cast<> 을 통해서 매우 저수준의 프로그래밍 코드를 적는다.

#### 본제로 돌아가서...

* **`glGetIntegerv(GL_MAX_UNIFORM_BUFFERS)`** 을 사용해서 프로그램 당 사용 가능한 유니폼 블록의 최대 개수를 알 수 있다.
* ***`glGetUniformBlockIndex(program, name)`*** 을 사용해서 **유니폼 블록의 인덱스**를 얻을 수 있다.

여기서, 어플리케이션 단의 버퍼를 *UBO* 에 바인딩하기 위해서는 한 단계가 더 필요하다. 직접적으로 쉐이더 프로그램에 바인딩이 됬던 일반 유니폼 변수와는 다르게 **바인딩 포인트**라고 하는 추상화 레이어를 통해 실제 프로그램에 바인딩을 해야한다. 이 추상화를 통해 프로그램이 바뀌어도 일부 유니폼이 자동으로 새로운 프로그램에 보일 수 있게 된다.

그리고 유니폼 블록의 인덱스에 **바인딩 포인트**를 할당할려면 다음 함수를 호출한다.

* ***`glUniformBlockBinding(program, index, binding_number)`***
  `index` 는 `glGetUniformBlockIndex()` 로 얻을 수 있다.
  `binding_number` 는 유니폼 블록 바인딩 포인트에 대한 인덱스이며, 만약 프로그램에서 명시적으로 `binding = N` 으로 지정하지 않으면 이 함수가 꼭 필요하다.

이렇게 함으로써, 해당 **바인딩 포인트**에 버퍼를 **할당**함으로써 *OpenGL* 에서 버퍼를 *UBO* 에 보이게 할 수 있다.

이제 마지막으로 다음 함수를 호출하여 버퍼를 실제 유니폼 바인딩 인덱스에 연결해서 값을 보이게끔 한다.

* ***`glBindBufferBase(GL_UNIFORM_BUFFER, binding_number, buffer)`***

  `buffer` 는 어플리케이션 단 버퍼 객체의 이름을 말한다.

***UBO*** 는 임시 상태와 고정 상태를 분리할 때 유용하게 사용될 수 있다. 그리고 UBO 는 프로그램마다 기본 64KB 와 14개의 UBO 를 지원하고 있다.

다음은 위 과정을 나타낸 프로그램 코드이다.

``` c++
GLuint harry_index = glGetUniformBlockIndex(program, "Harry");
GLuint bob_index = glGetUniformBlockIndex(program, "Bob");
Gluint susan_index = glGetUniformBlockIndex(program, "Susan");

glUniformBlockBinding(program, harry_index, 1);
glUniformBlockBinding(program, bob_index, 3);
glUniformBlockBinding(program, susan_index, 0);

glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffer_b);
glBindBufferBase(GL_UNIFORM_BUFFER, 1, buffer_c);
glBindBufferBase(GL_UNIFORM_BUFFER, 3, buffer_a);
```

### 5.2.3 유니폼을 사용해서 지오메트리 변환하기

> Chapter5 main.cc 을 참고한다.

## 5.3 Shader Storage Buffer Object (OpenGL 4.3~)

* ***SSBO*** 을 사용하면, 유니폼 버퍼 객체를 쉐이더 프로그램 안에서 값을 갱신할 수 있다. 유니폼 블록과 비슷하게 선언 가능하고, 다른 점은 **타깃**이 `GL_SHADER_STORAGE_BUFFER` 에 바인딩을 해야 한다는 것이다. 
  * 또한, 다른 스레드가 문맥 교환을 하지 않는 **Atomic 연산**을 지원하기 때문에 동기화 문제를 해결할 수 있다.
  * 쉐이더 스토리지를 쉐이더에서 쓰기 위해서는, `uniform` 대신에 ***`buffer`*** 지시어를 사용한다. SSBO 역시 `std140` 레이아웃 지시어를 지원하지만, `std430` 역시 지원한다.
* `std430` 과 `std140` 패킹 지시어의 차이는 촘촘하게 패킹된 정수 배열 및 부동소수점 변수를 지원하느냐 마느냐의 차이이다. 촘촘하게 패킹되었다는 것의 의미는, 메모리를 더 효율적으로 사용할 수 있게 되었다는 것이다.

밑은 SSBO 를 사용한 쉐이더 블록 선언 예제이다.

``` c++
#version 430 core
struct my_structure {
    int 	pea;
    int 	carrot;
    vec4 	potato;
};
struct vertex {
    vec4 position;
    vec3 color;
};
layout (binding = 0, std430) buffer my_storage_block {
    vec4    foo;
    vec3    bar;
    int     baz[24];
    my_structure veggies;
};
layout (binding = 1, std430) buffer my_vertices {
    vertex vertices[];
};
```

* *UBO* 와 동일하게, `glBufferData()` 을 통해서 버퍼에 데이터를 전달할 수도 있다.
* `glMapBuffer()` 와 모드 `GL_READ_ONLY(GL_READ_WRITE)` 을 통해 해당 버퍼의 값을 참조할 수도 있다.
* 다만 *UBO* 나 *버텍스 속성* 과 같이 Compact 하지 않으며 여러가지 기능을 제공하기 때문에, *OpenGL* 에서는 최적화의 여지가 더 적을 수도 있다.
  * 예를 들면, **Vertex Attribute** 는 버텍스 쉐이더가 실행하기 한참 전에 수행하여 *OpenGL* 의 서브메모리 시스템에 적재할 수 있다. 하지만 SSBO 의 경우에는 쉐이더 중간에 데이터를 읽을 수 있기 때문에 성능저하가 있을 수 있다.

#### Atomic memory operation

* ***SSBO*** 는 해당 연산 도중에 문맥 교환, 선점 등의 방지를 보장하는 **atomic 연산**을 보장한다. 이론적으로는 여러 쉐이더 호출이 서로 다른 메모리 위치에 대해서 어토믹 연산을 수행하는 경우에는 보통 때 처럼 문제없이 빠르게 동작을 해야한다. 하지만 서로 다른 스레드가 Race condition 을 빚을 경우에는 해당 스레드의 작업이 **직렬화 (Serialization)** 이 되면서 한 번에 하나씩 수행된다.

> Atomic 연산을 수행하는 함수는 다음 주소에서 볼 수 있다.
> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/atomicAdd.xhtml

위 주소에서 알 수 있는 점은, Atomic 연산은 오직 **32bit 정수형**만 지원한다는 것이다. 그리고 모든 어토믹 연산 함수는 연산이 수행되기 **전에** 있던 값을 반환하며, 만약 Race condition 이 발생하는 경우에는 해당 연산들의 순서가 보장되지 않는다.

### 5.3.1 Synchronization of memory accessing

쉐이더에서 버퍼 객체에 (SSBO) 데이터를 쓸 때는, `AtomicXYZ` 을 쓰던가 `glMapBuffer` 을 쓰던지 신경 쓰지 않으면 발생하는 문제가 존재한다.

1. ***Read-after-Write*** 문제는 프로그램이 메모리 위치에 쓴 다음에 읽기를 시도할 때, 최적화 혹은 아키텍쳐에 의해서 쓰기가 완료되기도 전에 읽기가 수행될 수도 있다.
2. ***Write-after-Write*** 문제는, 해당 위치에 다른 값들을 여러 번 덮어 쓸 때, 아키텍쳐에 따라서 메모리의 값이 맨 마지막에 쓰일 값으로 덮어 씌워지지 않을 수도 있다.
3. ***Write-after-Read*** 문제는 읽기를 수행하는 스레드가 다른 스레드가 쓴 데이터를 가져올 수도 있다는 현상이 있다.

* 따라서 이런 병렬 프로그래밍에서 발생할 수 있는 특유의 현상을 억제하기 위해, ***Memory barrier*** 라는 개념을 사용할 수 있다. 이 메모리 배리어는, *OpenGL* 에게 특정 단위의 스레드들이 해당 지점에 도착할 때 까지 진행을 억제하도록 한다. 
  * 이 *Memory barrier* 는 어플리케이션 및 Shader 프로그램에서 동작하도록 할 수 있다.

#### 어플리케이션에서 배리어 사용

* ***`void glMemoryBarrier(GLbitfield barriers)`*** 

#### 쉐이더에서 배리어 사용

* ***`void memoryBarrier()`***

## 5.4 Atomic counter

## 5.5 Texture

* ***Texture*** 는 읽기 및 쓰기가 가능한 구조화된 버퍼를 갖는다. 따라서 그냥 이미지의 정보를 저장하는 역할을 하기도 하지만, 다른 용도로도 사용할 수 있다.
  * 대개 *Texture* 은 2차원 배열을 가리키지만, 해당 텍스쳐를 배열로 포개거나 하는 식으로 3차원으로도 구성할 수 있으며, 1차원으로도 구성할 수 있다. 또한 나중에 알아볼 큐브 환경맵과 같이 정육면체 등으로도 생성할 수 있다.
* ***Texture*** 는 ***Texture unit*** (a piece of hardware that takes a sample of a texture) 에 바인딩해서 관리 할 수 있는 객체 형태로 표현될 수 있다. 다른 버퍼와 같이, 텍스쳐 역시 `glGenTextures` 후에, 만들어진 이름을 유닛에 바인딩 시켜야 텍스쳐로 표현될 수 있다.
  * 하지만, 버퍼 객체를 버퍼 바인딩 포인트에 바인딩하는 것과는 다르게 텍스쳐 이름이 타깃에 바인딩되면, **소멸할 때까지 해당 타입의 타깃이 유지**된다.

### 5.5.1 Texture 생성 및 초기화

텍스쳐를 생성하려면, 텍스쳐 객체를 생성하고, 그것을 타깃에 바인딩시키고, *OpenGL* 에 저장할 텍스쳐 사이즈를 알려줘야 한다.

* ***`glGenTextures(count, instance_ptr)`***
  `glGenBuffers` 와 동일한 메커니즘을 가지며, 다만 텍스쳐에 대해 "이름"을 만든다.
* ***`glBindTexture(target, name)`***
  텍스쳐 "이름"을 타깃에 바인딩한다.
* ***`glTexStorageXY()`*** 함수는 텍스쳐 타깃에 대해서 사용할 메모리 공간의 양을 정한다.
  * **`glTexStorage2D(target, mipmap_level, format, width, height)`** 
    은 2D 텍스쳐에 대해 메모리 공간 및 밉맵 레벨, 픽셀 당 내부 포맷을 할당한다.
* 만약에 공간을 할당하면서, 데이터를 넣기를 원하면 ***`glTexImageXY()`*** 계열의 함수를 사용할 수 있다.

위 3 가지 함수를 이용한 코드는 다음과 같을 수 있다.

``` c++
GLuint texture;
glGenTextures(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);
glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 256, 256);
```

만약 텍스쳐의 메모리 공간에 일부 데이터를 지정하고 싶다면 다음 함수를 쓴다.

* ***`glSubTexImageXY()`*** 함수는 텍스쳐 타깃에 대해서 일부 사각형 공간에 값을 갱신하거나 적는다.
  * **`glSubTexImage2D(target, mipmap_lv, x, y, width, height, format, data_format, data)`**
    `target` 의 밉맵 레벨 메모리 공간에, $ ([x, width), [y, height)) $ 영역을 data 값으로 옮긴다. 물론 데이터 타입과 해당 밉맵 레벨의 픽셀의 포맷이 맞아야 제대로 동작한다.

이를 활용한 코드는 다음과 같을 수 있다.

``` c++
float* data = new float[256 * 256 * 4];
generate_texture(data, 256, 256); /*! 라는 함수가 있다고 치자 */
glGenTexutres(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);
glSubTexImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_FLOAT, data);
delete[] data;
```

> 자세한 설명은
> https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glTexImage2D.xml 
> 에서 볼 수 있다.

### 5.5.2 Texture target, type

* 텍스쳐 타깃은 다음과 같다.

| GL_TEXTURE_*   | 설명                                                         |
| -------------- | ------------------------------------------------------------ |
| 1D             | 1차원 텍스쳐 (높이가 1)                                      |
| 2D             | 2차원 텍스쳐 (주로 쓰임)                                    |
| 3D             | 3차원 텍스쳐 (**볼륨**을 표현시 사용, 3차원 텍스쳐 좌표를 가짐)             |
| RECTANGLE      | 사각형 텍스쳐 (2D와 비슷하지만, non-mipmap 에 irregular 사이즈 지원) |
| 1D_ARRAY       | 1차원 배열 텍스쳐                                            |
| 2D_ARRAY       | 2차원 배열 텍스쳐                                            |
| CUBE_MAP       | 큐브 맵 텍스쳐                                               |
| CUBE_MAP_ARRAY | 큐브 맵 배열 텍스쳐                                          |
| BUFFER         | 버퍼 텍스쳐 (1D 와 비슷하나, 버퍼 객체로 표현되는 점이 다름)                  |
| 2D_MULTISAMPLE | 2차원 멀티 샘플 텍스쳐 (**MSAA**) |
| 2D_MULTISAMPLE_ARRAY | 2차원 배열 멀티 샘플 텍스쳐 (**MSAA**) |

> 여기서, `RECTANGLE` 은 version 3.1 부터 지원된 기능인데 나올 당시만 해도 텍스쳐 사이즈는 **power of 2** 사이즈가 아니면 텍스쳐 할당이 되지 않았는데 이것을 씀으로써 power of 2 가 아닌 사이즈를 가진 텍스쳐를 받아들일 수 있게 되었다. 하지만 *mipmap* 이 지원되지 않는 단점이 있고 현재 버전에서는 power of 2 가 아니더라도 받을 수 있기 때문에 효용성이 많이 떨어진다.
>
> This mainly has to do with their ability to use non-normalized texture coordinates in conjunction with filtering. The texelFetch functions bypass all filtering; they take integer texture coordinates. However, with texture rectangles, you can work directly in texel coordinates, but with floating-point values. This is a useful convenience.
>
> It could certainly be emulated by using the textureSize function to get the size and manually normalize a texture coordinate. But more often than not, rectangle textures will look more natural in the shader code.

### 5.5.3 쉐이더에서 텍스쳐 읽기

* 텍스쳐를 쉐이더에서 읽어서 프래그먼트 쉐이더에 적용하게 할 수도 있다.
  텍스쳐는 쉐이더에서 ***Sampler variable (샘플러 변수)*** 로 사용되며, 샘플러 타입 및 `uniform` 으로 선언하면 외부에서도 접근하게 할 수 있다. 
  * 이런 샘플러 변수 타입도 위의 `GL_TEXTURE_` 와 같이 여러 타입이 존재한다.

``` c++
#version 430 core
uniform sampler2D s;
out vec4 color;
void main() {
    color = texelFetch(s, ivec2(gl_FragCoord.xy), 0);
}
```

* 쉐이더 함수 ***`vec4 texelFetch(sampler2D, ivec2 position, int lod)`*** 
  $ (0, 0) $ 에서 텍스쳐의 Width, Height 까지의 **정수형 좌표** 를 position 으로 입력 받고, 밉맵 레벨 lod 에서 텍셀을 가져온다. 
  *  이 함수는 여러가지 오버로딩 버전을 가지며 따라서 첫번째 샘플링 인자 타입에 따라 어떤 함수가 오버로딩 될 지 결정된다.
  * **모든 텍스쳐 함수** 는 **vec4** 를 리턴하는데, 만약 네 채널보다 적은 채널을 가지는 텍스쳐를 읽을 때는 $ (R, G, B) := {0} $ 이 되며, 알파 채널은 $ 1 $ 로 채워진다. 물론 불필요한 채널은 쉐이더 컴파일러에서 최적화를 해서 없애준다.
* ***`gl_FragCoord`*** 내장 변수는 해당 프래그먼트의 보간된 텍셀을 가리킨다.

#### 텍스쳐 좌표 (TexCoord)

* ***`texture(sampler2D, vec2 p)`*** 
  해당 샘플링 텍스쳐의 ***Texture Coordinate*** 좌표 값을 이용해 텍셀을 가져온다.

> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/texture.xhtml

### 5.5.5 텍스쳐 데이터 읽는 방식 제어하기

* 일반적으로 *OpenGL* 에서 텍스쳐 좌표는 **정규화**된다. 따라서 텍스쳐 좌표는 범위 $ [0, 1] $ 을 가진다. *OpenGL* 은 이 범위를 벗어나는 텍스쳐 좌표에 대해서 ***Wrapping Mode*** 을 설정하여 어떻게 처리할 지를 제어할 수 있다.
* 또한 각 정점에서 보간된 샘플의 중간 값을 어떻게 계산할 지도, ***Filtering Mode*** 을 통해서 정할 수 있다.
* 샘플러 및 필터링 모드를 제어하는 인자는 텍스쳐에 독립적인 ***Sampler Object*** 에 저장된다.

*샘플러 객체* 를 쓰기 위해서는 다음과 같은 코드를 거친다.

``` c++
void glGenSamplers(GLsize n, GLuint* samplers);
void glSamplerParameteri(GLuint smapler, GLenum pname, GLint param);
void glSamplerParameterf(GLuint sampler, GLenum pname, GLint param);
void glBindSampler(GLuint unit, GLuint sampler);
```

`sampler` 인자는 만들어진 샘플러 객체를 바인딩하는데 쓰인다. 따라서 *타깃*에 바인딩해서 설정하지 않고, 샘플러 객체를 직접 수정할 수 있다. 

`pname` 은 해당 샘플러 객체의 옵션, 그리고 `param` 은 객체 옵션에 대한 값 인자 (`GL_`) 을 받는다. 

맨 마지막의 `glBindSampler` 함수는 **샘플러 객체**와, **바인딩할 대상 텍스쳐 유닛에 대한 인덱스**를 사용한다. 다만 샘플러 객체는 여러 텍스쳐 유닛에 적용이 가능하기 때문에 유연함을 보장한다. 텍스쳐 개수가 많은 때에도 각 텍스쳐마다 인자를 지정할 필요가 없이 동일한 샘플링 객체를 쓸 수 있다. 또한 텍스쳐 유닛의 텍스쳐를 변경시에도 텍스쳐 객체는 자동 해제되지 않는다. 그리고 동일한 텍스쳐를 다른 객체로 설정해 다른 인덱스에서 읽을 수 있다.

#### 여러 텍스쳐 사용하기

하나의 쉐이더에서 여러 개의 텍스쳐를 사용하려면, 여러 샘플러 유니폼을 만든 후에 각각 다른 텍스쳐 유닛을 참조하도록 하면 된다. 다만 여러 개의 텍스쳐를 쉐이더 컨텍스트에 바인딩을 해야하는데 *OpenGL* 은 이를 위함 함수를 제공하며, 하드웨어 당 몇 개의 바인딩을 걸 수 있는지 알 수 있게 함수를 제공한다.

* ***`glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)`***
  해당 함수는 현재 하드웨어에서 쉐이더에 바인딩 할 수 있는 최대 텍스쳐 유닛 수를 반환한다.
* ***`glActiveTexture(GL_TEXTURE0 ... GL_TEXTURE31)`***
  이 함수는 특정 텍스쳐 유닛에 텍스쳐 버퍼를 바인딩 할 수 있도록 제공한다. 따라서 다음과 같이, 쉐이더의 특정 텍스쳐에 `GL_TEXTURE_2D` 등의 버퍼를 바인딩할 수 있도록 한다.

``` c++
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture_one);
```

여러 텍스쳐를 쉐이더 문맥에 반영을 했다고 하면, 쉐이더 코드에 적힌 각각의 샘플러 객체들이 해당 유닛을 **참조하도록** 하지 않으면 안된다. 여기에는 두 가지 방법이 있다.

1. ***`glUniform1i(GLint location, GLint v0)`***
   을 사용해서 (물론 쉐이더가 켜져 있어야 한다) 해당 유니폼 변수의 위치를 가져와, 거기에 **원하는 유닛 인덱스**를 사용한다. 이 방법은 어플리케이션에서 직접 설정해 줘야 한다.
   * 사실, 샘플러 변수의 바인딩 넘버가 실제로 정수로는 취급되지는 않으나, 해당 텍스쳐 유닛을 설정하는 목적 아래에서는 정수 유니폼으로 다룰 수 있도록 *OpenGL*은 제공하고 있다.
2. **쉐이더 컴파일 시점에 ** ***(binding = N)*** 으로 $ N $ 에 해당 샘플링 객체에 대한 유닛 인덱스 값을 설정한다. 이렇게 하면 어플리케이션에 직접 설정할 필요도 없을 뿐더러, 결합성이 낮아지기 때문에 유지 보수 측면에서도 좋다. 

``` c++
layout (binding = 0) uniform sampler2D foo;
layout (binding = 1) uniform sampler2D bar;
layout (binding = 3) uniform sampler2D hoge;
```

#### Texture Filtering 

텍스쳐 맵의 텍셀과 화면의 픽셀이 완전히 1 : 1 매칭이 되는 경우는 거의 없으며, 화면의 픽셀 영역에 따라 텍스쳐 맵 역시 늘어나거나 줄어들어서 지오메트리 서피스에 적용이 된다.

기존 `texture()` 쉐이더 함수는 `texelFetch()` 와는 다르게, 텍셀의 값을 **부동 소수점**으로 받고 있기 때문에, 텍셀 하나하나를 정교하게 골라서 맵핑할 수가 없다. 하지만 이 방식은 유연하기 때문에 많이 쓰이는데, 다만 늘어나거나 줄어드는 텍스쳐 맵으로부터 컬러 프래그먼트를 계산해야 한다. 이 작업을 ***Texture filtering*** 이라고 한다.

* 위에서 본, 샘플러 인자 조정 함수를 `glTexParamteri` `glSampleParameteri` 을 사용해서, 텍스쳐가 확대 될 때와 축소 될 때의 값 계산 방법을 조정할 수 있다.
  * ***확대*** 를 위한 인자는 `GL_TEXTURE_MAG_FILTER` 이며,
    ***축소*** 를 위한 인자는 `GL_TEXTURE_MIN_FILTER` 이다.
  * 조정 값으로 들어갈 기본 텍스쳐 필터링 값은 ***`GL_NEAREST` `GL_LINEAR`*** 중 하나를 택할 수 있다. 
  * 하지만, `GL_TEXTRUE_MIN_FILTER` 의 경우에는 **해당 텍스쳐가 밉맵을 생성해야** 동작하기 때문에 `glGenerateMipmap(Glenum target)` 을 사용해서 밉맵을 써줘야 한다.

#### Mipmap

* ***Mipmap*** 은 *multum in parvo* 라는 용어에서 출발한, 텍스쳐를 사용하는 객체가 여러 개 있고, 각기 크기가 다 다를 때 반짝거림 현상과 VRAM 에서의 메모리 사용량을 낮추기 위해 사용되는 텍스쳐 기법이다. 따라서, **한의 이미지만 로딩하지 않고**, **크기에 따라 여러 이미지를 사용해서 하나의 '밉맵' 텍스쳐를 로딩**한다.
  * *OpenGL* 은 일련의 필터 모드를 사용해서 해당 지오메트리 표면에 대한 적절한 밉맵 레벨 텍스쳐를 선택해 사용할 것이다.
* 만약에 밉맵을 사용하고자 할 때, 사용할 밉맵 인덱스들 $ [0, ...) $ 을 제한하고 싶다면 `glTexParamteri` 에 다음 인자를 넣어서 다음과 같이 쓴다.

``` c++
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); // 0 에서
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);  // 4 까지만 사용.
```

#### Mipmap filtering

밉맵을 활성화 시킬려면, 해당 텍스쳐의 샘플링 설정 인자 중, `GL_TEXTURE_MIN_FILTER` 및 `GL_TEXTURE_MAG_FILTER` 의 값을 **`GL_XXX_MIPMAP_YYY`** 의 값으로 바꿔야 한다. 그래야만이 기본 텍스쳐를 계속 쓰지 않고 하부 밉맵 레벨의 텍스쳐를 사용해서 효율적인 렌더링을 꾀할 수 있다.

다음은 텍스쳐 필터의 리스트이다.

| 상수                      | 설명                                                         |
| ------------------------- | ------------------------------------------------------------ |
| GL_NEAREST                | 최단 인접 필터링을 수행한다.                                 |
| GL_LINEAR                 | 선형 필터링을 수행한다 (Bilinear)                            |
| GL_NEAREST_MIPMAP_LINEAR  | 두 밉맵을 선형 보간한 뒤, 최단 인접 필터링을 수행한다.       |
| GL_NEAREST_MIPMAP_NEAREST | 가장 가까운 밉맵을 골라 최단 인접 필터링을 수행한다.         |
| GL_LINEAR_MIPMAP_NEAREST  | 가장 가까운 밉맵을 골라 선형 필터링을 수행한다.              |
| GL_LINEAR_MIPMAP_LINEAR   | 두 밉맵을 선형 보간한 뒤에 선형 필터링을 수행한다. 이 경우 Trilinear 가 된다. |

밉맵 필터는 `GL_TEXTURE_MIN_FILTER` 에만 적용할 수 있다는 것을 주의해야 한다. 또한 밉맵 사이를 선형 보간하는 인자를 쓰게 되면, 오버헤드가 매우 커질 수 있는 단점이 존재한다. 따라서 게임에서는 대개 `GL_LINEAR_MIPMAP_NEAREST` 을 쓴다.

#### Wrapping mode

* 만약 프래그먼트 쉐이더에서 Texel 의 좌표가 $ [0, 1] $ 을 벗어난 좌표를 가리키게 되면, *OpenGL* 은 샘플러 객체에 지정된 **현재 텍스쳐 래핑 모드**에 따라서 다르게 처리한다. 이 역시 `glTexParamteri` 및 `glSamplerParameteri` 을 사용해서 래핑 옵션을 조정할 수 있도록 제공하는 타입 값과 옵션 값을 사용한다.
  * 텍스쳐 래핑 모드는 **`GL_TEXTURE_WRAP_S` `GL_TEXTURE_WRAP_T` `GL_TEXTURE_WRAP_R`** 을 사용한다. $ (S, T, R) $ 은 각각 텍스쳐의 $ (x, y, z) $ 축과도 같기 때문에, GL_TEXTURE_2D 를 타깃을 하는 경우라면 $ (S, T) $ 축만이 영향을 받는다.
  * 래핑 모드의 값은 **`GL_REPEAT` `GL_MIRRORED_REPEAT` `GL_CLAMP_TO_EDGE` `GL_CLAMP_TO_BORDER`** 을 사용해서 지정한다. 
    만약 *`GL_CLAMP_TO_BORDER`* 을 사용하는 경우라면 이 때는 `glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, vec4)` 을 사용해서 끝 부분의 색상을 지정해줘야 한다.

### 5.5.6 Array Texture

* **Array texture** 기능을 사용하면, 1D 및 2D 텍스쳐의 배열과, 큐브 맵 이미지 등을 **단일 텍스쳐 객체**에 로딩할 수 있다. 밉맵과 비슷할지도 모르지만 차이점은 전체 텍스쳐 이미지 배열을 **하나의 텍스쳐 객체**에 바인딩 해서 (`GL_TEXTURE_2D_ARRAY` 등의 타깃으로) **쉐이더 안에서 각각 텍스쳐의 인덱스**를 사용하게 할 수 있다. 
  * 하지만, 3D 텍스쳐에 대한 배열은 아직 구현되지 않았다.
  * 또한 배열 텍스쳐의 텍스쳐 레이어 사이에는 필터링이 적용되지 않는다.

`GL_TEXTURE_2D` 의 배열 텍스쳐를 *OpenGL* 에 불러오기 위해서는 `glTexImage3D` 혹은 `glTexStorage3D` 을 쓸 수 있다. 그리고 2D 배열 텍스쳐에서는 $ z $ 값이 레이어의 구분을 한다.

#### 배열 텍스쳐 로딩 및 구현

다음은 `GL_TEXTURE_2D` 의 배열 텍스쳐를 불러와, 텍스쳐를 위한 버퍼를 만들고 거기에 임의의 데이터를 복사하는 코드이다.

``` c++
GLuint tex;
glGenTextures(1, &tex);
glBindTexture(GL_TEXTRUE_2D_ARRAY, tex);

glTexStorage3D(GL_TExTURE_2D_ARRAY, 8, GL_RGBA8, 256, 256, 100);
for (int i = 0; i < 100; ++i) {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, i, 256, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                   image_data[i]);
}
```

> 이를 응용함으로써 외계인 비를 구현해보는 기회를 가진다. 
> 외계인 비 코드는 Chapter5/alienrain.cc 을 참고할 것.

### 5.5.7 쉐이더에서 텍스쳐에 출력하기

*OpenGL* 은 텍스쳐 읽기 기능만 및 **쉐이더에서 텍스쳐를 읽거나 텍스쳐에 값을 쓸 수 있도록** 해준다. 하나의 **이미지 (imageXY) **변수를 사용해서 텍스쳐의 한 이미지를 대표할 수 있다.

이미지 변수 (`imageXY`) 는 샘플러 유니폼처럼 선언하는데, 샘플러가 그랬던 것처럼 이미지 역시 여러가지 타입과 Dimension 을 지정할 수 있다.

> 자세한 것은 p180 을 참고할 것.

이미지 변수를 쉐이더에서 쓰기 위해서는 다음과 같이 쓴다.

``` c++
uniform image2D my_image;
```

* ***`vec4 imageLoad(readonly Ximage2D image, ivec2 P)`***
  쉐이더 제공 함수로, 이미지로와 좌표로부터 데이터를 읽는다. 그리고 로딩시에는 필터링이 전혀 수행되지 않는다. (정수형 텍셀이기 때문) `X` 는 `i` 혹은 `u` 등의 픽셀 값의 형식이다.
* ***`void imageStore(Ximage2D image, ivec2 P, vec4 data)`***
  쉐이더 제공 함수로, 이미지의 좌표에 (정수형 좌표) `data` 값을 갱신한다.
  `X` 는 `i` 혹은 `u` 등의 픽셀 값의 형식이다.
* ***`void glBindImageTexture(GLuint unit, GLuint textrue, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)`*** 
  `image` 텍스쳐 객체에 대한 어플리케이션 단의 바인딩 함수이다. `image` 계열은 glActive 와 같은 것이 없기 때문에 **직접** 유닛에 텍스쳐, 레벨, 배열 텍스쳐의 레이어드 여부, 레이어 인덱스, 접근 모드 등을 직접 설정해줘야 한다.
  * `access` 는 **`GL_READ_ONLY` `GL_WRITE_ONLY` `GL_READ_WRITE`** 중 하나로 읽기만 하게 할지, 쓰기만 하게 할지를 결정한다.
  * `format` 은 텍스쳐 객체의 메모리 공간을 어떤 식으로 해석할지를 결정한다. 주의해야 할 점은 포맷은 해당 텍스쳐 객체의 메모리 공간을 만들 때 정한 포맷과 일치해야 한다.
    * 따라서, 쉐이더에서 `image` 을 읽을 때는 쉐이더  코드에서 ***포맷 레이아웃 지시어*** 를 사용해서 해당 이미지 포맷을 제공해야만 한다.

밑의 코드는 위의 `image` 를 활용해서 한 이미지에서 다른 이미지로 데이터의 비트를 모두 인버트하는 코드이다.

``` c++
#version 430 core
layout (binding = 0, rgba32ui) readonly uniform uimage2D image_in;
layout (binding = 1) uniform writeonly uimage2D image_out;

void main(void) {
    ivec2 P = ivec2(gl_FragCoord.xy);
    uvec4 data = imageLoad(image_in, P);
    imageStore(image_out, P, ~data);
}
```

* 따라서, ***`XimageYZ`*** 을 사용하면 쉐이더에서 이미지를 만들어 배포할 수도 있고, 이것을 재사용할 수도 있다. 또한 `imageStore` 은 픽셀 쉐이더 뿐만이 아니라 여러 쉐이더 스테이지에서도 사용할 수 있다.
  * 문제는 *OpenGL* 이 파이프라인 구조를 채택하고 있기 때문에 경쟁 상태가 유발될 수 있다는 점이다. 따라서 이를 방지하기 위해 ***`imageAtomicXYZ`*** 와 같은 원자적인 이미지 함수를 제공하고 있다.

> 이미지에 대한 어토믹 연산과, SSBO 및 Image 을 사용해서 메쉬의 X-ray 효과를 낼 수 있다.
> 이 부분은 Chapter5/xray_dragon.cc 혹은 sb6/fragmentlist.cpp 을 참고할 것.

### 5.5.8 이미지에 대한 접근 동기화하기

* 이미지에 뭔가 쓰여졌는데, 나중에 또는 다른 쉐이더에서 그 이미지를 읽어야 한다면
  ***`glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BIT);`*** 
  을 사용해서 이미지가 다 쓰인 다음에 다음 페이즈로 넘어가도록 *GPU* 에게 요청할 수 있다.

### 5.5.9 텍스쳐 압축

텍스쳐는 생각외로 어마한 공간을 차지한다. 그리고 만약에 많은 텍스쳐를 한꺼번에 GPU 에 올려야 한다고 할 때, 메모리와 GPU 버스의 대역폭 등의 외부적인 요인에 따라서 처리 속도의 저하가 발생할 수 있다. 더 좋은 부품을 사서 성능을 억지로라도 올리는 방법이 있긴 하다만, 어플리케이션은 범용적인 성능을 가진 사람들이 쓰고, 이 성능이 현재 만족하는 성능보다 많이 낮을 가능성이 있다. 따라서 많은 양의 텍스쳐를 다루는 방법 중 하나는 ***텍스쳐 압축 (Texture compression)*** 을 통해서 성능 효과를 꾀한다.

1. 압축을 하면 이미지에서 요구하는 메모리 저장 공간이 **줄어든다**.
2. **압축 텍스쳐**를 사용하면, GPU 가 압축 텍스쳐의 부분을 **Fetching** 할 때, 적은 데이터만 읽어도 되기 때문에 **상대적으로 작은 메모리 대역폭을 필요**로 하게 된다. (중요)

> 이미지 포맷의 종류는 다음 주소에서 확인할 수 있다.
> https://www.khronos.org/opengl/wiki/Image_Format

* *OpenGL* 이 지원하는 메모리 압축 텍스쳐 포맷의 타입은 다음과 같다.
  * ***RGTC*** (Red green texture compression) 포맷은 텍스쳐를 **4x4 텍셀 블록** 으로 분할하고, 일련의 코드를 사용해 블록 내에서 채널 별로 압축을 한다. 아무튼 이 포맷을 사용해서 얻을 수 있는 메모리 저장 공간의 이득은 50% 이다. 그리고 이 포맷은 1채널 및 2채널 포맷에만 적용될 수 있다.
  * ***BPTC*** (Block partitioned texture compression) 포맷은 RGTC 와 비슷하게 분할하나, 각 블록은 16 바이트의 메모리 데이터를 점유한다. 블록의 인코딩 방식은 끝점 사이의 보간 값으로 표현한다.
    BPTC 는 채널 당 8비트의, 4채널 32비트 부동소수점 데이터 형시에 적용이 가능하다.
  * *OpenGL ES 3.0* 과 같은 임베디드 시스템에서 사용되는 저대역폭 포맷인 **ETC2** 와 **EAC** 도 존재하나, 데스크탑에서는 이 포맷을 쓰게 되면 자동으로 압축 해제를 해버리기 때문에 쓰는데 유의해야 한다.
  * 또한 구현에 따라서 이외에도 많은 포맷이 존재한다. 이 경우에는 관련 확장 포맷을 지원하는 지 확인을 해야 한다.

#### 압축 사용하기

> 

### 5.5.10 Texture view

보통 텍스쳐를 사용할 때, 사용할 텍스쳐 포맷은 무엇이며, 어떤 용도로 사용할지를 알고 있어야 쉐이더에서 가져오는 데이터와 일치시켜서 사용하도록 또는 쓰도록 할 수 있다. 예를 들어서 2D 배열 텍스쳐로 부터 읽는 쉐이더는 객체 변수의 타입을 `sampelr2DArray` 로 선언을 해야 쓸 수 있다. 또한 정수 포맷 텍스쳐로부터 읽을 것은 `isampler2D` 등... 아무튼 많다.

* 하지만 ***Texture view*** 을 사용하면, 쉐이더에서 기대하는 것과 어플리케이션에서 전달하는 버퍼의 형식이 달라도 유연하게 전달하는 것이 가능하다.
  1. 텍스쳐 뷰를 특정 타입의 텍스쳐가 다른 타입의 텍스쳐인 것처럼 위장할 수 있다. 예를 들어서 `GL_TEXTURE_2D` 의 텍스쳐가 있으면, 이를 이용해 `GL_TEXTURE_2D_ARRAY` 의 2D 배열 텍스쳐로 위장시킬 수 있다.
  2. 텍스쳐의 데이터의 내부 형식이 달라도, 이를 사용해서 다른 형식인 것처럼 조정할 수 있다. 예를 들어서 `GL_RGBA32F` 인 텍스쳐 데이터를 `GLRGBA32UI` 인 것처럼 보이게 할 수 있다. (물론 이렇게 해서 불러오는 값은 장담을 못한다)
  3. 그 외의 많은 것들이 있다...

#### 텍스쳐 뷰 만들기

* ***`glTextureView(dst_texture, target, orig_textrue, infernal_format, min_lv, num_lv, min_layer, num_layer)`*** 
  을 사용해서 텍스쳐 뷰를 만들 수 있다. 하지만 텍스쳐 뷰를 만들기 전에, 모든 타입이 다른 모든 타입으로 변환되지는 않는다. 각 타입마다 변환할 수 있는 **타깃 호환성** 이 존재한다.

  * `internalforamt` 인자는 새로운 텍스쳐 뷰에 대한 내부 포맷을 지정한다. 이 값 역시 텍스쳐의 내부 포맷과 호환이 되어야 하며 그렇지 않으면 실패한다. 
    * ***포맷 클래스*** 가 맞아야 한다.
  * `level` 변수들은, 만약에 텍스쳐가 밉맵을 가지고 있을 때 이 밉맵 중 어느 것들을 가져와서 텍스쳐 뷰를 만들지 개수를 지정한다.
  * `layer` 는 배열 텍스쳐의 경우에 레이어 일부에 대한 뷰를 생성할 때 사용한다.

  > 자세한 사항은 밑의 주소에서 설명하고 있다.
  > https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTextureView.xhtml 

따라서, 위와 같이 텍스쳐 뷰를 생성했으면 새로운 타입의 텍스쳐처럼 사용할 수 있다. 하지만 깊은 복사는 아니고 얉은 복사이기 때문에 텍스쳐 뷰를 이용해 값을 변경하면, 원본 텍스쳐의 뷰가 해당되는 범위 역시 변경된다.

