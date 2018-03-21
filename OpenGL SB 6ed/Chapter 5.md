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

***UBO*** 는 임시 상태와 고정 상태를 분리할 때 유용하게 사용될 수 있다.

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

## 5.3 Shader storage block

