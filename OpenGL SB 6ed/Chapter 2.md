# Chapter 2

## 2.1 Simple application

``` c++
#include <sb6.h>
class MyApplication final : public sb6::application {
public:
    void render(double currentTime) override {
        static const GLfloat red[] { 1.f, 0.f, 0.f, 1.f };
        glClearBufferfv(GL_COLOR, 0, red);
    }
};
DECLARE_MAIN(MyApplication);
```

* **`glClearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value)`** 
  첫 번째 인자를 **세 번째 인자의 값으로 Clear** 한다. 두 번째 인자는 지울 출력 버퍼가 여러 개 일때 사용할 수 있다.
  * 모든 *OpenGL* 함수는 ***gl*** 로 시작하며, 해당 함수의 일부 인자 타입을 이름 끝의 ***접미사***로 줄여 쓰는 컨벤션을 가진다. 따라서 제한된 형태의 오버로딩이 가능하다. 위 함수에서는 *float* 값을 가지는 *vector(배열)* 를 사용한다는 의미이다.

## 2.2 Using shader

* *OpenGL* 은 **Shader** 라고 불리는 그래픽 연산에 필요한 프로그램을, 고정 함수 블록에 연결시켜서 사용한다. 그래픽스 시스템은 파이프라인에 부착된 쉐이더 프로그램을 통해 데이터를 연산해서 최종 픽셀이 스크린에 그려지게 한다. (하지 않을 수도 있다)
* *OpenGL Shader* 는 **GLSL** 라는 쉐이딩 언어로 작성된다. 각각의 쉐이더 코드는 컴파일이 되면서 ***Shader object*** 로 바뀌며, 객체들이 연결되서 ***Shader program object*** 로 완전한 쉐이더 프로그램이 된다.
  * 하나의 프로그램 객체는, 여러 단계의 쉐이더 스테이지로 나뉜다. 5개로 줄여서 말하면, ***Vertex***, ***Tesselation Control***, ***Tesselation Evaluation***, ***Geometry***, ***Pixel (Fragment)*** 로 나뉜다. 그리고 최근에 들어서 추가된 ***Compute*** 쉐이더가 있다. 이 쉐이더는 병렬 프로그래밍과 같은 렌더링이 아닌 연산에 사용될 수 있다.
  * 파이프라인은 적어도 하나의 쉐이더를 포함시켜야 하는데, 화면에 픽셀을 그리기 위해서는 필수적으로 *Fragment shader* 가 필요하다.

버텍스 쉐이더와 픽셀 쉐이더의 코드는 다음과 같다.

``` c
const char vs[] =
"#version 430 core\n"
"void main() {\n"
"gl_Position = vec4(0.f, 0.f, .5f, 1.f); }";

const char fs[] =
"#version 430 core\n"
"out vec4 color;\n"
"void main(void) {\n"
"color = vec4(0.f, .8f, 1.f, 1.f); }";
```

* 각각의 쉐이더는 `gl_` 로 시작하는 ***내부 변수***를 가진다. 이 내부 변수는 각 스테이지 혹은 고정 함수의 기능과 연결되어 있다. 예를 들어서 `gl_Position` 은 현재 처리하고 있는 정점의 ***Clip space*** 에서의 출력 위치를 가진다.

프로그램에 쓸 쉐이더의 코드가 준비되었으면, 이를 컴파일시켜서 링크해야 한다. *OpenGL* 의 쉐이더 프로그램은 런타임에 컴파일 및 빌드가 된다.

코드는 다음과 같다.

``` c++
GLuint CompileShader() {
    auto vs_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_id, 1, vs, nullptr);
    glCompileShader(vs_id);

    auto fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_id, 1, fs, nullptr);
    glCompileShader(fs_id);

    auto program = glCreateProgram();
    glAttachShader(program, vs_id);
    glAttachShader(program, fs_id);
    glLinkProgram(program);

    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    return program;
}
```

새로운 함수는 다음과 같다.

1. ***`glCreateShader(GLenum shader_mode)`***
   빈 쉐이더 객체를 생성해서, 쉐이더 객체의 *id* 를 반환한다.


2. ***`glShaderSource(GLuint program, uint number, char_ptrptr code, uint length)`***
   쉐이더 소스 코드를 객체로 전달해서 **복사본**을 유지한다.
3. ***`glCompileShader(GLuint program)`*** 
   해당 쉐이더 객체를 컴파일한다.
4. ***`glCreateProgram()`*** 
   쉐이더 프로그램 객체를 생성한다.
5. ***`glAttachShader(GLuint program, GLuint shader_object)`***
   쉐이더 객체를 쉐이더 프로그램 객체에 붙인다.
6. ***`glLinkProgram(GLuint program)`*** 
   프로그램 객체에 어태치된 모든 쉐이더 객체를 링크한다.
7. ***`glDeleteShader(GLuint shader_object)`*** 
   쉐이더 객체를 삭제한다. 쉐이더가 프로그램에 링크가 되면, 프로그램이 **바이너리 코드를 보관하기 때문에** 원본의 쉐이더는 더 이상 필요없게 된다.

따라서 쉐이더 프로그램을 만들 때는, 각각의 쉐이더 객체를 만들고, 객체에 코드를 가져와 복사를 한 뒤에 컴파일을 해야 한다. 그 후에 에러가 없다면 쉐이더 프로그램 객체를 만들어서 거기에 링크하고 싶은 쉐이더 객체를 바인딩을 해서 링크를 시켜야 한다. 그 후에 메모리 낭비를 막기 위해 원본 쉐이더 객체는 삭제를 시키는 것이 좋다.

* 마지막을 해야할 일은, ***Vertex Array Object*** 을 생성해서 바인딩을 해야한다. ***VAO*** 는 파이프라인의 첫 번째 단계인 ***Vertex fetch*** 에서 전달할 데이터를 나타내는 객체이며, 객체에 저장된 데이터는 정점 쉐이더에 공급하기 위해 사용된다.
  * 현재 파이프라인에 넘겨줄 정점이 없다고 하더라도, 빈 VAO 을 생성해서 OpenGL 이 그릴 수 있도록 해야한다.
    **`glGenVertexArrays(size_t number, unsigned_ptr vao_ptr)`** 을 사용해서 VAO 을 만든다. 그리고 **`glBindVertexArray(GLuint array)`** 을 호출해서 VAO 을 바인딩한다.
  * *VAO* 는 *OpenGL* 파이프라인의 입력과 관련된 **모든 정보**를 관리한다.

8. ***`glUseProgram(GLuint program)`***
   쉐이더 프로그램 객체를 사용해서 특정 객체를 렌더링시킨다. 

* 그 후에 ***`glDrawArrays(GLenum primitive_type, GLint first, GLsizei count)`*** 을 이용해서 드로우를 명령한 뒤, 실행시키면 점이 보이는 것을 확인할 수 있다. 이 함수는 VAO 의 정점들을 OpenGL 파이프라인에 보내는 역할을 한다. 그러면 **각 버텍스에 대해** 정점 쉐이더가 수행된다.
  * 첫번째 인자는 그래픽스 프리미티브의 타입을 결정한다.
  * 만약에 `GL_POINTS` 을 사용했을 경우, 픽셀의 사이즈를 크게 할 수도 있다.
    이 경우는 ***`glPointSize(GLfloat size)`*** 을 사용한다.

## 2.3 Triangle

* *`glDrawArrays()`* 의 프리미티브 타입에 **GL_LINES** 혹은 **GL_TRIANGLES** 을 전달할 수도 있다. 하지만 그럴려면 버텍스 쉐이더가 그릴 각각의 정점의 위치가 달라서 **최종 면적이 0**이 되면 안된다.
  * 위의 버텍스 쉐이더는 모든 버텍스를 항상 같은 위치에 배치하기 때문에, 위 쉐이더 그대로 그릴려고 하면 출력이 되지 않는다. (면적이 0이 되기 때문에)
* 만약에 VAO 에 임의 정점 배열을 넣지 않고, 하드코딩으로 무언가를 렌더링하고 싶을 때는, 정점 쉐이더의 내부 변수인 **`gl_VertexID`** 을 사용하면 된다. 이 입력값은 *`glDrawArrays()`* 에 입력으로 들어간 초기 인자값부터 시작해서 *count* 만큼의 정점까지 정점별로 하나씩 증가한다. 

다음은 `gl_VertexID` 을 사용해서 삼각형을 그리는 정점 쉐이더의 코드이다.

``` c
const GLchar* vs[] = {
    "#version 430 core \n"
    "\n"
    "void main() { \n"
    "const vec4 vertices[3] = vec4[3]( vec4(0.25f, -0.25f, 0.f, 1.0f), "
    "vec4(-0.25f, -0.25f, 0.f, 1.0f), vec4(0.25f, 0.25f, 0.f, 1.0f) );\n"
    "gl_Position = vertices[gl_VertexID]; \n" 
    "}"
};
```

그리고 `glDrawArrays()` 을 사용해서 렌더링할 때는 프리미티브 타입을 **`GL_TRIANGLES`** 로 사용한다.

``` c++
glUseProgram(m_rendering_program);
glDrawArrays(GL_TRIANGLES, 0, 3);
```

