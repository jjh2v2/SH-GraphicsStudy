# Chapter 9 :: Fragment Processing & Framebuffer

## 9.1 Fragment Shader

쉐이더 코드가 각 프래그먼트의 색상을 결정하는 파이프라인 내 스테이지로, *FrameBuffer* 에 Composition (합성) 이 되기 전에 처리되는 단계이다. 각 프래그먼트 쉐이더에 대한 입력은 *Rasterizer* 에서 래스터화 되는 프리미티브들에 걸쳐서 선형적으로 부드럽게 보간된다. 이 때, *Rasterizer* 에서 *고정 함수* 에 의해 보간이 수행 될 때, ***보간을 어떻게 수행하고 심지어 수행할지 말지 여부를 결정할 수도 있다.***

### 보간 비활성화 시키기

기본적으로는 프리미티브에서 래스터화를 걸쳐서 입력되는 모든 멤버가 선형 보간이 되지만, **정수형 처럼 보간이 되지 말아야 하는 경우가 있다**. OpenGL 에서는 정수를 보간할 수 없기 때문에 정수에 대해서는 보간이 비활성화된다.

부동소수점 프래그먼트 쉐이더 입력의 보간을 명시적으로 비활성화 시킬 수도 있다. 보간이 **비활성화**된 프래그먼트 쉐이더 입력은 ***Flat input*** 이라고도 한다. (보통 쉐이더 입력의 보간이 적용된 것은 *Smooth input* 이라고 한다) 프래그먼트 쉐이더에 보간이 수행되지 않게 할려면 쉐이더 `in` 혹은 `varying` 앞에 ***`flat`*** 지시어를 붙인다.

``` glsl
flat in vec4 foo;
flat in int bar;
flat in mat3 baz;
```

반대로 명시적으로 선형 보간을 수행할려면 *`smooth`* 을 앞에 붙인다. 이 지시어들은 입력 블록`{}`에 대해서도 보간 지시어를 사용할 수 있으며, 입력 블록의 멤버 변수 각각 지시어를 사용해서 보간할 지 말지를 결정할 수 있다고 한다.

``` c++
flat in INPUT_BLOCK {
    vec4 foo;
    int bar4;
    smooth mat3 baz;
} // INPUT_BLOCK 의, baz 을 제외한 모든 멤버는 보간을 수행하지 않는다
```

여기서 이전 단계의 쉐이더 코드에서 선형 보간을 수행하지 않을 모든 출력 멤버에 대해서도 `flat` 지시어를 사용해서 명시적으로 보간하지 않음을 설정해줘야 한다.

만약 프래그먼트의 멤버가 보간이 되지 않을 때, 해당 `flat` 멤버의 값은 프리미티브를 구성하는 버텍스의 값 중 하나에서 올 수 있다. 문제는 프리미티브를 구성하는 정점의 개수가 2 개 이상이라면, 해당 정점들 중 첫번째 정점 혹은 마지막 정점 중 하나가 값을 가져올 정점으로 선택이 된다. 이를 ***Provoking vertex*** 이라고 한다.

*Provoking Vertex* 는 다음 함수와 열거형을 사용해서 첫번째 혹은 마지막 정점을 가져올 지 선택할 수 있다.

``` c++
void glProvokingVertex(GL_FIRST_VERTEX_CONVENTION | GL_LAST_VERTEX_CONVENTION);
```

## 9.2 Test for fragments

OpenGL 은 프래그먼트에 많은 테스트를 수행해서 실제 프레임버퍼에 써질지 말지, 혹은 어떻게 써 질지를 결정할 수 있다.

### 9.2.1 Scissor Test

#### A. Functions

*Viewport* 는 지오메트리를 직접 프론트엔드에서 클리핑하지만, ***Scissor Rectangle*** 은 Rasterization 이후에 따라올 처리의 일부로서 개별 프래그먼트를 가위 사각형에 대해 테스트한다.

이 때, OpenGL 은 가위 사각형들을 여러 개 설정할 수 있고, 배열을 지원한다. *SR* 을 설정하려면 다음과 같은 함수를 사용한다.

``` c++
void glScissorIndexed(index, left, bottom, width, height);
void glScissorIndexedv(index, const GLint* v);
```

두 함수에 대해 `index` 인자는 어떤 가위 사각형을 변경할 지 지정한다. `left` `bottom` `width` `height` 인자는 가위 사각형을 정의하는 윈도우 좌표 영역을 지정한다. 이 때 가위 테스트를 할 때는, 뷰포트의 영역에 있을 수 있도록 해야한다.

가위 사각형에 대해 선택하려면, **지오메트리 쉐이더의 gl_ViewportIndex** 내장 출력 변수를 사용한다.  (자세한 것은 코드를 보는 것이 좋을 듯) 가위 테스트를 활성화하려면 아래 함수를 호출한다.

``` c++
glEnable(GL_SCISSOR_TEST);
glDisable(GL_SCISSOR_TEST);
```

가위 테스트는 기본적으로 비활성화 되어 있고, 버퍼를 초기화 할 때도 가위 테스트는 비활성화 한 후에 하는 것이 좋다.

가위 테스트를 수행 시에, *Geometry shader* 에서는 `gl_InvocationID` 가 바인딩된 가위 테스트의 인덱스에 대해 순회되면서 호출된다. 만약 가위 사각형이 $ N $ 개 바인딩 되어 있으면, $ 0 $ 에서 $ N - 1 $ 까지 변수의 값이 증가한다.

#### B. Example

> Chapter9/_921_scissor.cc 을 참고한다.

#### SetupShaderProgram()

Scissor Test 에서는, *Geometry Shader* 의 `gl_InvocationID` 와 `gl_ViewportIndex` 을 사용해야 하므로, 지오메트리 쉐이더를 쉐이더에 링크시켜야 한다.

``` c++
constexpr const char* k_vs_path { "Chapter9/_921.vert" };
constexpr const char* k_gs_path { "Chapter9/_921.geom" };
constexpr const char* k_fs_path { "Chapter9/_921.frag" };

auto vert_shader { sb6::shader::load(k_vs_path, GL_VERTEX_SHADER) };
auto geom_shader { sb6::shader::load(k_gs_path, GL_GEOMETRY_SHADER) };
auto frag_shader { sb6::shader::load(k_fs_path, GL_FRAGMENT_SHADER) };
```

#### SetupBuffers()

*Geometry Shader* 코드의 내부를 보면, `gl_invocationID` 의 값으로 각각의 가위 사각형에 대해 시점이 다른 행렬을 곱할려고 하고 있는데, 4 개의 가위 사각형의 행렬이 들어가 있는 버퍼가 *Uniform buffer* 이다.

``` glsl
for (int i = 0; i < gl_in.length(); i++) {                                                
    gs_out.color = gs_in[i].color; 
    gl_Position = mvp_matrix[gl_InvocationID] * gl_in[i].gl_Position;          
    gl_ViewportIndex = gl_InvocationID; 
    EmitVertex();                                
} 
EndPrimitive();
```

이 때 mvp_matrix 유니폼 버퍼의 공간을 만들어주기 위해 버퍼를 바인딩하고 매 프레임마다 값을 변경하므로 (큐브의 모델 매트릭스까지 포함되어 있음) `GL_DYNAMIC_DRAW` 를 사용한다.

``` c++
glGenBuffers(1, &m_uniform_buffer);
glBindBuffer(GL_UNIFORM_BUFFER, m_uniform_buffer);
glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(vmath::mat4), nullptr, GL_DYNAMIC_DRAW);
```

#### render(double current_time)

화면 전체의 값을 초기화 할 때는 `glDisable(GL_SCISSOR_TEST)` 을 호출한 후에 버퍼를 초기화 해야한다. 만약 가위 테스트가 활성화된 상태로 초기화 하면, 버퍼 초기화가 제대로 작동하질 않는다.

그리고 본 렌더링을 실시하기 전에, `glEnable(GL_SCISSOR_TEST)` 으로 가위 테스트를 활성화시켜야 한다. 그렇지 않으면 각각의 회전하고 있는 정육면체 메쉬가 겹쳐보이는 현상이 일어나버린다. (가위 테스트가 먹히질 않는다)

그리고 Rasterization 후에 클리핑 처리될 *Scissor Rectangle* 을 4 개 만든다.

``` c++
// Set scissor rectangle onto viewport buffer.
glScissorIndexed(0, 0, 0, scissor_width, scissor_height);
glScissorIndexed(1, info.windowWidth - scissor_width, 0, scissor_width, scissor_height);
glScissorIndexed(2, 0, info.windowHeight - scissor_height, scissor_width, scissor_height);
glScissorIndexed(3, info.windowWidth - scissor_width, info.windowHeight - scissor_height, 
                 scissor_width, scissor_height);
```

마지막으로 매트릭스를 갱신하고 최종 렌더링한다. 매트릭스를 갱신할 때는 `UpdateProjectionMatrix` 멤버 함수를 사용한다. 여기서는 메모리에 얹혀진 *Uniform Buffer* 의 주소를 가져와서 값을 맵핑하는 식으로 매트릭스를 갱신한다.

``` c++
glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniform_buffer);
// Map projection * view matrix for four scissored screen.
auto mv_matrix_array{ 
    static_cast<vmath::mat4*>(
        glMapBufferRange(GL_UNIFORM_BUFFER, 0, 4 * sizeof(vmath::mat4),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
    )
};

for (auto i = 0; i < 4; i++) {
    mv_matrix_array[i] = proj_matrix *
        vmath::translate(0.0f, 0.0f, -2.0f) *
        vmath::rotate((float)current_time * 45.0f * (float)(i + 1), 0.0f, 1.0f, 0.0f) *
        vmath::rotate((float)current_time * 81.0f * (float)(i + 1), 1.0f, 0.0f, 0.0f);
}

glUnmapBuffer(GL_UNIFORM_BUFFER);
```

결과는 다음과 같다.

![_921](..\Results\OpenGL_sb6\_921.gif)

