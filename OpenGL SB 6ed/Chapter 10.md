# Chapter 10 : Compute Shader

***Compute Shader*** 는 CUDA 와 비슷하게 GPU 의 구조 특성을 전면적으로 활용하여 수 많은 데이터들을 **동시에 처리** 할 수 있도록 하는 여러 병렬 그룹 (***wrap***) 을 통해서 원하는 처리를 GPU 에서 하고자 할 때 쓰일 수 있다. 컴퓨트 쉐이더는 **wrap** 간에 서로 **동기화가 가능하고** 일반 계산을 더 쉽게 할 수 있도록 데이터를 공유하게 할 수도 있다.

컴퓨트 쉐이더는 OpenGL 의 일반 렌더링 파이프라인과는 약간 다른 구조를 가진다.

## 10.1 Using Compute shader

컴퓨트 쉐이더는 GLSL 로 작성하고, 다른 쉐이더와 같이 쉐이더 객체로 표현되며 쉐이더 프로그램이 필요하고 컴파일되고 바인딩이 된다. 하지만 `glCreateShader` 을 사용할 때 ***`GL_COMPUTE_SHADER`*** 로 지정을 해야 한다. 

* 컴퓨트 쉐이더는 **다른 타입의 쉐이더와 같이 사용될 수 없다.**

만약 다른 쉐이더와 같이 링크를 하게 되면 링크가 실패할 것이다. 링크된 프로그램 객체가 컴퓨트 쉐이더만 참조하는 경우에는 *Compute Program*, 그렇지 않은 경우에는 *Graphics Program* 이라고 부른다.

* 컴퓨트 프로그램은 그래픽스와 동일하게 **Uniform, Uniform block, Uniform buffer, storage** 등을 사용할 수 있다.

### 10.1.1 컴퓨트 쉐이더 수행하기

#### A. Functions

컴퓨트 프로그램을 현재 프로그램으로 설정하고 사용할 준비가 되었다면 다음 함수를 사용해서 수행하면 된다.

```  c++
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
```

* launch one or more compute work groups.
  Each work group is processed by the active program object for the compute shader stage. While the individual shader invocations within a work group are executed as a unit, work groups are executed completely independently and in unspecified order.

``` c++
void glDrawArraysIndirect(GLintptr indirect);
```

* 이 함수는 위의 `glDispatchCompute` 와 비슷하지만, 위 함수에서 전달할 일련의 인자들을 포함하는 버퍼 객체에 대한 오프셋? 으로 해석된다고 한다.

  * `indirect` : ` GL_DISPATCH_INDIRECT_BUFFER ` 로 바인딩된 3 차원 버퍼 객체의 ID. 버퍼의 구조체는 다음과 같아야 한다.ㅡ

    ``` c++
    typedef  struct {
            uint  num_groups_x;
            uint  num_groups_y;
            uint  num_groups_z;
        } DispatchIndirectCommand;
    ```

#### B. Global and Local work group

컴퓨트 쉐이더는 **Work group** 이라고 불리는 단위 별로 처리를 수행한다. 위 함수를 호출하면 **하나의 Global work group** 이 OpenGL 로 보내지며 *전역 그룹* 은 다시 *지역 작업 그룹* 으로 나뉘어 진다.

작업 그룹은 기본적으로 **work-item** (작업 아이템) 의 3D 블록으로 구성되며, 각 *work-item* 은 코드를 수행하는 컴퓨트 쉐이더의 호출에 의해 처리된다. 각 *지역 워크 그룹* 의 $ x, y, z $ 차원 크기는 쉐이더 소스 코드에서 **레이아웃 지시어**로 설정이 가능하다.

``` c++
layout (local_size_x = 4, local_size_y = 4, local_size_z = 10) in;
```

이 때 한 작업 그룹의 최대 크기는 `GL_MAX_COMPUTE_WORK_GROUP_SIZE` 을 통해서 질의가 가능하며, `glGetIntegeri_v()` 의 타깃 인자로 설정해서 질의할 수 있다.

OpenGL 의 사양서에 따르면 $ x $ 및 $ y $ 차원에 대해서는 1024 개 이상, $ z $ 차원에 대해서는 최소 64 개 이상일 것이다. 또는 `GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS` 로 질의하면 작업 그룹의 최대 요소의 개수를 얻을 수 있다.

프로그램의 컴퓨트 쉐이더 지역 작업 그룹 크기는 `glGetProgramiv(program, GL_COMPUTE_WORKGROUP_SIZE, size)` 로 얻을 수 있다. `size` 는 3 개의 int 값을 갖는 C 배열이다.

#### C. 컴퓨트 쉐이더 입력 및 출력

* 컴퓨트 쉐이더는 **내장 출력이 없다**.

하지만 다른 그래픽스 쉐이더와 마찬가지로 몇몇개의 **내장 변수**가 존재한다.

1. `gl_LocalInvocationID` : 지역 작업 그룹 내의 쉐이더 호출의 인덱스이다. `uvec3` 타입이며 zero-based count 방식이다.
2. `gl_WorkGroupSize` : 지역 작업 그룹 크기를 가진다. `uvec3` 타입이다.
3. `gl_NumWorkGroups` : 지역 작업 그룹의 총 개수를 가진다. `uvec3` 이다.
4. `gl_WorkGroupID` : 현재 쉐이더에서 돌리고 있는 작업 그룹의 인덱스를 나타낸다. 이 역시 `uvec3` 이다.
5. `gl_GlobalInvocationID` : `gl_WorkGroupID` * `gl_WorkGroupSize` + `gl_LocalInvocationID`
6. `gl_LocalInvocationIndex` : 지역 작업 그룹 내에서 3D 의 작업 그룹 인덱스를 1D 로 펼친 꼴이다.

* 컴퓨트 쉐이더는 내장 출력이 없지만, fragment shader 와 같은 고정 출력이 없다는 의미이고, **임의의 쉐이더 스토리지, 이미지 함수, 카운터 값의 증가 등은 가능하다.** 하지만 이런 연산은 외부 메모리의 내용을 변경시키거나 하는 결과를 낳기 때문에 **Side effect** 를 동반한다고 할 수 있다.

다음 컴퓨트 쉐이더는 이미지를 읽어서 데이터를 역으로 변환하고 다시 이미지에 쓰는 프로그램이다.

``` glsl
#version 430 core
local (local_size_x = 32, local_size_y = 32) in;
layout (binding = 0, rgba32f) uniform image2D img_input;
layout (bidning = 1) uniform image2D img_output;

void main() {
    vec4 texel;
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    texel = imageLoad(img_input, p);
    texel = vec4(1) - texel;
    imageStore(img_output, p, texel);
}
```

``` c++
glBindImageTexture(0, texel_input, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
glBindImageTexture(1, tex_ouput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
glDispatchCompute(IMAGE_WIDTH / 32, IMAGE_HEIGHT / 32, 1);
```

### 10.1.2 Communication

*TCS* 가 `patch` 스토리지 지시어를 사용해서 변수를 선언한 것 처럼, *CS* 에서는 ***`shared`*** 스토리지 지시어를 사용해서 변수를 선언할 수 있다. 이 지시어를 사용하면 동일한 로컬 작업 그룹 내의 컴퓨트 쉐이더 호출들 간에 변수를 공유할 수 있게 된다. (공유 메모리에 바인딩이 되는 것인지는 잘 모르겠다)

`shared` 공유 변수에 접근하는 것은 VRAM 전역 메모리에 접근하는 것 보다 매우 빠르다. 하지만 공유 변수가 올라가는 공유 메모리에는 용량 제한이 있기 때문에, 많은 변수들을 올릴 수 없다. OpenGL 에서 지원하는 최소 공유 메모리의 양은 32KiB 이므로 주의해야 한다.

컴퓨트 쉐이더에서 사용할 수 있는 공유 메모리 양은 다음 함수로 호출이 가능하다.

``` c++
glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
```

#### A. Barrier synchronization

GPU 의 여러 프로세서들은 각 지역 그룹을 작은 *chunk* 로 분할하고, 단일 청크 내의 호출들을 동시에, 동기적으로 실행한다. 이 청크들 사이에는 비동기적이며 순서없이 하지만 효율적으로 수행됨을 보장한다.

그러나 이 청크들 사이에 **커뮤니케이션을 하는 방법** 역시 보장해야 한다. 메시지를 받는 청크가 그것이 가장 최근의 메시지임을 보장할 방법 또한 있어야 한다. 이것을 보장해주는 것이 바로 ***Barrier*** 라고 하는 개념이다. (CUDA 에서도 많이 봐왔다)

*Barrier* 는 컴퓨트 쉐이더 내에서 다음 함수를 사용해서 호출할 수 있다.

> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/barrier.xhtml

``` c++
barrier();
```



## 10.2 예제들

### 10.2.1 Parallel prefix sum (병렬 접두합)

*Parallel prefix sum* 은 입력값들의 배열이 주어졌을 때, 현재 배열 요소까지의 모든 입력 배열 값의 합을 각 출력 배열 요소에 저장하는 새로운 배열을 계산하는 알고리즘이다. 여기서 구현할 *prefix sum* 은 ***Inclusive*** 한 접두합이다. *Inclusive* 하다는 것은, 현재 요소를 결과에 포함하는 접두합 연산을 말한다.

![ipps](http://williamrjribeiro.com/wp-content/uploads/2016/07/prefix-sum.jpg)

이 접두합은 Sequential 하게 한다면 $ N $ 개의 요소가 있을 때 $ O(N) $ 의 시간이 걸리지만, 병렬 처리의 힘을 빌리면 $ O(\lg{N})$ 까지 줄일 수 있게 된다. 만일 $ N = 100,000,000 $ 이라면 병렬 처리를 사용하면 약 27 번의 연산으로 결과를 도출할 수 있게 된다.

![ppf](http://apprize.info/programming/opengl_1/opengl_1.files/image207.jpg)

#### A. 예제 코드

위 Inclusive Parallel Prefix Sum 을 구현한 컴퓨트 예제이다.

``` c++
#version 430 core
layout (local_size_x = 1024) in;
layout (binding = 0) coherent buffer block1 {
    float input_data[gl_WorkGroupSize.x];
}
layout (binding = 1) coherent buffer block2 {
    float output_data[gl_WorkGroupSize.x];
}
shared float shared_data[gl_WorkGroupSize.x * 2];

void main(void) {
    uint id = gl_LocalInvocationID.x;
    uint rd_id, wr_id, mask;
    
    const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
    uint step = 0;
    
    shared_data[id << 1] = input_data[id << 1];
    shared_data[id << 1 + 1] = input_data[id << 1 + 1];
    
    barrier();
    memoryBarrierShared();
    
    for (auto step = 0; step < steps; ++step) {
        mask = (1 << step) - 1;
        rd_id = ((id >> step) << (step << 1)) + mask;
        wr_id = rd_id + 1 + (id & mask);
        shared_data[wr_id] += shared_data[rd_id];
        
        barrier();
        memoryBarrierShared();
    }
    
    output_data[id << 1] = shared_data[id << 1];
    output_data[id << 1 + 1] = shared_data[id << 1 + 1];
}
```

이 코드에서 주의해야 할 점은 데이터의 크기 혹은 지역 작업 그룹의 크기가 $ 2^p $ 이어야 한다는 것이다. 그렇지 않으면 *prefix sum* 의 결과 값이 제대로 나오지 않을 것이다. 

또한 barrier() 말고도 `memoryBarrierShared()` 하는 함수가 있다.

* [**`void memoryBarrierShared(void);`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/memoryBarrierShared.xhtml)

  memoryBarrierShared waits on the completion of all memory accesses resulting from the use of shared variables and then returns with no other effect. When this function returns, the results of any modifications to the content of shared variables will be visible to any access to the same buffer from other shader invocations. In particular, any modifications made in one shader stage are guaranteed to be visible to accesses performed by shader invocations in subsequent stages when those invocations were triggered by the execution of the original shader invocation (e.g., fragment shader invocations for a primitive resulting from a particular geometry shader invocation).

  4.30 부터 제공되는 함수...

*Parallel Prefix sum* 알고리즘은 **다차원 데이터 집합** 에 대해서도 잘 먹힌다. 만약 2 차원 이미지에 대해서 접두합을 수행하게 할려면, 다음과 같이 하면 될 것이다.

1. 알고리즘을 이미지의 **픽셀 행**에 대해 수행한다.
2. 그 결과를 다시 불러와서 **픽셀 열**에 대해 수행한다. 이 경우 쉐이더 분리 모델을 사용해서 텍셀을 도치하는 식으로 알고리즘을 적용하면 될 것이다.

![d2d_ppf](http://apprize.info/programming/opengl_1/opengl_1.files/image208.jpg)

이러한 원본 이미지의 2D 접두합을 담는 가공된 이미지는 [***Summed-area table***](https://en.wikipedia.org/wiki/Summed-area_table) 이라고 하며 컴퓨터 그래픽스 분야에서 매우 중요한 자료 구조 중 하나이다.

#### B. 이미지 활용

이제 쉐이더 스토리지 대신에 이미지 에 대한 접두합을 계산해본다. 이 경우에는 이미지의 행을 읽어서 **이미지의 열**에 출력하도록 한다. 그래서, 입력 이미지와 출력 이미지가 전치가 되는데 쉐이덛를 두 번 적용하면 모든 행과 열에 대해서 *PPS* 가 적용된 결과를 얻을 수 있다.

``` c++
#version 430 core
layout (local_size_x = 1024) in;
layout (binding = 0, r32f) readonly uniform image2D input_image;
lyaout (binding = 1, r32f) writeonly uniform image2D output_image;
shared float shared_data[gl_WorkGroupSize.x * 2];

void main(void) {
    uint id = gl_LocalInvocationID.x;
    uint rd_id, wr_id, mask;
    ivec2 P = ivec2(id << 1, gl_WorkGroupID.x);
    
    const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
    uint step = 0;
    
    shared_data[id << 1] = imageLoad(input_image, P).r;
    shared_data[id << 1 + 1] = imageLoad(input_image, P + ivec2(1, 0)).r;
    
    barrier(); memoryBarrierShared();
    
    for (auto step = 0; step < steps; ++step) {
        mask = (1 << step) - 1;
        rd_id = ((id >> step) << (step << 1)) + mask;
        wr_id = rd_id + 1 + (id & mask);
        shared_data[wr_id] += shared_data[rd_id];
        
        barrier();
        memoryBarrierShared();
    }
    
    imageStore(output_image, P.yx, vec4(shared_data[id << 1]));
    imageStore(output_iamge, P.yx + ivec2(0, 1), vec4(shared_data[id << 1 + 1]));
}
```

이미지의 가공된 *Summed-area table* 이 주어지면 그 이미지의 임의의 사각형 내의 요소들의 합을 계산할 수 있게 된다. 이를 위해서는 사각형 내의 가장자리의 네 값 혹은 해당 영역의 최종 합 결과만 필요하다.  (주의해야할 점은 임의 사각형 영역의 합을 구하기 위해서 도중에 더하고 뺄 사각형의 시작 위치는 처음 위치여야 한다)

![summed_area_table](http://apprize.info/programming/opengl_1/opengl_1.files/image209.jpg)

그리고 사각형 내에 포함된 **모든 요소의 합을 취해서 해당 사각형의 면적 (총 요소 개수)로 나누면** 사각형 내 요소들의 **평균값**이 된다. 이를 [**box filter**](https://en.wikipedia.org/wiki/Box_blur) 라고 하며 자주 쓰이는 적용 사례가 **Gaussian blur** 이다.

특히나 이미지 안의 임의 점을 기반으로 해서 주변 특정 개수의 픽셀에 대한 평균을 구할 때는 가변 크기 필터를 사용할 수 있다. **가변 크기 필터** 란 픽셀 별로 필터링 사각형의 차원 (?) 혹은 영역을 변경할 수 있는 필터를 말한다.

이런 기법을 사용해서 적용할 수 있는 것이 ***Depth of field (피사계 심도)*** 효과이다. 

#### C. [***Depth of Field***](https://en.wikipedia.org/wiki/Depth_of_field)

*피사계 심도* 는 사진학 및 광학에서 도입된 용어인데, 카메라는 **초점 거리** 와 **초점 깊이** 를 가지고 있어서 이 초점이 맞지 않으면 초점이 맞는 물체를 제외한 나머지 물체가 흐려지는 것을 말한다. 초점 거리는 카메라로부터 완벽히 초점이 맞아야 하는 물체 간의 거리를 말한다. 초점 깊이는 이 초점에서 위치가 맞지 않는 물체로 초점 상태의 정도가 변화하는 비율을 말한다.

![dof](http://www.conceptonestudios.com/wp-content/uploads/2016/12/8643193_orig.jpg)

피사계 심도를 시뮬레이션 할며녀, 다음과 같은 단계를 거친다.

1. 장면을 보통 때와 같이 렌더링한다. 이 때 **각 프래그먼트의 깊이 값 (뷰 시점에서의~) 을 저장한다. **
2. 시뮬레이션한 카메라의 초점 거리와, 저장된 $ z $ 버퍼에서의 $ z $ 값과 동일한가를 분석한다. 만약 동일하거나 거의 같다면 이미지는 선명하고 포커스가 맞게 된다. 하지만 픽셀의 깊이가 *초점 거리* 와 다르면 이미지에 적용하는 블러의 양도 함께 증가한다.
3. 컴퓨트 쉐이더를 통해 처리한다. (사실 컴퓨트를 사용하지 않고도 일반 렌더링 파이프라인으로도 어거지로 구현 가능하다.)
4. 가변 블러링된 장면을 최종 프레임 버퍼에 보내서 모니터에 표시한다.

#### D. *DoF* 예시

> Chapter10/_1021_depthoffield.cc 을 참고한다.

##### startup() -> InitializeObjectInformation()

이 함수에서는 5 개의 용 메시 정보를 가지는 오브젝트를 생성한다. albedo (diffuse) 색은 각자 다 다르다.

##### startup() -> GenerateDepthRenderFramebufferComponents()

오프스크린을 수행할 프레임버퍼를 만든다. 깊이 값에 대한 텍스쳐는 MIN 및 MAG 필터에 대해서 `GL_LINEAR` 로 설정한다. 색깔 텍스쳐는 부동소수점형 `GL_RGBA32F` 으로 만든다. 왜냐면 $ a $ ***(alpha)*** 값에 깊이 정보를 저장할 예정이기 때문이다.

##### startup()

마지막으로 컴퓨트 쉐이더에서 쓸 예정인 임시 텍스쳐를 하나 만들고 (사이즈는 기본 텍스쳐와 도치된 사이즈로 정한다.) 텅 빈 VAO 을 만든다.

##### render() -> UpdateMatrixes((float)total_time + 30.f)

이 함수에서는 오브젝트를 회전시키기 위해 모델 매트릭스를 갱신한다. 갱신한 후에는 `GL_DEPTH_TEST` 을 키고 렌더링 할 준비를 한다.

##### render() -> RenderScene(total_time)

우선 오프스크린 렌더링을 해야하기 때문에 아까전에 만들어놓은 `depth_fbo` 을 바인드하고, 해당 *FB* 에 어태치먼트를 설정한 뒤, 초기화를 한다. 그리고 `view_program` 이라는 라이트 렌더링 및 $ a $ 값에 $ z $ 값을 갱신하는 쉐이더를 사용해서 5 개의 오브젝트를 렌더링한다.

render.fs.glsl 의 마지막 코드를 보면, `GL_COLOR_ATTACHMNET0` 에 색상을 갱신할 때 알파 값으로 View 공간에서의 $ z $ 값을 집어넣는 것을 알 수 있다. 이 값은 아주 나중에 컴퓨트 쉐이더로 처리한 텍스쳐를 가지고 **box filter** 을 사용하는데 중요하다.

``` c++
// Write final color to the framebuffer
color = vec4(diffuse + specular, fs_in.V.z);
```

##### render() -> ProcessGaussianFilter()

*Parallel Prefix Sum* 을 사용해서 텍스쳐의 정두합을 계산한다. 다만 컴퓨트 코드에서 이미지를 가져올 때 `readonly` 및 `writeonly` 와 같은 지시어의 특성을 따라야 하기 때문에 `glBindImageTexture` 로 텍스쳐를 이미지로 바인드 시, **`GL_READ_ONLY` 및 `GL_WRITE_ONLY`** 을 사용해서 무엇을 읽기로 쓰고 무엇을 쓰기로 쓸 지를 정해야 한다.

``` c++
glBindImageTexture(0, color_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
glBindImageTexture(1, temp_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
```

설정을 완료했으면 컴퓨트 쉐이더를 실행한다. 여기서 `glMemoryBarrier` 함수를 사용해서 이미지 접근 권한을 얻어와야 한다.

``` c++
glDispatchCompute(info.windowHeight, 1, 1);
glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
```

> 자세한 것은 https://www.khronos.org/opengl/wiki/GLAPI/glMemoryBarrier 에서 참고할 것.

이 과정을 두 번 하면 이미지가 원래대로 돌아올 뿐만 아니라 모든 열과 행에 대해서 *Prefix sum* 이 반영된, *Summed-area table* 이 만들어진다.

`gensat.cs.glsl` 컴퓨트 쉐이더에서는 절두합을 실시하는데, $ a $ 은 그대로 두고 $ rgb $ 채널에 대해서만 절두합을 실행한다.

``` c++
shared_data[P0.x] = i0.rgb;
shared_data[P1.x] = i1.rgb;
barrier();

for (uint step = 0; step < steps; step++) {
    mask = (1 << step) - 1;
    rd_id = ((id >> step) << (step + 1)) + mask;
    wr_id = rd_id + 1 + (id & mask);

    shared_data[wr_id] += shared_data[rd_id];
    barrier();
}

imageStore(output_image, P0.yx, vec4(shared_data[P0.x], i0.a));
imageStore(output_image, P1.yx, vec4(shared_data[P1.x], i1.a));
```

##### render() -> FinalRender() 

마지막으로 *Summed-area table* 을 이용해서 피사계 심도가 적용하여 최종 렌더링한다. 실제 블러링을 반영하는 쉐이더 단계이기도 하다.

`display.fs.glsl` 에서는 다음과 같이 최종 색상을 반영한다.

1. 현재 `gl_FragCoord.xy` 위치의 *Summed-area table* 의 값을 가져온다. 또한 `gl_FragCoord.xy` 자체의 값도 가져온다.
2. 블러링 할 사각형 영역을 계산하기 위해 **초점 거리** (focal_distance) 및 **초점 깊이** (focal_depth) 을 사용해서 사각형 반직경 (half-radius) 을 계산한다.
3. [summed_area_field] 이미지대로 사각형을 더하고 빼고 빼고 더하기 위한 P[4] 배열 텍셀 값을 생성한다. 이 값은 정수형이 아닌, `float ` 텍셀 좌표를 따르기 때문에 (아마도?) 기존 텍스쳐 사이즈로 나눠야 한다.
4. [**`textureLoad(sampler, p, lod)`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/textureLod.xhtml) 을 사용해서 절두합 테이블의 값을 가지고 온다.
5. 4 개의 값을 적절하게 더하고 뺀 후, 반직경 사각형의 영역을 계산해서 나누면 **box filtering** 을 수행한다. 그러면 최종 *rgb* (가변 블러 처리된) 을 얻게 된다. alpha 값은 1.0 으로 둔다. (실제 렌더링은 오프스크린에서 이미 끝났기 때문)

결과는 Result/OpenGL sb6/_1021.mp4 을 본다. (gif 올리기에는 용량이 너무 큼)

만약 리소스를 줄이고 싶다면 따로 버퍼를 만들어서 거기에 깊이 값을 올리던지, 아니면 포맷을 용량이 작은 것으로 줄여서 올리던지 하는 방법이 있다.

### 10.2.2 Flocking algorithm using compute shader

> 나중에 할 것임.