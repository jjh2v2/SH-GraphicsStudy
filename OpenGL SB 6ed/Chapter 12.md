# Chapter 12 : Rendering Technique

## 12.1 Lighting models

빛의 속성을 물리적으로 최대한 정확하게 근사해서 보여주는 고급 모델링도 존재하지만 (Lay-tracing) 실시간 구현에는 무리가 있기 때문에 물리적으로는 정확하지 않더라도 그럴듯한 결과를 보여주는 **근사적인 (속임수)** 방법을 사용하는 것이 일반적이다.

### 12.1.1 Phong Lighting Model

* 가장 일반적인 라이팅 모델 중 하나.
* **Ambient, Diffuse, Specular reflectivity** 등 대체로 세 가지 속성을 가진다. (Diffuse 는 Albedo 와 다르다)

#### Ambient Light

*Ambient Light* 는 특정 방향을 가지지 않는다. 어딘가에 광원은 존재하지만, 광선들이 장면 이곳 저곳에 분산되서 방향이라는 것이 없어지게 될 때, 미세하게나마 빛의 영향을 받는 물체의 색을 말한다. 즉, 앰비언트 라이트는 광원에 적용되는 (직접 영향을 받지 않아도 적용되는) 전역 '밝기' 요소이다.

앰비언트 광원이 최종 색상에 영향을 주는 값을 계산하기 위해서는 해당 메쉬의 **앰비언트 재질 속성을 앰비언트 라이트 값으로 곱하**기만 하면 된다.

#### Diffuse Light

방향이 있는 광원에 영향을 받을 때의 요소를 말한다. 메쉬의 특정 프래그먼트의 **유닛 노멀 벡터 $ \vec N $** 과 해당 프래그먼트에서 빛으로의 **유닛 방향 벡터 $ \vec L $ **을 ***dot product*** 하여 [0, 1] 사이의 값으로 클램핑 한 값을 토대로 Albedo 에 곱하면 최종 Diffuse 값이 나온다.

``` c++
uniform vec3 vDiffuseMaterial;
uniform vec3 vDiffuseLight;
float fDotProduct = max(0, dot(vNormal, vLightDir));
vec3 vDiffuseColor = vDiffuseMaterial * vDiffuseLight * fDotProduct;
```

#### Specular Light 

스페큘러 라이트는 광택 등이 있는 물체에서 빛을 받을 때 **보는 시점과 빛의 방향** 에 따라 광택이 보이거나 안 보이게끔 하는 데 필요한 빛 요소이다. 이런 식으로 만들어지는 더 밝은 부분을 **Specular Highlight** 라고 한다.

*Specular Light* 을 계산하는데는 **유닛 노멀 벡터** $ \vec N $ , **프래그먼트에서 빛의 방향 벡터** $ \vec L $ , 프래그먼트를 반사한 **빛의 반사 벡터** $$ \vec R $$ , 마지막으로 프래그먼트에서 뷰 시점까지의 **역카메라 시점 벡터** $$ \vec V $$ 가 사용된다. 최종적으로는 빛의 반사 벡터 $ \vec R $ 와 역카메라 시점 벡터 $ \vec V $ 의 *dot product* 을 클램핑한 값이 사용되지만, $ \vec R $ 을 생성하는 데는 $ \text{normalize}(\vec N + -\vec{L}) $ 이 계산되어야 한다.

또한 *dot product* 후에 만들어진 값에 대해서, ***Shininess*** 인자를 $ \text{pow}() $ 로 반영해야 한다. 그렇지 않으면 조금이라도 반짝 거릴만한 영역이 죄다 하얀색으로 보이거나 엄청 빛나게 보일 것이다.

``` c++
vec3 viewDir = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);  
float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess_power);
vec3 specular = specular_albedo * material.specular_color * spec;
```

라이팅에 의해 출력되는 최종 색상값은 다음과 같다.

``` c++
color = ambient + diffuse + specular;
```

#### Gouraud Shading

*고라우드 기법* 은 버텍스 쉐이더에서 해당 라이팅을 수행하는 기법이다. 이 경우에는 연산량이 적은 장점이 있지만, 메쉬의 폴리곤이 적거나 할 때는 라이팅이 제대로 수행되지 않을 수도 있는 단점이 존재한다.

#### Phong Shading

*퐁 쉐이딩* 에서는 *고라우드*에서 처럼 최종 라이팅 색상 값을 보간하지 않고, **메쉬의 노멀 벡터, 탄젠트 벡터 등을 보간**한 뒤에 픽셀 단위 (프래그먼트 단위) 로 전체 빛 계산을 수행한다. 이 경우에는 연산량이 높아지는 단점이 있지만 이를 메꾸고도 남을 만큼의 퀄리티 상승이 보장될 수 있다.

오늘날 하드웨어에서는 *Gouraud* 는 거의 안 쓰이고 최소 *Phong* 혹은 *Blinn-Phong* 과 같은 기법이 마지노선으로 쓰인다. 

### 12.1.2 Blinn-phong Lighting

*Blinn-phong lighting* 모델은 퐁 라이팅 모델의 최적화 또는 확장격의 라이팅 구현 방법이라고도 할 수 있다. *Specular light* 을 계산할 때 반사각 $ \vec R $ 을 구해서 $ \vec R \cdot \vec V $ 을 구하지 않고 다음으로 대체할 수 있다.
$$
\vec R \cdot \vec V = \vec N \cdot \vec H \\
\vec H = \frac{\vec L + \vec V}{| \vec L + \vec V |}
$$
기술적으로 퐁 공식을 적용하는 곳이라면 어디든 적용할 수 있다. 이렇게 구현한 *Specular* 빛과 일반 퐁의 *Specular* 빛의 차이가 있을 수도 있는데 그럴 때는 스페큘러 멱승값을 조절하면 거의 비슷하거나 동일한 결과를 얻을 수 있다.

### 12.1.3 Rim lighting

*Rim lighting* 은 뷰 시점과 객체와 광원이 수직을 이루는 상태에서 **사물 주위로 빛이 번지는 현상**을 시뮬레이션 하는 효과이다. 광원이 뒤에 있지 않은 경우에는 쉐이딩에 영향을 주지 않는다. 

림 라이팅이라도 불리는 이유는 광원이 물체의 테두리를 따라서 빛의 테두리가 만들어지기 때문이다.

![rim_lighting](http://apprize.info/programming/opengl_1/opengl_1.files/image230.jpg)

구현 시 필요한 것은 서피스 노말 벡터 $ \vec N $ 과 물체에서 뷰 시점의 방향 (역뷰 벡터?) $ \vec V $ 이다. 또한 광원 림 라이트의 색과 림 라이트 멱승 값이 필요하다. 어떻게 림 라이트가 계산되는 가 설명하는 것은 쉽기 때문에 넘어간다. 공식은 다음과 같다.
$$
L_{rim} = C_{rim}(1 - \vec{N} \cdot \vec{V})^{P_{rim}}
$$
이를 기존 라이팅 모델에 더하기만 하면 된다.

> 예제는 Chapter12/_1213_rimlight.cc 에서 찾아볼 수 있다.

### 12.1.4 Normal Mapping

> 노멀 맵핑에 대한 또 다른 문서는 OpenGLTutorial 의 Chapter 28. Normal Mapping.md 을 참고한다.

**노멀 맵핑** 은 **Bump mapping** 이라고도 불리는데 더 많은 정점을 메쉬에 추가하지 않고 디테일 정도를 높일 수 있는 방법 중 하나이며 현재에서도 매우 많이 쓰인다. 노멀 맵핑을 구현하려면 각 텍셀의 서피스 노멀을 저장하는 텍스쳐가 필요하며, 이 텍스쳐는 나중에 모델에 적용하여 프래그먼트 쉐이더에서 각 프래그먼트마다의 서피스 노멀을 계산하는데 사용된다.

노말 맵에 가장 일반적으로 사용되는 좌표 공간은 **Tangent space** 이다. 이는 양의 $ z $ 축이 서피스 노멀 방향으로 정렬된 지역 좌표계이며 (즉, 노멀 맵의 텍셀의 모든 값은 광원이 양의 $ z $ 방향에 있을 때를 기준으로 만들어졌다) 이 좌표 공간의 다른 두 벡터는 **Tangent vector, bitangent (이중접선) vector** 이다. 

탄젠트 벡터는 대개 지오메트리 데이터의 일부로 인코딩되서 버텍스 쉐이더의 입력으로 제공되는데, 좌표계에서 **노멀 벡터 및 탄젠트 벡터** 가 주어질 때 **바이탄젠트는 외적으로 구할 수 있다**. (탄젠트는 대개 $ u $ 을 따르고 바이는 $ v $ 을 따른다고 한다)

따라서 노멀, 탄젠트 및 바이탄젠트를 사용해서 일반 좌표계의 있는 **방향 벡터**를 **Tangent space** 로 변환할 수 있는 매트릭스를 만들 수 있다. (혹은 일반 벡터도 상관 없으나, 관여되는 모든 벡터가 동일 공간의 것이어야 한다)
$$
\vec N = \text{unit normal vector} \\
\vec T = \text{unit tangent vector} \\
\vec B = \vec N \cdot \vec T \\ \\
TBN = 
\begin{bmatrix}
\vec{T}.x & \vec{T}.y & \vec{T}.z \\
\vec{B}.x & \vec{B}.y & \vec{B}.z \\
\vec{N}.x & \vec{N}.y & \vec{N}.z
\end{bmatrix}
$$
이렇게 행렬 또는 행렬에 준하는 것을 만들면 카테시안 공간의 어떤 벡터라고 탄젠트 공간으로 변환할 수 있게 된다. 이 것을 사용해서 광원 계산에 사용할 수 있다.

``` c++
vec3 V = P.xyz;
vec3 N = normalize(mat3(mv_matrix) * normal);
vec3 T = normalize(mat3(mv_matrix) * tangent);
// Calculate the bitangent vector (B) from the normal and tangent vectors.
vec3 B = cross(N, T);

// The light vector (L) is the vector from the point of interest to
// the light. Calculate that and multiply it by the TBN matrix.
vec3 L = light_pos - P.xyz;
vs_out.lightDir = normalize(vec3(dot(L, T), dot(L, B), dot(L, N)));

// The view vector is the vector from the point of interest to the
// viewer, which in view space is simply the negative of the position.
// Calculate that and multiply it by the TBN matrix.
V = -P.xyz;
vs_out.eyeDir = normalize(vec3(dot(V, T), dot(V, B), dot(V, N)));
```

#### 예제

> Chapter12/_1214_bumpmapping.cc 을 본다.

##### bumpmapping.vs.glsl

메쉬 모델은 $ \vec N $ 와 $ \vec T $ 을 가진다.

``` c++
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
```

이를 사용해서 $ \vec B $ 을 만들어야 한다. 다만 바이탄젠트를 만들기 이전에, 오브젝트의 방향에 대해서 정렬을 해야하므로 회전 행렬 혹은 해당 오브젝트의 모델 매트릭스를 곱하고, 노멀라이즈 한다.

``` c++
vec3 N = normalize(mat3(mv_matrix) * normal);
vec3 T = normalize(mat3(mv_matrix) * tangent);
```

그 후에, $ \vec B $ 을 외적으로 만든다.

``` c++
vec3 B = cross(N, T);
```

이제, 해당 정점에서의 빛 방향 벡터 $ \vec N $ 을 만들고, 카테시안 공간의 $ \vec N $ 을 탄젠트 공간으로 변환하기 위해 **FBN** 을 적용한다. 다만 실제로 매트릭스를 만들지는 않고 내적과 노멀라이즈를 사용해서 간단히 탄젠트 공간에서의 빛 방향을 만들 수 있다.

``` c++
vs_out.lightDir = normalize(vec3(dot(L, T), dot(L, B), dot(L, N)));
vs_out.eyeDir = normalize(vec3(dot(V, T), dot(V, B), dot(V, N)));
```

라이팅 시뮬레이션이 탄젠트 공간 안에서 이루어지기 때문에 $ \vec V $ 또한 탄젠트 공간으로 변환한다.

##### bumpmapping.fs.glsl

여기서는 `varying` 으로 가져온 탄젠트 공간의 $ \vec N $ $ \vec T $ $ \vec B $ $ \vec V $ $ \vec L $ 을 이용해서 노멀 맵핑에 의한 퐁 라이팅, 림 라이트를 수행한다. 다만 $ \vec N $은 노멀 맵에서의 $ \vec N $ 으로 갈아치운다.

``` c++
vec3 N = normalize(texture(tex_normal, fs_in.texcoord).rgb * 2.0 - vec3(1.0));
```

따라서 다음과 같이 정점을 증가시키지 않고도 디테일이 잘 살아있는 라이팅 시뮬레이션 결과를 얻을 수 있다.

![normal_mapping](../Results/OpenGL_sb6/_1214.PNG)

### 12.1.5 Environment Mapping (Reflection Mapping)

> https://opentutorials.org/course/2221/12595

일반 라이팅 쉐이더는 매우 복잡해질 가능성이 있으며, 임의의 환경에 대해서 동작하는 공식을 만드는 것은 거의 불가능하다. 이 때 필요한 것이 ***Environment Mapping*** 이다. 실시간 그래픽스 프로그램에서 사용되는 *환경 맵핑* 에는 몇 가지 종류가 있다.

1. [구 환경 맵핑](https://en.wikipedia.org/wiki/Sphere_mapping)

   시뮬레이션된 주위 사물에 의해 조명을 받는 **반구** 형태의 이미지로 표현한다. 직관적이고 신속하게 환경 맵을 만들 수 있지만 360 도 전체를 시뮬레이션 할 수는 없다.

2. [정방 환경 맵핑(equirectangular map)](https://en.wikipedia.org/wiki/Equirectangular_projection)

   이퀴렉텡귤러 맵은 반구에서 더 나아가서 360 도 전체를 시뮬레이션 할 수 있도록 구성된 **사각형 맵** 이다. 

3. [큐브 환경 맵](https://en.wikipedia.org/wiki/Cube_mapping)

   큐브 맵은 가운데에서 밖으로 주위를 볼 수 있는 박스 안에 있다고 가정하고, 이 박스를 표현하는 **6 개의 면**으로 구성된 특수한 텍스쳐를 이용해서 환경을 맵핑하는 방법이다.

#### A. **Spherical Environment Map**s

![reflection](https://www.opengl.org/archives/resources/code/samples/advanced/advanced97/notes/img130.gif)

*구 환경 맵* 은 시뮬레이션할 재질에 대해서 주위 환경으로 만들어지는 맵을 사용해 라이팅을 표현하는 텍스쳐 맵이다. 쉐이딩 되는 지점에 대해 뷰 방향 $ \vec V $ 과 서피스 노멀 $ \vec N $ 을 구한 다음에 두 방향으로 만들어지는 노멀라이즈된 리플렉션 벡터 $ \vec R $ 을 만들고, 텍스쳐 좌표를 계산해서 해당 프래그먼트에 쏴줄 컬러를 얻는다.

구 환경 맵은 대체로 이런 식으로 되어 있다.

![spherical_map](https://i.stack.imgur.com/TYbvO.jpg)

구 환경 맵을 맵핑하는 방법은 다음과 같다.

##### 방법

1. $ \vec V $ 와 $ \vec N $ 을 사용해서 반사 벡터 (방향) $ \vec R $ 을 도출한다.

2. $ \vec R $ 을 사용해서 스케일 인자 $ m $ 을 계산한다.
   $$
   m = 2\sqrt{\vec R_x^2 + \vec R_y^2 + (\vec R_z + 1)^2}
   $$

3. 마지막으로, 최종 텍셀 위치를 구한다.
   $$
   s = \frac{\vec R_x}{m} + 0.5 \\
   t  = \frac{\vec R_y}{m} + 0.5
   $$


##### 예시

> Chapter12/_1215_spherical.cc 을 찾아볼 것

###### render.fs.glsl 에서...

``` c++
// Reflect u about the plane defined by the normal at the fragment
vec3 r = reflect(u, normalize(fs_in.normal));

// Compute scale factor
r.z += 1.0;
float m = 0.5 * inversesqrt(dot(r, r));

// Sample from scaled and biased texture coordinate
color = texture(tex_envmap, r.xy * m + vec2(0.5));
```

결과는 다음과 같다.

![_1215_1](../Results/OpenGL_Sb6/_1215_1.png)

#### B. Equirectangle Environment Mapping

> https://www.slideshare.net/yorung/360vr

*이퀴렉탱글러 맵* 은 구 환경 맵과 유사하지만, 360 도 전방위 환경의 정보를 반영할 수 있다는 것이 장점이다. 그리고 구의 극단에서 샘플링할 때 생기는 텍스쳐가 늘어나는 현상이 눈에 덜 띈다. 

![equi_map](https://c1.staticflickr.com/6/5231/7151570807_3cf6337fed_b.jpg)
$$
tc := \text{Texel which has x, y component [0, 1]} \\
\text{Phase 1} \\
tc_t = \vec R_y \\
tc_s = \text{normalize}(\vec R \text { which y component is 0}).x * \frac{1}{2} \\
\text{Phase 2} \\
s := \text(sign)(\vec R.z) * \frac{1}{2} \\
tc_s = 0.75 - s (\frac{1}{2} - tc_s) \\
tc_t = 0.5 + tc_t\frac{1}{2}
$$

#### C. Cube Environment Map

> 자세한 설명은 OpenGL Tutorial/Chapter 17. Cubemaps 에서 볼 수 있다.

### 12.1.6 **Material Properties** 

지금까지는 전체 모델에 대해서 단일 재질을 사용했지만, 실제 어플리케이션에서는 모든 모델이 단일 재질을 가질리가 없을 뿐더러 여러 가지 재질을 가지고 있기 때문에 이런 사항들을 전부 고려해야 한다. 서피스 마다의 재질 속성을 할당할 수 있을지도 모르고, 텍스쳐에 서피스 정보를 저장하게 할 수도 있다. 또한 스페큘러 멱승 지수를 다르게 해서 흐리게 하거나 좀 더 밝게 할 수도 있을 것이다.

모델의 거칠기 정도 등도 각 오브젝트마다 제각각 다를 것이다. 이런 거칠기를 조절할 수 있는 방법 중 하나는 환경 맵을 미리 **블러** 시켜서 텍스쳐로 저장해, 광택 인자로 사용하게 해서 맵의 선명한 버전에서 블러된 버전으로 점진적으로 변화시키게 할 수 있기도 하다.

해당 예제에서는 다음과 같은 환경 맵과 광택 맵 (***Shininess Map***) 을 사용할 것이다.

![_12_1_6_1](http://apprize.info/programming/opengl_1/opengl_1.files/image242.jpg)

*광택 맵* 에서 텍셀이 지정한 부분이 $ 0 $ 이라면 블러된 환경 맵을, $ 1 $ 이라면 블러되지 않은 환경 맵을 사용하기로 했다.

##### 예제

> Chapter12/_1216_glossy.cc 을 참고할 것.

### 12.1.7 Shadow Mapping

> 자세한 설명은 OpenGL Tutorial/Chapter 25. Shadow Mapping 을 참고할 것.

그림자 계산에 있어서 가장 중요한 점은 **관심 지점에 빛이 도달하는가**를 결정하는 것이다. 쉐이딩되는 점에서 라이트까지 장애물이 없는 지를 결정해야 한다. 이 과정을 자연스럽게 하는 버퍼가 있는데, 깊이 버퍼가 바로 그 예이다.

그림자 매핑은 **광원의 시점에서 장면을 렌더링하여 정보를 만드는 기법**이다. 깊이 정보만 필요하며, 따라서 오프스크린이 자연스레 따라오고 깊이 어태치먼트만 있는 프레임 버퍼를 만들어서 사용해야 한다. 

> Chapter 25. Shadow Mapping 에서 발췌...
>
> 우선 World 공간의 좌표 $$ \overline{P} $$ 가 그림자에 위치해 있는가를 확인할려면, $$ \mathbf{T} $$ 을 곱해 빛의 시점에서 Depth value 가 사상된 위치의 Depth value 보다 큰 지 (그림자에 위치해 있는지), 아니면 작은 지 (빛의 영향을 받고 있는지) 를 확인해야 한다. 그럴려면 Vertex Shader 에서 Model matrix 에 사상된 각 정점 $$ \overline{P} $$ 을 $$ \mathbf{T} $$ 와 곱한 뒤에 Fragment Shader 로 보낸다. 그리고 이 보낸 좌표 값을 Depth Map 의 텍셀의 값과 비교해서 그림자인가 아닌가를 반환한다. 이 반환한 값을 *Diffuse 및 Specular* 에 곱해서 적용하면 기본적인 그림자가 만들어진다.
>
> 다만 여기서 주의해야 할 점은, Orthogonal Perspective 에서는 $$ \mathbf{T} $$ 에 의해서 변형된 모든 값 (Clipping 제외)이 $$ [-1, 1] $$ 사이에 있기 때문에 파이프라인 단계에서 Clip space 로 변형 시 $$ w $$ 축 값으로 $$ x,y,z $$ 을 나눠주는 것을 명시적으로 할 필요는 없다는 점이다. 그러나 일반 점 조명 (Point light) 혹은 Specular light 와 같은 경우에는 이 과정이 별도로 필요하다.
>
> 그리고 $$ fragPosLightSpace $$ 의 값은 $$ [-1, 1] $$ 값 사이인데, Depth map 의 좌표는 $$ [0, 1] $$ 사이이고 Depth value 역시 $$ [0, 1] $$ 의 범위를 가지므로 간단한 맵핑이 다시 필요하다.

이와 같은 연산은 그래픽스에서 매우 흔한 연산이기 때문에, OpenGL 은 [***Shadow Sampler***](https://www.khronos.org/opengl/wiki/Sampler_(GLSL)#Shadow_samplers) 을 지원하고 있다. GLSL에서는 2D 텍스쳐에 대한 섀도우 샘플러를 `sampler2DShadow` 타입의 변수로 선언하며, 그 외의 영우에 `sampler1DShadow` `samplerCubeShadow` `samplerRectShadow` 와 같은 샘플러를 생성할 수 있다. 각 타입에 대한 배열 역시 생성할 수 있다.

>https://renderman.pixar.com/resources/RenderMan_20/softShadows.html
>http://fabiensanglard.net/shadowmappingPCF/index.php
>https://stackoverflow.com/questions/15250380/what-are-shadow-samplers-in-opengl-and-what-are-possible-uses-for-them

#### Example

> Chapter12/_1217_shadow.cc 을 참고한다.

`GL_DEPTH_COMPONENT32F` 와 같은 깊이 버퍼 타입을 가지는 $ z $ 버퍼 텍스쳐를 실제 렌더링 쉐이더에서 그림자를 만들어 내고자 할 때는 그냥 `GL_TEXTURE0` `GL_TEXTURE_2D` 와 같은 타깃으로 바인딩하면 된다. 다만 무사히 잘 넘어가기 위해서는 실 렌더링 쉐이더의 바인딩 포인트 샘플러가 *Shadow Sampler* 형이어야 한다. 여기서는 `sampler2DShadow` 을 사용했다.

``` c++
// Depth buffer
glGenTextures(1, &depth_texture);
glBindTexture(GL_TEXTURE_2D, depth_texture);
glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
```

이 때 $ z $ 버퍼로 쓰일 텍스쳐는 나중에 그림자 렌더링시 `textureProj` 함수를 쓰기 위해 특정한 옵션을 더 설정해줘야 한다. 그것이 바로 `GL_TEXTURE_COMPARE_MODE` `GL_TEXTURE_COMPARE_FUNC` 이고, 고정 함수 형태로 제공된다.

마지막으로 유저 시점에서 광원 시점에서 생성된 $ z $ 버퍼의 값을 가져오기 위해서는, 위치 값을 **그림자 행렬**로 변환해서 해당 $ z $ 값이 그림자 버퍼의 $ z $ 값보다 큰 지 작은 지를 판별해야 한다. 큰 경우에는 그림자가 생기며 작은 경우에는 그림자가 생기지 않을 것이다.

**그림자 행렬 (Shadow Matrix)** 는 NDC 로 투영까지 시켜주기 때문에 ( $ T = PV $ 에서 맨 앞에 * 0.5 + 0.5 까지 더 한 것이 그림자 행렬) 따로 NDC 값으로 정리해줄 필요가 없다. 또한 이 투영 과정은 GLSL 에서 다음 함수로 제공한다.

``` c++
float textureProj( 	sampler2DShadow sampler,
  	vec4 P,
  	[float bias]);
```

> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/textureProj.xhtml

이 함수는 쉐이더를 쓸 때 설정한 고정 함수에 의해서 $ z $ 요소와 비교한 뒤에, 테스트가 성공했는가 실패했는가를 각각 1.0 또는 0.0 으로 반환한다. 만약 대상이 되는 텍스쳐가 `GL_LINEAR` 혹은 멀티 샘플을 수행하고 있을 경우에는 각 샘플에 대해 수행해서 평균을 낸다. 지금 예제의 경우에는 고정 함수가 `GL_LEQUAL` 이므로 $ z $ 값이 버퍼의 $ z $ 값보다 작거나 같으면 1, 아니면 0 을 반환할 것이다.

만약 무아레 현상이 일어나면, $ z $ 버퍼를 갱신하는 쉐이더를 쓸 때 `glPolygonOffset()` 함수를 써서 무아레를 방지하게 할 수도 있다. 

다음은 매우 간략화된 그림자 매핑 프래그먼트 쉐이더의 코드이다.

``` c++
#version 430 core
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2DShadow shadow_tex;

in VS_OUT {
    vec4 shadow_coord;
} fs_in;

void main() {
    diffuse_specular = CalculateColor();
    color = ambient + texutureProj(shadow_tex, fs_in.shadow_coord) * diffuse_specular;
}
```

#### 덤

그림자 맵은 장점과 단점이 공존한다. 각 광원이 **자신만의 $ z $ 맵을 가져야 하기 때문에 메모리 소비가 매우 크다.** 또한 각 라이트는 장면을 따로 렌더링해야하기 때문에 성능에도 영향을 미칠 수 있다. 그리고 그림자 맵의 단일 텍셀이 실제 광원 계산에 수행되는 화면 좌표 상의 정확도를 유지해야 하기 때문에 고해상도의 $ z $ 버퍼 텍스쳐가 필요할지도 모른다. 그렇지 않으면 지글거림 (Shadow acne) 가 일어날 수도 있다. 

*Shadow acne* 가 일어나는 경우에는 `glPolygonOffset` 을 사용해서 지글거림 현상을 제거할 수도 있다. 일단 위 함수를 설정했다면 (스케일 인자를 설정) `glEnable(GL_POLYGON_OFFSET_FILL)` 을 사용해서 효과를 활성화시켜야 한다. 물론 깊이 값을 계산할 때만 설정해야 하며 계산이 끝난 후에는 `glDisable` 을 사용하자.

결과는 Result/Opengl_Sb6/_1217 을 보면 알 수 있다.

### 12.1.8 Fog



## 12.2 Non-Photo-Realistic Rendering (NPR)

지금까지는 현실을 모방한 무언가 그럴싸한 것들을 렌더링해왔다면, 이 절에서 다룰 것들은 연필 스케치 효과나 완전히 추상적인 방식을 사용해서 렌더링할 것이다. 이런 현실적이지 않은 렌더링 기법들을 **NRP** 이라 한다. (약어)

### 12.2.1 Cel Shading 

> Cell 이 아니다!

![cel_shading](https://static.raru.co.za/cover/2016/03/14/4425134-f125-1-l.jpg?v=1458028816)

대부분의 텍스쳐 매핑 예제에서, 2D 텍스쳐를 사용했지만 Cell shading 에서는 대개 ***1차원의 텍스쳐***를 사용한다. 이 방식은 컴퓨터 게임에서 만화 풍으로 렌더링할 지오메트리를 묘화할 때 자주 사용되는 기법이다. 다른 용어로 ***Toon shading*** 이라고 하기도 한다.

이 방식의 주요 요점은 1차원 텍스쳐 맵 (`GL_TEXTURE_1D`) 을 *look-up table* 로 사용해서 텍스쳐 맵에서 읽어온 단일 색상(`GL_NEAREST`) 으로 지오메트리를 칠하는 것이다.

기본 개념은 뷰 공간에서의 **Diffuse Light Intensity** ($ \vec N \cdot \vec L $) 을 1D 텍스쳐 좌표로 사용해서 색상 테이블을 `texelFetch` 혹은 `texture` 로 호출해서 텍스쳐로 참조하는 것이다.

#### 예제

> Chapter12/_1221_cellshading.cc 을 참고한다.

##### startup()

프레임버퍼는 필요없다. 그저 색을 담아둘 `GL_NEAREST` 의 `GL_TEXTURE_1D` 버퍼만 필요하다. 따라서 다음과 같이 색상을 만들고, 텍스쳐를 만들어서 `GL_TEXTURE_1D` 타깃에 바인딩한다.

``` c++
constexpr GLubyte toon_tex_data[] = {
    0x22, _0x00, 0x44, _0x00, 
    0x66, _0x00, 0x88, _0x00, 
    0xAA, _0x00, 0xCC, _0x00,
    0xFF, _0x00,
};

glGenTextures(1, &tex_toon);
glBindTexture(GL_TEXTURE_1D, tex_toon);
constexpr int count = sizeof(toon_tex_data) / 4; // 7.
// GL_RGB8 has 32bits, 
// width : Specifies the width of the texture, in texels (not byte unit, but count).
glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGB8, count);
glTexSubImage1D(GL_TEXTURE_1D, 0, 0, count, GL_RGBA, GL_UNSIGNED_BYTE, toon_tex_data);

// Important (GL_NEAREST)
glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
```

> 대개 툰 쉐이딩을 할 때는 4 색 혹은 7 색 단계의 밝기가 자주 쓰인다고 한다.

##### toonshading.fs.glsl

`texture` 함수를 사용해서 `sampler1D` 의 룩업 테이블을 조회한다. 여기서는 Diffuse 만 고려하기 때문에 간단하게 코드가 작성된다.

``` c++
uniform sampler1D tex_toon;

// Simple N dot L diffuse lighting
float tc = pow(max(0.0, dot(N, L)), 5.0);
// Sample from cell shading texture
color = texture(tex_toon, tc) * (tc * 0.8 + 0.2);
```

만약 Ambient, Diffuse, Specular 까지 전부 다 고려할 예정이라면 회색조의 룩업 테이블만 두고 해당 룩업 테이블을 조회해서 원래 색상에 *multiply* 로 반영하면 툰 쉐이딩을 적용한 퐁 라이팅 모델을 구현할 수 있지 않을까 싶다.

결과는 Results/OpenGL_Sb6/_1221.mp4 에서 볼 수 있다.



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

``` c++
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

* [**`uint packHalf2x16(vec2 v)`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/packHalf2x16.xhtml)

  packHalf2x16 returns an unsigned integer obtained by converting the components of a two-component floating-point vector to the 16-bit floating-point representation found in the OpenGL Specification, and then packing these two 16-bit integers into a 32-bit unsigned integer. The first vector component specifies the 16 least-significant bits of the result; the second component specifies the 16 most-significant bits. 

  * `v` : Specify a vector of two 32-bit floating point values that are to be converted to 16-bit representation and packed into the result. 

이 함수는 4.20 버전부터 지원하기 때문에 4.20 버전 아래인 경우에는 다음 문건을 참고하자. (요건은 그냥 어태치먼트를 늘려서 무식하게 값들을 집어넣는 방법을 쓰자 라는 것)

> https://learnopengl.com/Advanced-Lighting/Deferred-Shading

쉐이더 코드는 다음과 같다. 다만 주의할 점은, outvec0 의 샘플러 타입은 `usampler2D` 이고 outvec1 의 샘플러 타입은 `sampler2D` 이라는 것이다.  outvec0 의 버퍼 픽셀 타입은 `GL_RGBA32UI` 이다.

``` c++
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

``` c++
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[0]);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, gbuffer_tex[1]);
```

기본 모드에서는 `light.glsl` 을 담고 있는 쉐이더를 쓴다.

##### light.fs.glsl -> unpackGBuffer(ivec2 coord, out fragment_info_t fragment)

glsl 에서 함수를 작성할 때, `in` 혹은 `out` 지정자를 인자에 설정해서 포인터와 같은 처리를 수행할 수 있도록 할 수 있다.  아무튼 이 함수에서는 텍스쳐로 가져온 (`GL_TEXTURE_0` `GL_TEXTURE_1`) G버퍼를 언패킹해서 각 픽셀의 정보 저장체에 집어넣는다.

* [**`unpackHalf2x16(uint v)`**](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/unpackHalf2x16.xhtml)

  unpackHalf2x16 returns a two-component floating-point vector with components obtained by unpacking a 32-bit unsigned integer into a pair of 16-bit values, interpreting those values as 16-bit floating-point numbers according to the OpenGL Specification, and converting them to 32-bit floating-point values. The first component of the vector is obtained from the 16 least-significant bits of v; the second component is obtained from the 16 most-significant bits of v. 

  * `v` : Specify a single 32-bit unsigned integer values that contains two 16-bit floating point values to be unpacked. 

실 사용 코드는 다음과 같다

``` c++
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

``` c++
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

![_1231](../Results/OpenGL_Sb6/_1231.png)

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

