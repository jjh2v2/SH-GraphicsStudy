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

