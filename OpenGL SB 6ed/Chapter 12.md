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