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

따라서 노멀, 탄젠트 및 바이탄젠트를 사용해서 일반 좌표계의 있는 벡터를 **Tangent space** 로 변환할 수 있는 매트릭스를 만들 수 있다.
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

