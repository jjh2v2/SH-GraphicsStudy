# Chapter 12 : Rendering Technique part 2

## 12.3 다른 렌더링 방식

전통적인 **Forward rendering** 은 버텍스 쉐이더에서 시작하여 다른 여러 일련의 스테이지를 거쳐 프래그먼트 쉐이더에서 끝나는 완전 무결한 그래픽스 파이프라인을 수행한다. 프래그먼트 쉐이더는 최종 색상을 계산하고 (후처리 제외) 각 드로잉 커맨드가 수행이 되면 프레임버퍼의 내용이 점진적으로 완성이 된다.

하지만 실제 그래픽스 세계에서는 엄청나게 많은 변수가 존재하거니와 그것들을 전부 한 쉐이더에 때려 담거나 지원하도록 한다면 언젠가는 한계에 봉착할 것이다. 퍼포먼스도 무시할 수 없다. 따라서 다음 절에서는 **쉐이딩 정보의 일부만 계산**하고 모든 객체가 렌더링 된 후에 **장면을 완성** 하거나, 프래그먼트 쉐이더에서 지오메트리 프로세싱을 수행하게 할 수도 있다.

### 12.3.1 [Deferred Shading](https://en.wikipedia.org/wiki/Deferred_shading)

수 많은 물체들을 *Forward rendering* 을 사용해서 컬러 버퍼에 렌더링 할 때, 이미 임의 위치에 어떤 물체의 색상이 존재하는데도 그 보다 더 앞에 있는 ($ z $ 값이 작은) 물체에 의해서 색상이 다시 덧 씌워질 때, 이를 ***Overdraw*** 라고 한다. 이 경우 이전 계산의 결과는 새로운 렌더링 결과 값으로 교체된다. 이 말인 즉슨, **기존에 수행한 일들을 모두 버리게 되며, 이 동작이 많을 경우 퍼포먼스의 상당한 손실**로 이어지게 된다.

이를 해결하기 위해 ***Deferred Shading*** 을 사용해볼 수 있다. *지연 렌더링* 이라는 말 답게, 프래그먼트 쉐이더를 통해 수행하는 복잡한 처리를 최대로 지연시킬 수 있다.

이를 위해서는 먼저 장면을 매우 간단한 프래그먼트 쉐이터로 렌더링한다. 이 쉐이더는 나중에 쉐이딩 계산에 필요한 각 프래그먼트의 인자들을 프레임버퍼에 출력한다. 따라서 대부분의 경우 **다중 프레임버퍼 어태치먼트**가 필요하다.

또한 디퍼드 렌더링의 경우 퐁 라이팅 모델을 구현하기 위해 필요한 최소 정보는 *Diffuse color*, *Normal* $ \vec N $, 월드 공간 위치 $ V $, 뷰 공간 위치 $ V' $, 탄젠트 벡터 $ \vec T $ 등이 있음을 알 수 있다.  아무튼 이런 중간 정보를 간단한 쉐이더를 통해 각 버퍼에 집어넣는다. 이러한 중간 정보를 담는 프레임버퍼를 ***G-buffer*** (*Geometry buffer*) 라고 한다.

일단 *G-buffer* 을 생성하면 화면의 모든 각 점을 하나의 풀 스크린 사각형으로 쉐이딩 할 수 있다. 이 최종 패스는 최종 라이팅 알고리즘을 전부 적용하겠지만, 요점은 모든 공간의 각 메쉬의 프래그먼트에 대해 한번씩 적용하는게 아니라, **최종 프레임 버퍼의 각 픽셀에 대해서만 한 번씩 적용된다**. 따라서 프래그먼트 쉐이딩의 비용을 감소시킬 수 있다. 하지만 이 G-buffer 을 저장하고 관리하는데 드는 비용이 디퍼드 렌더링을 사용함으로써 얻는 비용보다 크지 않아야 한다.

#### G-buffer 생성하기

> 여기서부터는 예시인 Chapter12/_1231_deffred.cc 을 짚어가면서 설명한다.

*Deferred Renderer* 의 첫 스테이지는 ***G-buffer*** 을 생성하는 것이다. 위에서도 말했지만 G-buffer 는 여러 어태치먼트를 사용한 프레임버퍼에 쉐이더를 돌리면서 어태치먼트에 바인딩된 버퍼에 필요한 값들을 우겨 넣는 식으로 사용된다. *OpenGL* 에서는 컬러 버퍼의 최대 어태치먼트 수를 제한하고 있다. 기본적으로 `GL_COLOR_ATTACHMENT` 는 0 에서 7 까지 제공하고는 있다.

이 예제에서 사용할 버퍼는 `GL_RGBA32F` 와 `GL_RGBA32UI` 을 사용한다. 예제에서는 이렇게 각 채널당 32비트를 쓰지만은 실 작업물에서는 대역폭을 다 잡아먹지 않도록 버퍼 포맷을 신중하게 선택해야 한다.

일반적으로 16비트 부동소수점 값은 색상과 노멀을 처리하기에 충분하다. (사실 색상 자체는 HDR 을 포함해서 맨 마지막에 더 높은 정밀도로 처리하기만 한다면 8비트로도 충분하다) 정밀도를 유지하기 위해 월드 공간 좌표를 저장할 때는 32비트 부동소수점 값을 선호한다. 머터리얼에서 얻어오는 쉐이딩에 필요한 요소 (스펙큘러 멱승값 등) 도 저장할 수 있다.

##### startup() -> GenerateGBuffer()

이 예제에선, `GL_RGBA32UI` 의 처음 채널부터 16비트씩 3 개의 탄젠트 공간에서의 노멀 $ \vec N $ 을, 그 다음에는 16비트의 $ RGB $ *Albedo* 색상 값을, 마지막 32비트에는 객체의 머터리얼 인덱스를 저장한다. `GL_RGBA32F` 버퍼에는 3 개의 채널에 해당 픽셀의 월드 공간 좌표를, 마지막 채널에는 객체의 스펙큘러 멱승값을 저장한다.

```c++
// gbuffer_tex[0] saves three 16bits normal into R 32bit and G 16bit, 
// 16bits albedo colors into G latter 16bit and B 
// and 32bit material index as final item.
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT); 
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

// gbuffer_tex[1] saves three 32bits floating point world coordinate and 32bit specular power value.
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);
glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT); 
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

// Just depth buffer. :)
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[2]);
glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT); 

glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer_tex[0], 0);
glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer_tex[1], 0);
glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gbuffer_tex[2], 0);
```

> 사실 화면 공간 좌표에서 프래그먼트의 월드 공간 좌표를 재구성하는 방법은 여러가지가 있지만 ($ w $ 값을 저장한다던지) 이 예제에서는 직접 프레임버퍼에 저장한다고 한다.

##### render.fs.glsl

 이 프레임버퍼는 `render.glsl` 혹은 `render-nm.glsl` 에서 렌더링한다. 여러 개의 16비트 요소 및 32비트 요소를 패키징해야하는데 glsl 에서는 이를 위해 **`packHalf2x16`** 이라는 함수를 제공하고 있다.

- [**`uint packHalf2x16(vec2 v)`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/packHalf2x16.xhtml)

  packHalf2x16 returns an unsigned integer obtained by converting the components of a two-component floating-point vector to the 16-bit floating-point representation found in the OpenGL Specification, and then packing these two 16-bit integers into a 32-bit unsigned integer. The first vector component specifies the 16 least-significant bits of the result; the second component specifies the 16 most-significant bits. 

  - `v` : Specify a vector of two 32-bit floating point values that are to be converted to 16-bit representation and packed into the result. 

이 함수는 4.20 버전부터 지원하기 때문에 4.20 버전 아래인 경우에는 다음 문건을 참고하자. (요건은 그냥 어태치먼트를 늘려서 무식하게 값들을 집어넣는 방법을 쓰자 라는 것)

> https://learnopengl.com/Advanced-Lighting/Deferred-Shading

쉐이더 코드는 다음과 같다. 다만 주의할 점은, outvec0 의 샘플러 타입은 `usampler2D` 이고 outvec1 의 샘플러 타입은 `sampler2D` 이라는 것이다.  outvec0 의 버퍼 픽셀 타입은 `GL_RGBA32UI` 이다.

```c++
uvec4 outvec0 = uvec4(0);
vec4 outvec1 = vec4(0);

vec3 color = texture(tex_diffuse, fs_in.texcoord0).rgb;

outvec0.x = packHalf2x16(color.xy);
outvec0.y = packHalf2x16(vec2(color.z, fs_in.normal.x));
outvec0.z = packHalf2x16(fs_in.normal.yz);
outvec0.w = fs_in.material_id;

outvec1.xyz = fs_in.ws_coords;
outvec1.w = 30.0;

color0 = outvec0;
color1 = outvec1;
```

#### 렌더링하기

##### render()

그리고 어태치먼트로 바인딩 된 G버퍼를 최종 렌더링에 돌릴 때 같이 바인딩해서 쉐이더가 읽어들일 수 있도록 한다.

```c++
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);
```

기본 모드에서는 `light.glsl` 을 담고 있는 쉐이더를 쓴다.

##### light.fs.glsl -> unpackGBuffer(ivec2 coord, out fragment_info_t fragment)

glsl 에서 함수를 작성할 때, `in` 혹은 `out` 지정자를 인자에 설정해서 포인터와 같은 처리를 수행할 수 있도록 할 수 있다.  아무튼 이 함수에서는 텍스쳐로 가져온 (`GL_TEXTURE_0` `GL_TEXTURE_1`) G버퍼를 언패킹해서 각 픽셀의 정보 저장체에 집어넣는다.

- [**`unpackHalf2x16(uint v)`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/unpackHalf2x16.xhtml)

  unpackHalf2x16 returns a two-component floating-point vector with components obtained by unpacking a 32-bit unsigned integer into a pair of 16-bit values, interpreting those values as 16-bit floating-point numbers according to the OpenGL Specification, and converting them to 32-bit floating-point values. The first component of the vector is obtained from the 16 least-significant bits of v; the second component is obtained from the 16 most-significant bits of v. 

  - `v` : Specify a single 32-bit unsigned integer values that contains two 16-bit floating point values to be unpacked. 

실 사용 코드는 다음과 같다

```c++
uvec4 data0 = texelFetch(gbuf_tex0, ivec2(coord), 0);
vec4 data1 = texelFetch(gbuf_tex1, ivec2(coord), 0);

vec2 temp = unpackHalf2x16(data0.y);
fragment.color = vec3(unpackHalf2x16(data0.x), temp.x);
fragment.normal = normalize(vec3(temp.y, unpackHalf2x16(data0.z)));
fragment.material_id = data0.w;

fragment.ws_coord = data1.xyz;
fragment.specular_power = data1.w;
```

##### light.fs.glsl -> light_fragment(fragment_info_t fragment)

여기서 최종 렌더링을 수행한다. 각 픽셀의 정보를 가진 fragment 를 인자로 입력해서, 총 $ 64 $ 개의 빛 라이팅 연산을 수행한다. 이 때 View 공간에서 수행하는 것이 아니라, World 공간에서 수행하는 것을 유념한다.

또한 스펙큘러 라이팅 처리 시, 월드 공간 상의 $ \vec V $ 가 입력으로 들어가지 않음을 주의한다. (여기서는 그냥 예제 상 $ \vec N $ 과 $ \vec R $ 만을 이용해서 대충 수행하는 것 같다)

```c++
for (int i = 0; i < num_lights; i++) {
    vec3 L = light[i].position - fragment.ws_coord;
    float dist = length(L);
    L = normalize(L);
    vec3 N = normalize(fragment.normal);
    vec3 R = reflect(-L, N);
    float NdotR = max(0.0, dot(N, R));
    float NdotL = max(0.0, dot(N, L));
    float attenuation = 50.0 / (pow(dist, 2.0) + 1.0);

    vec3 diffuse_color  = 1.0 * light[i].color * fragment.color * NdotL * attenuation;
    vec3 specular_color = vec3(1.0) * pow(NdotR, fragment.specular_power) * attenuation;

    result += vec4(diffuse_color + specular_color, 0.0);
}
```

중간의 `attenuation` 은 감쇠 인자로, 64 개의 빛이 동시에 내리쬐고 있기 때문에 최대한 빛의 영향을 작게 하기 위해 필요한 변수이다.

##### 결과

결과는 다음과 같다.

![_1231](C:/Users/JongminYun/Documents/Devel/GraphicsStudy/Results/OpenGL_Sb6/_1231.png)

#### 노멀 맵핑은?

노멀 맵핑의 경우에는, 라이팅에 필요한 모든 벡터 요소를 탄젠트 벡터로 변환시키면 된다. 혹은 G버퍼를 만드는 쉐이더의 프래그먼트 부분에서 월드 공간의 $ \vec N $ 과 $ \vec T $ 을 이용해 $ \vec B $ 을 만든 뒤, $ TBN $ 행렬을 만든다. 이를 사용해 노멀 맵의 임의 텍셀의 노멀 (물론 0 에서 1 까지의 값만을 가지고 있기 때문에 -1 에서 1 까지의 값으로 변환 시키고) 을 곱한 뒤 G버퍼에 저장한다. 그러면 나중에 최종 렌더링 쉐이더에서 반영된 노멀 값으로 쓸 수 있을 것이다.

#### 단점

1. 디퍼드 쉐이딩 구현의 **대역폭을 신경쓰지 않으면** 성능이 더 안 좋을 수 있다. 

   이 예제에서는 G버퍼의 각 픽셀에 대해 $ 32 * 8 = 256 $ 비트의 정보를 사용했지만 효율적으로 사용한 것은 아니다. 월드 좌표 공간을 직접 *G-buffer* 에 패킹했기 때문에 그것만으로도 $ 96 $ 비트가 소모되었다.

   하지만 실제로는 최종 패스를 렌더링 할 때는 **각 픽셀의 화면 공간 좌표** 를 사용한다. 이 좌표는 `gl_FragCoord` 의 x,y 요소 및 깊이 버퍼 $ z $ 의 값을 통해 구할 수 있다.

   이 좌표에서 월드 좌표를 구할려면 다음과 같은 과정을 거친다.

   1. 뷰포트 변환을 원 상태로 돌린다.
   2. $ PV $ 행렬 (프로젝션 * 뷰) 의 역을 적용해서 **클립 공간에서 월드 공간으로 **이동시킨다.

2. 보통 **AA** 와 잘 동작하지 않는다는 점이 단점으로 들 수 있다.

3. 대다수의 디퍼드 쉐이딩 알고리즘은 **투명도를 지원하지 않는다.** G버퍼의 각 픽셀에 대해 단일 프래그먼트의 정보만을 저장하기 때문이다. 투명도를 지원할려면 뷰어에서 가장 가까이 있는 투명한 프래그먼트부터 멀리 있는 불투명한 프래그먼트까지의 정보를 모두 알아야 하는데, 이를 수행하는 알고리즘 중에서는 **Order Independent Transparency** 라는 방식을 사용해서 수행할 수 있다.

   이 방식은 이 책의 범위를 아득히 뛰어넘지만 이 책의 저자가 홈페이지에 따로 글을 남긴 것 같다.

   > http://www.openglsuperbible.com/2013/08/20/is-order-independent-transparency-really-necessary/

### 12.3.2 [Ambient Occlusion](https://en.wikipedia.org/wiki/Ambient_occlusion)

*앰비언트 오클루젼* 은 [***Global Illumination***](https://en.wikipedia.org/wiki/Global_illumination) 의 요소 중 하나를 시뮬레이션 하는 기법이다. 글로벌 일루미네이션 (전역 광원) 은 장면의 객체들 간에 **산란하는 빛의 효과를 표현**하는 개념? 이며, 이 기법 하에서 서피스는 인접 서피스에서 반사된 빛에 의해 간접적으로 빛을 받고 이 과정이 반복된다.

*Ambient Light* 는 이러한 산란된 빛에 대한 근사치를 나타내며, 작은 고정값이 라이트 계산에 더해지게 된다.하지만 객체에 **깊은 주름이나 빈틈** 이 있다면, 광원을 **가리는 인접 서피스** 때문에 해당 부분에 빛이 덜 비춰질 것이다. 이것을 시뮬레이션 하는 것이 ***AO*** 이다. 이 기법 중 임시 처리 및 근사치 방식을 사용해서 정확하지는 않으나 효율적인 방법을 구현할 수 있는 [***SSAO (Scree-space Ambient Occlusion)***](https://en.wikipedia.org/wiki/Screen_space_ambient_occlusion) 을 사용해본다.

> The algorithm is implemented as a [pixel shader](https://en.wikipedia.org/wiki/Pixel_shader), analyzing the scene [depth buffer](https://en.wikipedia.org/wiki/Depth_buffer) which is stored in a texture. For every [pixel](https://en.wikipedia.org/wiki/Pixel) on the screen, the pixel shader samples the depth values around the  current pixel and tries to compute the amount of occlusion from each of  the sampled points. In its simplest implementation, the occlusion factor depends only on the depth difference between sampled point and current  point. 

> 또 다른 예제는 여기서 찾아볼 수 있다.
> http://john-chapman-graphics.blogspot.kr/2013/01/ssao-tutorial.html
> https://learnopengl.com/Advanced-Lighting/SSAO

![_ao_1](http://apprize.info/programming/opengl_1/opengl_1.files/image256.jpg)

2 차원의 서피스와 서피스 주위에 무수한 포인트 라이트 (360 도 전방으로 빛을 쬔다) 가 있다고 할 때, *Ambient Light* 는 해당 서피스의 **한 점을 지나는 빛의 양** 이라고 할 수 있다. 완전히 평면이 서피스의 경우에는 모든 점은 서피스 위의 모든 빛의 양을 받는다. 하지만 위 그림과 같이 울퉁불퉁할 경우에는 모든 서피스에 대해 받는 빛의 양이 같지 않을 것이다.

#### 방법

위 서피스에서, 가장 위쪽에서는 전부는 아니더라도 거의 모든 라이트는 볼 수 있을 것이다. 하지만 구덩이 아래쪽에서는 **빛이 가려지고**, 산란 빛 (Ambient Light) 을 받는다. 사실 완전한 GI 을 구현하려면 각 점에 대해 수백 혹은 수천개의 방향으로 광선을 추적해서 (Ray tracing) 어디에 충돌했는가를 확인해야 하지만, 실시간 어플리케이션에서는 비용이 너무 크기 때문에 **화면 공간에서 직접 차폐, 즉 가려짐 (Occlusion) 여부**를 계산하는 방법을 사용한다.

> In order to get high quality results with far fewer reads, sampling is performed using a randomly rotated [kernel](https://en.wikipedia.org/wiki/Kernel_(image_processing)). The kernel orientation is repeated every *N* screen pixels in order to have only high-frequency noise in the final  picture. In the end this high frequency noise is greatly removed by a *N*x*N* post-process blurring step taking into account depth discontinuities  (using methods such as comparing adjacent normals and depths). Such a  solution allows a reduction in the number of depth samples per pixel to  about 16 or fewer while maintaining a high quality result, and allows  the use of SSAO in soft real-time applications like computer games. 

이 기법을 구현하려면, **화면 공간의 각 점**에서 임의의 방향에 따라 가상의 선을 진행시키고, 그 선의 진행 과정에서 다른 픽셀의 $ z $ 값과 비교해 얼마나 가려졌는가를 계산할 수 있다. 그렇게 하기 위해서는 오프스크린 렌더링이 필수이며 프레임버퍼에는 *G-buffer* 등을 사용해 화면 공간에 최종 렌더링되는 각 프래그먼트별 **노멀**, **뷰 공간의 선형적 깊이**를 어태치먼트에 들어가도록 렌더링 해야한다.

두 번째 패스에서는 이 정보를 사용해서 각 픽셀에 대한 가려짐 수준을 계산하며, 특별한 VAO 도 필요없고 오로지 사각형 (`GL_TRIANGLES_STRIP`) 으로만 렌더링한다. 이 쉐이더는 첫 번째 패스에서 렌더링한 깊이 값을 읽고, **임의의 방향으로 진행 한 다음에 (사실 SSAO을 잘 쓰려면 임의의 방향으로 진행하게 해서는 안된다)** 각 점에서 방향에 따라 가상의 선을 진행시키면서, 도달한 픽셀의 $ z $ 의 값이 (뷰 공간의 깊이 $ z $ 값) 선을 따라 계산된 깊이보다 작은지를 계산한다. **만약 작다면 그 점은 가려진 점으로 간주한다**.

![hemisphere](http://apprize.info/programming/opengl_1/opengl_1.files/image257.jpg)

임의의 방향을 선택하기 위해, **단위 반지름 구 (Hemisphere)** 상의 여러 임의 벡터를 담은 UBO 을 미리 초기화 시킨다. 임의의 벡터들은 서피스의 **밖을 향해야** 한다. 즉, 어떤 점의 **서피스 노멀**을 90 도 중심으로 해서 $ x, y, z $ 축으로 $ \pm $ 90' 영역 내의 있는 벡터들만 가상의 선으로 사용할 수 있게 끔 고려해야 한다. 따라서 첫 번째 패스에서 *G_buffer* 에 노멀을 저장해야 한다. (물론 이 노멀은 회전을 반영해야 한다) 

따라서 반구 내의 임의 벡터를 얻기 위해, **서피스 노멀과 임의의 방향 벡터를 내적**해서 음수라면 임의 방향 벡터를 반대 방향으로 돌린다. 그러면 반구 내 범위에 존재하는 방향 벡터가 된다.

가상의 선의 방향이 될 임의 벡터**들**을 구했으면, 벡터들을 따라 이동한다. 먼저 서피스의 점에서 시작해서 선택한 거리 벡터를 따라 작은 거리를 단계적으로 이동한다. 그러면 이동할 때마다 $ x, y, z $ 좌표의 새로운 점이 생성이 된다. 이 때 $ x, y $ 좌표를 사용해서 버퍼의 뷰 공간 선형 깊이 $ z $ 값을 읽는다. 이 이동한 시점의 $ z $ 값과 기존 점의 $ z $ 값을 비교해, 기존 점의 $ z $ 값이 더 크면, 기존 점은 가려진 것으로 판단한다는 것이다.

사실 이 알고리즘을 한 방향에 대해서만 쓰면 매우 부정확하지만, 방향을 무수한 개수로 나눠서 쓰고자 하면 서로가 보간되어 통계적으로 잘 동작하며, 출력 이미지 품질이 더 더욱 좋아지게 된다. 또한, ***SSAO*** 을 수행하고자 할 때는 *선택할 임의 방향의 수, 방향 진행 단계의 수, 방향 각도* 등이 이미지 품질의 요소에 영향을 미친다.

![noise](https://learnopengl.com/img/advanced-lighting/ssao_banding_noise.jpg)

사실 임의의 방향 벡터를 진행시킬 때, 고정된 거리로만 이동시켜서 *AO* 를 구현하면 방향의 개수가 적은 단계에서는 이미지 품질이 그렇게 좋지 않은 것을 알 수 있다. 하지만 이동 거리를 임의 거리로 이동시키게 하면 노이즈는 끼지만 전반적인 이미지 품질은 높아지는 것을 알 수 있다....고 한다. 이 방식에 의해 생긴 [**노이즈를 상쇄시키는 방법**](https://en.wikipedia.org/wiki/Kernel_(image_processing))을 쓰면 약간 블러링된 이미지가 나오나 노이즈가 없어진다고 한다. 

*AO* 항이 만들어졌으면 이를 렌더링된 이미지에 적용할 수 있다. *AO* 는 단순히 *AL* 이 가려지는 정도를 수치적으로 나타내기 대문에 최대 앰비언트 라이트 항에 오클루전 항을 곱해주면 된다. 이 *AO* 항은 모델의 음푹 들어간 곳이 *AL* 을 덜 받도록 한다. 

#### 예제

> Chapter12/_1232_ssao.cc 을 본다.

##### render.fs.glsl

***일반 렌더 (SSAO 계산 X)*** 파일에서는 뷰 공간의 노멀 벡터, 라이팅 방향 벡터, 뷰 방향 벡터를 저장하고, Diffuse color, Specular color 을 계산해서 최종 컬러를 저장한다. 또한 뷰 공간에서의 선형 $ z $ 값을 $ RGBA $ 의 alpha 채널에 저장한다. (중요)

``` c++
// Normalize the incoming N, L and V vectors
vec3 N = normalize(fs_in.N);
vec3 L = normalize(fs_in.L);
vec3 V = normalize(fs_in.V);

// Calculate R locally
vec3 R = reflect(-L, N);

// Compute the diffuse and specular components for each fragment
vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
diffuse *= diffuse;
vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

// Write final color to the framebuffer
color = mix(vec4(0.0), vec4(diffuse + specular, 1.0), shading_level);
normal_depth = vec4(N, fs_in.V.z);
```

##### ssao.fs.glsl

이 쉐이더에서는 **SSAO** 처리를 한다. 

* [**`textureLod(sampler, texel, lod)`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/textureLod.xhtml)

  **textureLod** performs a texture lookup at coordinate P from the texture bound to sampler with an explicit level-of-detail as specified in lod. lod specifies λbase and sets the partial derivatives as follows: 

파일에서 SAMPLE_POINT 유니폼 버퍼 구조체는 C++ 코드에서 SAMPLE_POINT 구조체와 일치한다.

``` c++
// Uniform block containing up to 256 random directions (x,y,z,0)
// and 256 more completely random vectors
layout (binding = 0, std140) uniform SAMPLE_POINTS {
    vec4 pos[256];
    vec4 random_vectors[256];
} points;
```

`pos` 는 방향 벡터를 나타내고, `random_vectors` 는 임의 이동에 쓰일 거리 벡터를 나타낸다.

메인 함수에서는 우선 `textureLod` 을 사용해 노멀과 뷰 공간 선형 $ z $ 값을 가져온다. 그리고 *PRNG* (유사 무작위 랜덤 넘버) 를 사용해 임의 이동에 쓰일 벡터를 가져온다. 이를 사용해서 반구의 무작위 반지름을 생성한다.

``` c++
// n is a pseudo-random number generated from fragment coordinate and depth
n = (int(gl_FragCoord.x * 7123.2315 + 125.232) * int(gl_FragCoord.y * 3137.1519 + 234.8)) ^
     int(my_depth);
// Pull one of the random vectors
vec4 v = points.random_vectors[n & 255];
// r is our 'radius randomizer'
float r = (v.r + 3.0) * 0.1;
if (!randomize_points) r = 0.5;
```

쉐이더를 바인딩할 때 `glUniform1i` 로 집어넣은 **가상 선 카운트**를 사용해 *AO* 를 처리하도록 루프를 돌린다. 이 때 방향 벡터가 해당 프래그먼트의 노멀 벡터를 중심으로 한 **바깥 방향 반구**의 범위에 있도록 **내적을 사용해서** 보정을 시킨다. 이 때 dir 은 View 공간상의 방향 벡터이다.

``` c++
// Get direction and put it into the correct hemisphere
vec3 dir = points.pos[i].xyz;
if (dot(N, dir) < 0.0)
    dir = -dir;
```

각 프래그먼트의 방향 벡터 당 4 번의 진행 과정을 거친다. 방향 벡터는 바깥쪽으로 진행하기 때문에, 뷰어쪽으로 진행하므로 $ z $ 값은 한 단계마다 $ \vec{Dir} * | r | $ 만큼씩 감소한다. 

``` c++
// Step in the right direction
f += r;
// Step _towards_ viewer reduces z
z -= dir.z * r;
```

$ f $ 는 현재까지 진행한 거리이다.

이제 $ f $ $ \vec{Dir} $ $ P $ $ \text{ssao_radius} $ 이 4 개를 사용해서 최종 진행한 곳의 (ssao_radius 로 너무 크게 진행하지 않도록 보정을 한다) 텍셀의 가져와, $ z $ 선형 값을 가져온다.

``` c++
// Read depth from current fragment
float their_depth = textureLod(sNormalDepth, (P + dir.xy * f * ssao_radius), 0).w;
```

그리고 이 프래그먼트의 오클루젼에 대한 기여도인 가중치 $ d $를 계산한다. 이 $ d $ 값은 오클루젼을 누적시킬 때 쓰인다. 여기서 `my_depth` 는 이동하기 전 최초 위치에서의 선형 $ z $ 값이다.

``` c++
float d = pow(abs(their_depth - my_depth), 2.f);
```

이제 이동한 위치에서 계산된 $ z $ 값과 이동한 위치에서 $ z $ 버퍼에 기록된 본래 $ z $ 값과 비교를 하여, 이동한 위치에서 계산된 $ z $ 값이 후자 $ z $ 값보다 크면 오클루젼을 누적시킨다. (가려진다는 말이다)

``` c++
// If we're obscured, accumulate occlusion
if ((z - their_depth) > 0.0) {
    occ += 4.0 / (1.0 + d);
}
```

임의 방향과 임의 거리를 정해서 $ z $ 값을 탐색하는 루프가 끝나면, **최종 오클루젼 값을 계산**한다. 여기서 `total` 은 진행된 모든 방향의 것을 합산한 총 진행 단계 값이다. 그리고 라이팅이 끝난 후의 객체 색상을 텍스쳐에서 얻고, 앰비언트 색상을 앰비언트 값과 같이 SSAO 레벨만큼 스케일해서 **혼합한다.**

``` c++
// Calculate occlusion amount
float ao_amount = 1.0 - occ / total;
// Get object color from color texture
vec4 object_color =  textureLod(sColor, P, 0);
// Mix in ambient color scaled by SSAO level
color = object_level * object_color + mix(vec4(0.2), vec4(ao_amount), ssao_level);
```

어두운 면에서는 ao_amount 가 0.2 보다 작을 것이고 밝은 면에서는 ao_amount 가 0.2 보다 클 것이다. 현재 예제에서는 `ssao_level` 이 1.0 으로 고정되어 있기 때문에 `mix()` 함수에 의해서 항상 `vec4(ao_amount)` 가 더해질 것이다.

결과는 Results/OpenGL_Sb6/_1232.mp4 에서 볼 수 있다. (덤 : 노이즈가 낀 화면은 box filter 등으로 블러링을 먹여 지울 수 있다)

### 12.3.3 [Julia set](https://en.wikipedia.org/wiki/Julia_set)

### 12.3.4 기초적인 Ray tracing

