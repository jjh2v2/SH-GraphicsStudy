# Chapter 5 : Shading Basics

## 0. Intro

* 3D 차원 공간에서 메쉬가 렌더링되기 위해서는, 메쉬 자체의 정점들의 구성도 중요하지만 **쉐이더에서 어떤 비주얼적인 알고리즘을 거쳐서 렌더링이 되는가**에 대한 모델을 고려하는 것도 매우 중요하다. 예를 들면, 포토리얼리즘스러운 모델을 사용해서 모델을 렌더링하거나, 아니면 NPR (Non-Photorealistic Rendering) 모델을 사용해서 개성이 있는 그래픽 묘화를 하거나 하는 경우가 존재한다.

## 1. Shading Models

* 오브젝트들을 렌더링한 후의 외모를 결정하는 데는 **오브젝트의 색상이 어떻게 결정될 것인가를 결정하는 *Shading Model*** 을 결정하지 않으면 안된다. 대개의 쉐이딩 모델은 *Surface orientation, View direction, Light direction* 을 종속 인자로 받는다.

### Example : Gooch Shading Model

> https://en.wikipedia.org/wiki/Gooch_shading

* Gooch Shading Model 은 NPR 모델의 일종으로, 기본 베이스는 *따듯한 톤의 컬러와 차가운 톤의 컬러를 준비하여, $$ n $$ 과 $$ l $$ 을 비교하여 색을 배치* 하는 것이다. 만약 $$ dot(n, l) $$ 가 0 보다 크다면, 따뜻한 톤의 컬러가 프래그먼트를 이루고, 그렇지 않으면 차가운 톤이 컬러를 이룬다.

$$
\begin{align}
\mathbf{c}_{cool} &= (0, 0, 0.55) + 0.25\mathbf{c}_{surface} \\
\mathbf{c}_{warm} &= (0.3, 0.3, 0) + 0.25\mathbf{c}_{surface} \\
\mathbf{c}_{highlight} &= (1, 1, 1), \\

t &= \frac{(\mathbf{n} \cdot \mathbf{l}) + 1}{2} \\
\mathbf{r} &= 2(\mathbf{n} \cdot \mathbf{l})\mathbf{n} - \mathbf{l} \\
s &= (100(\mathbf{r} \cdot \mathbf{v}) - 97)^{\mp} \\

\mathbf{c}_{shaded} &= s\mathbf{c}_{hightlight} + (1-s)(t\mathbf{c}_{warm} + (1 - t)\mathbf{c}_{cool}) \\
\end{align}
$$

* Note : $$ \mp $$ 는 clamp(0, 1) 을 말함.

* Note : $$ \mathbf{r} = 2(\mathbf{n} \cdot \mathbf{l})\mathbf{n} - \mathbf{l} $$ 은 빛의 방향 벡터 $$ \mathbf {l} $$ 을 표면의 노멀 벡터 $$ \mathbf{n} $$ 에 대한 반사 벡터 $$ \mathbf{r} $$ 을 나타낸 것이다.

## 2. Light Sources

* 일반적으로 Shading 에서 빛을 모델링 할 때, 주광원은 해당 씬의 모델의 렌더링의 전반적인 쉐이딩을 담당한다. 당연히 실제 세계에서는 이렇지는 않고, 간접광원 등의 여러가지 요소가 쉐이딩을 결정하는데 영향을 미친다.
  * `Chapter 9` 에서는 *Photorealistic Shading Model* 이라고 하는 실제 빛의 현상을 모델링해서 현실적으로 쉐이딩을 하고자 하는 기법을 다룬다. 대조적으로 그렇지 않은 쉐이딩 모델은 다양하게, 개성적으로 모델링을 할 수도 있다.
* 빛을 모델에 쉐이딩할 때, 알아야하는 두 번째 사항은 빛의 영향을 받아서 쉐이딩되는 경우와, 빛의 영향을 받지 않았을 때 쉐이딩을 어떻게 처리해서 최종 색으로 결정하게 할 것인가이다. 빛의 영향을 받거나 받지 않는 경우에 대해 다음의 기준을 세워서 나눌 수 있다.
  1. *빛의 거리 (Distance from light)*
  2. 그림자로 인한 차폐
  3. 빛이 표면을 향하고 있지 않은 경우

보통 빛이 $$ n $$ 개 있다고 할 때, 그리고 $$ \mathbf{c}_{light_{i}}$$ 가 각 빛의 주광원 색이고 $$ \mathbf{l}_i $$ 가 빛으로 뻗어나가는 방향이라고 하면,
최종으로 쉐이딩되는 색 $$ \mathbf{c} $$ 는 다음과 같을 것이다.
$$
\mathbf{c}_{shaded} = 
	f_{unlit}(\mathbf{n}, \mathbf{v}) + 
	\sum_{i = 1}^{n} \mathbf{c}_{light_{i}}f_{lit}(\mathbf{l}_i, \mathbf{n}, \mathbf{v})
$$

* $$ f_{unlit}(\mathbf{n}, \mathbf{v}) $$ 는 **빛의 영향을 받지 않을 때의** 해당 모델이 어떻게 보이는가를 나타낸 것이다. 대개 심플화된 모델에서는 $$ f_{unlit}(\mathbf{n}, \mathbf{v}) $$ 는 $$ (0, 0, 0) $$ 으로 완전히 검은 색일지도 모른다. 그렇지만 현실적으로 렌더링하고자 하는 경우, **간접 조명**에서 오는 빛을 모델링하는 데 쓰이기도 한다.
  * 예시로서는 하늘 색의 간접 조명, 혹은 주위의 오브젝트에서 반사되는 간접 조명등이 있다.
    자세한 설명은 `Chapter 10` `Chapter 11` 에서 설명함.
* 단위 면적에 내리쬐는 빛의 밀도 (선속?) 는 $$ (\mathbf{l} \cdot \mathbf{n})^+ $$ 와 동일하다. 위의 모델은 빛이 표면의 법선과 평행할 때의 모델이기 때문에, 엄밀한 최종 색은 다음과 같을 것이다. 밑 모델은 **PBR** 의 기반이 된다.

$$
\mathbf{c}_{shaded} = 
	f_{unlit}(\mathbf{n}, \mathbf{v}) + 
	\sum_{i = 1}^{n} 
		(\mathbf{l_i} \cdot \mathbf{n})^+
		\mathbf{c}_{light_{i}}f_{lit}(\mathbf{l}_i, \mathbf{n}, \mathbf{v})
$$

* 만약 $$ f_{lit}(\mathbf{l}_i, \mathbf{n}, \mathbf{v}) = \mathbf{c}_{surface} $$ 라고 한다면, 밑의 새로 쓰여지는 식은 **Lambertian Shading Model** 이 된다. 
  밑의 식은 보통 *이상적으로 Diffuse 하게 반사되는 표면, 즉 매트 표면에 딱 맞게 적용된다.* 덤으로 이 램버트 쉐이딩 모델은 `Chapter 9` 이후부터 나올 다양한 쉐이딩 모델의 베이스가 된다.

$$
\mathbf{c}_{shaded} = 
	f_{unlit}(\mathbf{n}, \mathbf{v}) + 
	\sum_{i = 1}^{n} 
		(\mathbf{l_i} \cdot \mathbf{n})^+
		\mathbf{c}_{light_{i}}
		\mathbf{c}_{surface}
$$

### 2.1 Directional Lights

* **Directional Light** 는 $$ \mathbf{l} $$ 과 $$ \mathbf{c}_{light} $$ 가 레벨 (씬) 전체에 대해 **고정이다.** 다만 그림자 맵핑에 의해 $$ \mathbf{c}_{light} $$ 는 차폐되어 $$ (0, 0, 0) $$ 이 될 수 있다. Directional Light 는 임의 공간에 대해 실제로는 광원을 가지고 있지만, 추상화되어 **씬 위에서의 실제적인 광원 포인트는 없는** 빛 모델이다.
* 물론 직사광선 역시 Light Volume 을 가져서, 이 Volume 의 경계면에서 `Smoothstep` 과 같은 것으로 $$ \mathbf{c}_{light} $$ 을 조절할 수 도 있다.

### 2.2 Punctual Lights

* **Punctual Light** 는 *Directional Light* 와는 다르게 **씬 위에서의 위치를 가지는 빛**을 통합적으로 말한 것이다. 이 Punctual Light 에서 위치가 존재하며, 구 방사형으로 일정한 빛을 뿜을 (일정하지 않을 수도 있음) 경우, 이 빛을 **Point Light** 라고 한다.
* Punctual Light 에서 빛의 위치를 $$ \mathbf{p}_{light} $$ 이라고 하며, 그리고 빛의 영향을 받을 예정인 표면의 위치를 $$ \mathbf{p}_0 $$ 이라고 할 때, 기호 $$ r $$ 은 다음과 같다.

$$
\begin{align}
	\mathbf{d} &= \mathbf{p}_{light} - \mathbf{p}_0 \\
	r &= \sqrt{\mathbf{d} \cdot \mathbf{d}} = |\mathbf{d}|
\end{align}
$$

#### Point / Omni Light

> https://en.wikipedia.org/wiki/Inverse-square_law

* Punctual Light 에서 구 방사형으로 빛을 고르게 내뿜는 빛 모델을 **Point Light** 라고 한다. 혹은 **Omni Light** 라고 하기도 한다. 이 빛 모델의 특징은, 거리 $$ r $$ 에 따라서 $$ \mathbf{c}_{light} $$ 가 점차적으로 (반비례하게) 변한다는 것이다. 현실 세계에서 빛이 $$ r $$ 만큼 진행하면, 거기에 대한 단위면적 빛의 밀도가 $$ r^2 $$ 만큼 감쇄한다는 것을 나타낸 것이라고 보면 될 것 같다.
* 위의 설명을 밑의 수식으로 표현할 수 있으며, 이 수식이 *Point Light* 의 기본 골자가 된다.
  이 식을 **Inverse-square light attenuation** 이라고 한다.

$$
\mathbf{c}_{light}(r) = \mathbf{c}_{light_0}(\frac{r_0}{r})^2
$$

* 다만 위 식 그대로 쓰기에는 문제점이 존재한다.

  1. ISLA 에서 **$$ r $$ 이 $$ 0 $$ 으로 진행할 때, 최종 결과값이 기하급수적으로 증가한다. 또한 $$ r = 0 $$ 일 경우에는 값은 `INF` 가 될 수도 있다.** 심각한 경우 프로그램의 에러를 일으킬 수 있다. 이 문제를 해결하기 위해 UE4 나 프로스트바이트 엔진과 같은 상용 엔진은 아래 두 개의 솔루션을 사용하고 있다.

     * `UE4` 의 경우 : 분모가 되는 부분에 고정된 미세실수 $$ \epsilon $$ 을 더해서 값이 무한으로 증가하는 것을 막는다. 언리얼 엔진에서는 해당 $$ \epsilon $$ 을 0.01 (1CM) 로 고정해서 사용한다.
       $$
       \mathbf{c}_{light}(r) = \mathbf{c}_{light_0}(\frac{r_0}{r + \epsilon})^2
       $$

     * `CryEngine` `Frostbite` 의 경우 : 광원이 되는 물체 안으로 오브젝트가 투과할 수 없음을 이용해서, 해당 광원의 **물체가 되는 부분의 크기를 $$ r_{min} $$ 으로 설정해서 이를 분모에 더해 사용한다**. 매직넘버가 없음은 장점이지만 조금 더 복잡해진다.
       $$
       \mathbf{c}_{light}(r) = \mathbf{c}_{light_0}(\frac{r_0}{r + r_{min}})^2
       $$
       

  2. ISLA 의 두번째 문제는, $$ r $$ 이 무한으로 진행할 때 일반 수식으로는 $$ \mathbf{c}_{light}(r) $$ 은 절대로 0 이 될 수 없다. 컴퓨터 그래픽스에서 다양한 빛이 존재하는 경우, 최적화 등을 위해 포인트 라이트에는 빛이 도달하는 최대 거리를 지정하는 경우가 많다. 이 때 빛이 도달하는 최대 거리에서 $$ \mathbf{c}_{light}(r) = 0 $$ 이 되기 위해서는 위 식에서 **Windowing Function** 이라 하는 것을 곱해줘서 최대 거리에 빛의 세기가 0 에 도달해야 한다.
     $$
     \begin{align}
     f_{win}(r) &= (1 - (\frac{r}{r_{max}})^4)^{+2} \\
     \mathbf{c}_{light}(r) &= \mathbf{c}_{light_0}f_{win}(r)(\frac{r_0}{r + r_{min}})^2
     \end{align}
     $$

* *ISLA* 을 사용하지 않고, 성능을 보다 높이기 위해 (Inversed-Square 을 쓰지 않는) 더 추상적으로 일반화된 수식을 쓰는 경우도 존재한다. 밑의 수식은 Just Cause 2 에서 극다량의 빛을 더 빠르게 연산하기 위해 쓰여진 식으로,
  $$ f_{dist}(r) $$ 을 **Distance Falloff Functions** 라고 부른다. *DFF* 는 위의 *ISLA* 을 쓸 때 고려해야 할 두 가지의 이슈를 단번에 해결하지만, 덜 정확한 단점이 존재한다.

$$
\begin{align}
	\mathbf{c}_{light} &= \mathbf{c}_{light_0}f_{dist}(r) \\
	f_{dist}(r) &= (1 - (\frac{r}{r_{max}})^2)^{+2}
\end{align}
$$

#### Spotlights

* *Point Light* 에, 빛이 퍼지는 범위 각도 $$ \theta_{u} $$ 가 추가된 형태.
  $$ \theta_{u} $$ 는 **Umbra angle** 이라고 부른다.

  이 *Umbra angle* 을 $$ c_{light} $$ 의 식에 추가하려면, 각도에 따른 Falloff function 인 $$ f_{dir}({\mathbf{l}}) $$ 이 필요하다. 
  즉, 다음과 같은 식이 된다.

$$
\mathbf{c}_{light} = \mathbf{c}_{light_0}f_{dist}(r)f_{dir}(\mathbf{l})
$$

* 다만 엄밀히 말하면 $$ f_{dir}(\mathbf{l}) $$ 는 콘 형태의 Spotlight 외에도 다양한 형태로도 구현이 될 수 있다. 이 함수에는 $$ \mathbf{l} $$ 이 인자로 들어가지만, 실제로는 $$ \mathbf{l} $$ 을 기반으로 한 각 인자 $$ \theta_u , \theta_s , \theta_p $$ 가 사용된다.
  * $$ \theta_u $$ : **Umbra angle** 로, Spotlight 에서 빛의 영향이 아예 없는 경계각을 나타낸다.
  * $$ \theta_s $$ : Spotlight Angle 로, $$ \mathbf{l} \cdot normalize(d_0 - d) $$ 의 현재 쉐이딩할 표면과 빛, 그리고 빛의 중심이 되는 방향과의 각도를 나타낸다. (종속변인)
  * $$ \theta_p $$ : **Penumbra Angle** 로, Spotlight 에서 빛이 Falloff 되는 각도를 나타낸다. 이 각도 안에서는 방향에 따른 빛의 세기는 동일해진다.

따라서 다음과 같이, *Spotlight* 의 최종 $$ f_{dir}({\mathbf{l}}) $$ 을 수식화 할 수 있다.
$$
\begin{align}
\cos\theta_s &= \mathbf{l} \cdot \frac{d_0 - d}{|d_0 - d|} \\
t &= (\frac
	{\cos(\theta_s) - \cos(\theta_u)}
	{\cos(\theta_p) - \cos(\theta_u)})^{\mp} \\
f_{dir_F}(\mathbf{l}) &= t^2 \\
f_{dir_T}(\mathbf{l}) &= smoothstep(t)
\end{align}
$$

* $$ f_{dir_F}(\mathbf{l}) $$ 는 Frostbite 엔진에서 차용한 식이며, $$ f_{dir_T}(\mathbf{l}) $$ 은 Three.js 에서 차용한 식이다.

#### Other Punctual Lights

* 위에서도 있지만 $$ f_{dir}(\mathbf{l}) $$ 은 단순히 콘 형태의 *Spotlight* 을 구현하는데 쓰일 뿐만 아니라, 각종 종속변인으로 다양한 빛을 쉐이딩하게 하는데도 쓸 수 있다. 예를 들면, 각 인자를 사용해서 빛 모양을 내는데 텍스쳐를 입혀 모양을 쓰게 하거나 할 수 있다는 것.
  * 예시로, *Tomb Raider (2013)* `[953]` 에서는 $$ (x, y, z) $$ 축을 사용해서 독자적인 Falloff function 을 사용했다고 한다. 또한 $$ c_{light_0} $$ 을 고정시키지 않고, 곡선 그래프 툴을 사용해 시간의 흐름에 따라 보다 동적인 라이트를 사용하게끔 구현했다고 한다.
    * 구체적인 예로는 횃불이 깜빡인다거나, 빛이 나무들에 가려서 투과되다 말다가 하는 식의...

### 2.3 Other Light Types

* *Directional Light, Point Light, Spot Light* 이외에도 현실 세계의 빛의 모델링을 추상화해서 여러 가지 독자적인 빛을 구현할 수 있다. *Tomb Raider (2013)* 에서는 Point Light 을 좀 더 발전시켜서 Capsule Light 라는 것을 만들었는데, 이 빛 모델은 선형으로 되어서 어떤 물체의 표면과 해당 빛의 광원에서 가장 가까운 지점을 골라, 쉐이딩하게 한다.
* 또한 **Area Light** 라는 것도 존재하는데, 이는 일반적인 Point Light 보다 더 현실적으로 빛을 모델링한 것이라고 한다. 다만 광원과 동시에 표면에 대한 광원 등이 존재하므로 아주 추상화된 Point Light 보다는 연산이 복잡해질 수 있다. 그렇지만 현재 GPU 의 성능이 올라감과 동시에 Area Light 의 연산이 그렇게 느려지지는 않기 때문에 현재는 많이 쓰는 편이라고 한다.

## 3. Implementing Shading Models

* 이론으로 설명한 모든 쉐이딩 모델은 어플리케이션에서 쓰기 위해서는 코드로 작성을 해서 렌더링할 수 있도록 해야한다.

### 3.1 Frequency of Evaluation

* 쉐이딩을 구현할 때, 해당 쉐이딩 모델을 어느정도 빈번하게 재갱신해서 렌더링하게 하는냐를 결정하는 것도 나름 중요하다. 예를 들어서, 어플리케이션 기동시나 아니면 아예 갱신이 되지 않는 값의 경우에는 미리 쉐이더로 베이킹을 해서 값을 결정해서 쓰게하는 경우가 있다

  또는 시간에 따라서 하늘의 빛이 달라지는 경우에, 매 프레임마다 갱신하지 않고 프레임 주기를 둬서, 해당 주기가 됬을 때에만 재갱신을 하게끔 하는 경우도 있을 수 있다.

  보통 플레이어의 카메라 뷰 및 프로젝션 매트릭스는 매 프레임마다 재갱신이 되어야 하는 경우에 속한다. 또한 동적으로 값이 변하는 머터리얼이 있을 때, 이 역시 매 프레임마다 다시 렌더링을 하지 않으면 안된다.

* 일반적인 쉐이딩 렌더링 모델은 *PS (Pixel Shader)* 에서 행하는 경우가 많았지만, 요즘에는 Compute Shader 에서 쉐이딩 모델을 동작시켜서 속도를 더 높이는 경우도 있다고 한다. 자세한 설명은 `Chapter20` 에서 설명...

* *VS* 와 *PS* 간의 선형 보간에 의한 노멀, 혹은 각종 값들의 오차가 생기는 것을 방지하기 위해 보통 길이가 1인 유닛 벡터의 경우 VS 의 출력에 값을 넣을 때 한번, 그리고 PS 에서 입력으로 가져올 때 다시 한번 노멀라이즈를 한 후에 사용하는 것이 좋다고 한다.

  * 예외도 존재함. 무조건 평준화를 해서 입출력 값을 갱신하고자 하면 안됨.

* 벡터들을 계산할 때, 해당 벡터의 좌표계를 통일하면 더 좋다.

## 4. Aliasing and Antialiasing

* 메쉬 혹은 삼각형 메쉬가 3D 스크린에 보일 때, 3D 스크린의 정보들을 2D 스크린에 렌더링 해야한다. 이 때 복잡한 메쉬의 외곽선이 자글자글해보이거나 텍스쳐 자체가 무아레 현상 등을 일으키는 경우를 **Aliasing (에일리어싱)** 이라고 한다.
* 해당 *Aliasing* 문제를 해결하는 여러가지 기법들을 통틀어서 **Antialiasing** 이라고 한다.

### 4.1 Sampling and Filtering Theory

* 이미지를 렌더링하는 작업은 어떻게 보면 어떤 신호들을 샘플링해서 다시 필터링하는 작업과 같다. 왜냐하면 이미지를 만들어내는 작업 과정이, 3차원 공간의 복잡한 장면들에서 이미지의 각각의 픽셀에 필요한 색들을 얻기 위한 일련의 샘플링 과정이기 때문이다.
* **Sampling** 의 최종 목표는, 아날로그 (혹은 아날로그로도 볼 수 있는 여러 원 데이터들을) 데이터를 이산적으로 분류해서 나타내는 것이다. 이렇게 함으로써 불필요한 정보들을 걸러낼 수 있으며, 해당 데이터를 가지고 원상복구하는데 필요한 데이터들만 추출할 수 있게 된다.
  * Sampling 을 할 때, 아날로그의 데이터들이 이산 데이터로 변환되기 때문에 *Aliasing* 은 피할 수 없다.
    그런데 *Aliasing* 은 아날로그 데이터를 어떤 방식으로 샘플링하느냐에 따라서도 강하게 나타나는가 아니면 적게 나타나는가를 조정할 수 있다. **Aliasing 은 샘플링 되는 신호의 주파수가 낮을 수록 강하게 일어난다.**
* **Filtering** 은 샘플된 이산적인 데이터들을 다시 아날로그 혹은 원 데이터와 같은 형태로 복구하는 작업을 말한다. 

* **Sampling 을 할 때, 정보를 온전히 보존하려면 샘플링하는 주파수는 원 데이터의 최대 주파수의 2배보다 더 커야한다.** 이를 **Nyquist rate (limit)** 라고 한다. 그래픽스에서 흔히 볼 수 있는 예로는, 체크무늬의 텍스쳐가 바닥에 깔려있을 때 멀리 있는 체크무늬의 텍스쳐는 한 픽셀에 담을 수 있는 양이 너무 많은 나머지 (해당 픽셀이 가지는 주파수가 커진다는 것으로 봐도 됨) 프레임버퍼의 고정된 샘플링 레이트보다 높아져 에일리어싱이 일어나는 예가 있다.
  * 다만 일반 3차원 공간은 메쉬가 특정 픽셀에 위치하는 경우가 주기적이지 않고 이산적이기 때문에, 3차원 공간 자체를 샘플링할 때 Aliasing 을 완전히 없애는 방법은 존재하지가 않는다. (이산적인 경우는 주파수가 무한이다)

#### Reconstruction

* Sampling 된 이후의 이산적인 데이터는, 정보가 충분하면 *Filtering* 을 통해서 원 데이터로 복구할 수 있다. 이산화된 데이터를 아날로그 등으로 복구하기 위해 여러가지 필터가 존재한다.
  * *Box Filter*, *Low-pass Filter*, and *Tent Filter...* etc.
  * 실제로는 위 3개의 기본 필터를 섞어서 사용하는 식으로 쓴다고 한다.
* 컴퓨터 그래픽스에서는 Filtering 된 데이터들을 다시 이미지에 써넣지 않으면 안되기 때문에, 재샘플링을 실시한다.

#### Resampling

* *Filtering 된* 이산화된 데이터들을 재샘플링할 때, 원 주파수보다 더 높은 주파수로 샘플링 하는 것을 **Magnification ** 이라고 하고, 낮은 주파수로 샘플링하는 것을 **Minification** 이라고 한다. 다만 필터링된 데이터를 기반으로 Minification 을 그대로 실행하면, Nyquist rate 에 의해 Aliasing 이 발생되기 때문에, Minification 을 할 때는 **이산화된 데이터에서 a 만큼 나눈 값을 종속변인으로 하여 필터링을 실시하고, 샘플링해야 한다.**
* 디지털 이미지 혹은 그래픽스로 예를 들자면, Filtering 은 이미지를 블러링하는 작업이라고 볼 수 있으며, Resampling 은 해상도를 줄이거나 블러링된 이미지를 토대로 다른 이미지에 최종 결과물을 렌더링하는 것이라고 볼 수 있다.

### 4.2 Screen-Based Antialiasing

* 컴퓨터 그래픽스에서 *Antialiasing* 하는 방법들은 전부 다 **스크린 영역을 기반**으로 한다는 점에서 공통점을 가진다.

* MSAA, FXAA, FSAA, MLAA, SMAA, QAA, TAA, TMAA 등의 여러가지 안티에일리어싱 방법이 있지만 전부 다 장단점이 존재하고 성능이 좋으면서도 메모리를 먹지 않고 빠른 방법은 존재하지 않는다. (No silver-bullet)

* *스크린 기반의 안티에일리어싱* 의 기반이 되는 전략은, 스크린에 대해서 샘플링 패턴을 써서 각각의 샘플에 대해 *Weight* 을 곱해 최종 픽셀 색상인 $$ \mathbf{p} $$ 을 계산하는 것이다.

  $$ w_i $$ 는 픽셀의 $$ i $$ 번째 샘플의 weight 이다.
  $$ \mathbf{c}(i, x, y) $$ 는 픽셀의 $$ i $$ 번째 샘플 (서브픽셀이 될 수 있음) 의 색상이다.

$$
\mathbf{p}(x, y) = \sum_{i=1}^{n}w_i\mathbf{c}(i, x, y)
$$

* 픽셀 당 하나보다 더 많은 샘플을 두어 계산하는 알고리즘을 **SuperSampling (SSAA) 혹은 FullScene (FSAA)** 라고 한다. 이 방법은 샘플 수만큼 최종 결과로 렌더링되는 이미지의 사이즈보다 큰 사이즈의 버퍼에 렌더링을 하게 하고, 렌더링된 버퍼를 블러링해서 $$ \mathbf{p}(x, y) $$ 을 계산한다.

  * 가장 간단한 방법이지만, 샘플 수 만큼 해상도를 배로 잡아먹는 메모리 이점이 없는 단점이 있다.
  * NVIDIA 에서는 SSAA 을 개량한 *dynamic super resolution* 이 존재한다.
  * *SSAA* 는 **Accumulation buffer** 에서 아이디어를 얻었다. A-buffer 을 사용한 안티에일리어싱 방법으로는 해상도는 같지만, 샘플마다 이미지 버퍼를 따로 두거나, 아니면 각 픽셀이 저장할 수 있는 비트를 크게하여 offset 된 이미지들의 각 픽셀의 값을 저장하여 블렌딩하는 방법이다. 다만 이 방법은 같은 메쉬를 $$ n $$ 번 반복해서 렌더링해야 하는 단점이 있기 때문에 (모던 API 라면 동시에 쓰는 작업이 가능할지도) 리얼타임 어플리케이션에서는 선호되지 않는 방법이라고 함.

* **MSAA (Multi-Sampling Antialiasing)** 은 SSAA 보다는 복잡하지만 포워드 렌더링에서 합리적인 비용으로 안티에일리어싱을 할 수 있는 방법 중 하나다.

  * MSAA 는 렌더링 되는 메쉬의 프래그먼트에 대해 픽셀 당 한번만 샘플링 과정을 거친다. 샘플링 과정은 약간 특이한데, *Latin hypercube* 형태로 가상의 샘플 포인트가 있고, 메쉬의 프래그먼트가 어느정도 픽셀을 덮고 있는가에 따라서 **중심을 기준으로 계산되거나, 아니면 가상의 샘플 포인트 중 하나를 기준으로 계산된다.**

    자세한 이해는 `Figure 5.26` 을 볼 것.

  * 만약에 어느 픽셀에 프래그먼트가 하나밖에 존재하지 않고, 그 프래그먼트 마저도 중심을 덮지 않는다면, GPU 에 따라 *Centroid Interpolation* 이 적용되어 해당 프래그먼트가 중심점을 가지게 될 수도 있다. 이를 사용해서 메쉬의 외곽선이 도중에 끊기거나 하는 *off-triangle problem* 을 막을 수 있지만, 반면에 의도치 않은 결과를 유발할 수도 있다. `[530, 1041]`

  * MSAA 의 확장된 예로서는 CSAA, EQAA, CFAA `[1625]` 등이 존재한다.

* MSAA 을 포함한 샘플링을 여러번 해서 안티에일리어싱을 하는 경우, 색상의 블렌딩이 일어나는데 해당 안티에일리어싱이 일어나기 전 **HDR 값을 가진 버퍼가 블렌딩되지 않도록 해야한다.** 따라서 HDR 을 쓰고자 한다면, 우선 후처리를 통해서 HDR 버퍼를 톤맵핑을 사용해 $$ [0, 1] $$ 사이의 컬러 값으로 변환한 다음, 안티에일리어싱을 사용해야 한다.

* **TXAA** 와 **MFAA (Multi-frame antialiasing)** 은 **TAA (Temporal Antialiasing)** 을 사용한다. MFAA 는 NVIDIA 에서 GeForce 600 번대의 드라이버부터 지원이 가능하도록 한 안티에일리어싱 방법이라고 한다. *TAA* 라고 불리는 안티에일리어싱 방법은 이전 프레임들의 이미지와 현재 이미지를 *Weight* 을 사용해서 블렌딩하는 것을 기반으로 하고 있다.

  * *TAA* 을 사용할 때, 장점으로는 이미 다 만들어진 이미지를 사용해서 안티에일리어싱을 하기 때문에 디퍼드 렌더링에서도 효과를 볼 수 있다는 점이 있으며, 저해상도의 이미지들을 사용해서 블렌딩한 후, 이를 디스플레이에 업스케일링해도 그럴싸한 퀄리티를 유지할 수 있다는 점이 있다. `[382, 836, 1405, 862]`
  * *TAA* 의 단점으로는, 어디까지나 시간 축에 대해서 안티에일리어싱을 하는 꼴이기 때문에 갑자기 장면이 전환되거나 카메라가 전환 될 때는 고스트 현상이 일어날 수 있다. 다만 이 경우 *Weight* 을 현재 프레임만 쓰도록 하거나, 아니면 **Velocity Buffer** (`Section 12.5`) 을 사용해서 이전 프레임의 물체가 어디였는가를 파악하거나, 아니면 없는 경우 discard 해서 선택적으로 TAA 을 적용할 수도 있다. `[1110]`

#### Sampling Patterns

* 단순히 어떤 방식으로 안티에일리어싱을 구현하는 가 말고도, 픽셀 안의 샘플을 어떤 방식으로 샘플링해야 하는가도 매우 중요하다. Naiman `[1257]` 은 인간의 눈은 거의 수평하거나 수직인 선이 있을 때 Aliasing 을 가장 느낀다고 한다. 그리고 그 다음으로 45도로 경사진 선이 있을 경우 두 번째로 Aliasing 을 느낀다고 한다.

  * **Rotated Grid SuperSamping (RGSS)** 는 샘플들을 거의 45도 회전시킨 정사각형 패턴으로 샘플링하게 해서 안티에일리어싱을 해결하고자 했다.

    RGSS 는 **Latin hypercube (N-rooks sampling)** 의 형태로 픽셀의 샘플을 위치해서, 그리드 안의 샘플들의 행과 열이 서로 겹치지 않도록 했다고 한다. 이렇게 함으로써 거의 수평이거나 수직인 선에 대해 안티에일리어싱이 잘 먹혀들게 했다고 한다.

* 다만, 샘플들의 위치를 정할 때에는 픽셀 안의 서브픽셀 그리드에 대해, 샘플들이 고루고루 분포할 수 있도록 해야한다. 그렇지 않으면 특정 케이스에서 안티에일리어싱이 전혀 먹혀들지 않는 상황도 발생할 수 있다. `Figure 5.27`

  * 해당 샘플들의 위치를 결정하는 방법으로는 *Stratified Sampling* *,Stochastic Sampling* *,Interleaved Sampling* 등이 있다. `[1413, 1758]`

#### Morphological Methods

* 대개 AA 는 메쉬들의 외곽선들의 지터링을 보간해서 해결하기 위해 사용하는 경우가 대다수이다. (텍스쳐 자체의 지터링을 해결하기 위해서는 대체로 이등방성 필터링을 쓴다.) 다만 Morphological 한 AA 방법은, 후처리 연산을 통해서 외곽선의 구조들을 파악한 후, 구조에 따라 부분적으로 블러링을 먹여서 AA 을 구현한다.
* 2009 년, Reshetov `[1483]` 의 **Morphological Antialiasing (MLAA)** 와, 이 AA 방법의 연산량을 줄여서 게임과 같은 실시간 렌더링 프로그램에서 쓸 수 있도록 한 **Subpixel Morphological Antialiasing (SMAA)** `[820, 830, 834]` 가 존재한다.
  * Jimenez `[836]` 은 보다 더 개량된 SMAA 구현을 했다고 한다. 그가 구현한 방법은 FXAA 보다 빠른 경우가 존재한다고 한다.

## 5. Transparency, Alpha and Compositing

* **Screen-Door Transparency `[1244]`** 는 픽셀 정렬된 아주 작은 체커보드 패턴의 텍스쳐를 사용해, 메쉬의 머터리얼의 투명치에 따라서 패턴의 픽셀을 *Threshold* 로 두어 프래그먼트를 Discard 하는 방식으로 **유사하게** 반투명을 구현하는 방법이다. 이 방법은 동시에 여러 물체에 해당 기법을 쓰게 되면 반투명되는 물체가 다른 반투명인 물체에 겹쳐보이게 되는 문제점이 있지만, 매우 간편하고 불투명한 물체에 간단히 쓰일 수 있기 때문에 자주 사용되는 방법이다.
  * 이 기법에서 사용된 체커보드 패턴의 텍스쳐와 같이, 어떤 값에 의해서 텍셀들이 경계점으로 쓰일 수 있는 텍스쳐를 **Cutout Texture** 라고 한다. 또한 이 *Cutout Texture* 을 사용해서 픽셀이 아닌 Subpixel 단위에서 안티에일리어싱을 구현하게끔 한 것을 **Alpha to Coverage** (`Section 6.6`) 기법이라 한다.
* 위 기법을 제외한 대다수의 투명 물체 렌더링 알고리즘은 투명한 오브젝트의 색상과 뒷편의 불투명한 오브젝트의 색상을 섞어서 렌더링을 한다. 이 때 사용되는 컨셉이 **Alpha blending** 인데, Alpha $$ \alpha $$ 는 보통 색상의 투명도를 나타내지만 혹은 해당 프래그먼트의 픽셀에 대한 점유도 (Coverage) 을 나타낼 수도 있다는 것을 명심하자.
  * 픽셀의 알파 $$ \alpha $$ 는 상황에 따라 투명도 혹은 픽셀의 점유도를 나타낸다.

### 5.1 Blending Order

* 프래그먼트의 색상을 섞을 때는 대개 $$ \mathbf{over} $$ operator 을 사용한다.
  $$ \mathbf{over} $$ operator 는 다음과 같다.

  각 문자의 첨자 $$ s $$ 는 현재 렌더링되는 색을 나타내며, $$ d $$ 는 이미 버퍼에 렌더된 (블렌딩된) 색을 나타낸다.

$$
\mathbf{c}_o = \alpha_s \mathbf{c}_s + (1 - \alpha_s) \mathbf{c}_d
$$

* 다만 $$ \mathbf{over} $$ operator 을 사용해서 단순히 반투명을 구현하려면 매우 부정확할 뿐 더러, 어느 반투명 물체가 먼저 렌더링되느냐에 따라 최종 색이 달라질 수도 있다. 이 경우 자료구조를 사용해서 카메라의 $$ far $$ 에 가까운 반투명 물체부터 가까운 반투명 물체까지 순서대로 정렬한 뒤 렌더링하는 방법이 존재한다. (물론 성능은 장담못함)
* **Additive Blending** 은 $$ \mathbf{over} $$ operator 의 변형으로, 색을 블렌딩하는 것이 아니라 단순히 이미 렌더된 색 위에 또 다른 색을 추가로 얹는다. 대개 광원이 빛나는 효과나 파티클의 스파크 등에 적용할 수 있다. 그렇지만 실제 현실에서는 광원의 빛에 의해 주위의 색의 채도가 높아진다거나 하는 부수효과가 있기 때문에 해당 식이 완전하다고는 못함. `[1192, 1273, 1813]`

$$
\mathbf{c}_o = \alpha_s \mathbf{c}_s + \mathbf{c}_d
$$

* 반투명한 물체의 프래그먼트가 앞에서 부터 뒤로 렌더링될 때 사용할 수 있는 operator 로 $$ \mathbf{under} $$ operator 가 있다. 이 operator 는 $$ \mathbf{c}_o $$ 은 순서에 독립적이지 않지만, $$ \mathbf{a}_o $$ 은 순서에 독립적이다.

  주의해야 할 점은 위의 식과는 다르게 $$ \mathbf{c}_d $$ 가 Source 가 되며 $$ \mathbf{c}_s $$ 가 Destination (버퍼) 가 된다는 점이다.
  또한 $$ \mathbf{a}_o $$ 은 Source 와 Dest 두 프래그먼트의 Coverage 을 합친 꼴로 볼 수 있다. (이전에 Alpha 가 일반 투명도 외에도 Coverage 로 작용할 수 있다는 것을 기억하자고 했음) `Figure 5.34`

$$
\begin{align}
\mathbf{c}_o &= \alpha_d \mathbf{c}_d + (1 - \alpha_d) \alpha_s \mathbf{c}_s \\
\mathbf{a}_o &= \alpha_s - \alpha_s \alpha_d + \alpha_d
\end{align}
$$

### 5.2 Order-Independent Transparency

* $$ \mathbf{under} $$ operator 는 $$ \mathbf{over} $$ operator 와 같이 쓰일 수 있다. 예를 들면, 반투명한 오브젝트들을 렌더링할 때 별도의 컬러 버퍼에 반투명한 오브젝트들의 블렌딩된 컬러를 *under* 을 사용해서 최종 결정하고 그 후에 *over* 을 사용해서 불투명한 오브젝트의 컬러 버퍼의 색상과 섞어서 최종 색을 계산해낼 수 있다.

  또는 **Depth Peeling, Order Independent Transparency** `[449, 1115]` 라고 알려진 알고리즘을 사용해서 반투명한 물체를 렌더링할 때도 $$ \mathbf{under} $$ 가 쓰인다.

* *Order-Indenpendent Transparency* 기법은 CPU 혹은 GPU 단에서 반투명한 물체를 소팅할 필요가 없는 알고리즘을 통틀어서 말하는 기법들이다. 이 기법들 중 *Depth-Peeling* 은 불투명한 물체들의 z 버퍼와 반투명한 물체의 z 버퍼, 그리고 반투명한 물체의 색상을 담는 버퍼로 구성된다.

  * *Depth-Peeling* 은 쉐이더 단에서 **불투명한 물체의 z 버퍼에 가장 가까운 z 값을 가지는 반투명한 물체를 뒤에서 부터 앞으로 차례차례 한 겹씩 벗겨나가면서 $$ \mathbf{under} $$ operator 을 사용해 색상을 블렌딩한다.** 다만 이렇게 물체를 렌더링할 경우에는 고정된 횟수로 몇 겹을 렌더링할 것인지, 아니면 UAV 나 OpenGL 의 Shader Storage Buffer 등을 사용해서 임의의 반투명한 물체가 렌더링 되었는가 안되었는가를 확인하고 렌더링 파이프라인을 끝내야하는 약간의 번거로움이 존재한다.
    * Atomic Counter 을 사용해도 될 것 같기도 하다...
      http://www.lighthouse3d.com/tutorials/opengl-atomic-counters/
  * 일반 $$ \mathbf{under} $$ operator 을 사용할 때에는, 공식의 특성에 따라 가장 맨 먼저 렌더링 된 색상이 점차적으로 비율이 적어지기 때문에 뒤에서부터 앞으로 렌더링하는 것을 추천한다.

* **Dual Depth Peeling** 은 Bavoil 과 Myers 에 의해서 `[118]` 개발된 Depth-Peeling 의 변형으로, **앞과 뒤를 동시에 렌더링해가는 식**으로 반투명한 물체를 렌더링한다. 이렇게 함으로써 렌더링 되는 패스의 수를 이론상 반으로 줄일 수 있다.

  * Liu et al. `[1056]` 에서는 GPU 을 사용한? 버킷 소트를 이용해서 Dual Depth Peeling 을 싱글 패스에 32개의 레이어를 동시에 렌더링하게 하는 방법을 개발했다고 한다. 물론 이 경우 상당한 메모리를 소모하는 단점이 있다...

* **Weighted Sum** 또는 **Weighted Average** `[118]` 은 싱글 패스로 돌아가며, OIT 한 렌더링 방법이다. **이 방법은 투명도가 높은 물체들에 대해서는 꽤 그럴싸한 반투명 렌더링 결과를 보이지만**, 투명도가 낮아질 수록 두 방법이 사용하는 버퍼 색 결정의 공식의 특성 상 잘 동작하지 않는다. 또한 *Depth-Peeling* 과는 다르게 불투명도가 $$ 1.0 $$ 일 경우에도 불투명하게 렌더링이 되지 않고, 역으로 색이 이상하게 출력되거나 반투명인 것처럼 출력될 수 있다.

  * Weighted Sum 의 공식은 다음과 같다.
    $$ \mathbf{c}_d $$ 는 Opaque 한 버퍼의 색상이다.
    $$
    \mathbf{c}_o = 
    		\sum_{i = 1}^{n}(\alpha_i \mathbf{c}_i)
    	+	\mathbf{c}_d (1 - \sum_{i = 1}^{n} \alpha_i)
    $$
    위 공식에서 짐작이 갈 수 있듯이, 만약 $$ \alpha_i $$ 의 합이 $$ 1 $$ 을 넘어버리는 경우가 오면 불투명 색상은 마이너스 값이 되버리는 치명적인 결함이 존재한다.

  * Weighted Average 의 공식은 다음과 같다. 평균을 구해서 색을 반영하는 방법은 기존 Sum 의 결함을 해결하고자 착안되었다고 봐도 될 것 같음.
    $$
    \begin{align}
    		\mathbf{c}_{sum} &= \sum_{i = 1}^{n}(\alpha_i \mathbf{c}_i), 
    	\ \alpha_{sum} = \sum_{i = 1}^{n} \alpha_i \\
    		\mathbf{c}_{wavg} &= \frac{\mathbf{c}_{sum}}{\alpha_{sum}},
    	\ \alpha_{avg} = \frac{\alpha_{sum}}{n} \\
    	u &= (1 - \alpha_{avg})^n \\
    	\mathbf{c}_o &= (1 - u) \mathbf{c}_{wavg} + u \mathbf{c}_d
    \end{align}
    $$

* **Weighted Blended OIT** 는 Average 의 결함인, 먼 프래그먼트와 가까운 프래그먼트의 알파값이 동일하다고 할 때 가까운 프래그먼트의 색상이 더 돋보이지 않고 평준화에 의해 모든 반투명 프래그먼트가 동일한 Weight 을 가지게 된다는 단점을, $$ z $$ 값에 따라 가중치를 둠으로써 해결한 방법이다. `[1176, 1180]`.

* 아쉽게도 현재 나와있는 반투명 렌더링 방법 중에 완벽하게 반투명 물체를 렌더링할 수 있는 방법은 존재하지 않는다. `[1931, 1141]` 을 보자.

## 6. Display Encoding

* 쉐이더에서 빛, 텍스쳐 및 다른 여러가지 작업을 할 때 쓰는 값들은 대개 **선형적**임을 가정하고 작업하는 경우가 많다. 다만 텍스쳐 혹은 색상을 저장하는 버퍼가 **비선형적**인 색상을 쓰고 있을 경우에는 쉐이더의 결과물이 예상한 것과 다를 수 있다.
  * 이 문제를 해결하려면, 텍스쳐 혹은 버퍼가 sRGB 혹은 이미 **Gamma Correction** 된 색상을 쓰고 있을 때 $$ 1 / 2.2 $$ 을 승산해주고 작업하면 된다. (Decode 작업을 하면 된다)
* 왜 이렇게 비선형과 선형 컬러 공간이 구별되었냐면, LCD 이전에 쓰던 CRT 음극선관이 빛 에너지를 사용해서 세기를 구현할 때의 비선형성 때문이다. CRT 의 음극선관에서 방출되는 빛의 세기는 실제 모니터의 픽셀의 빛의 세기의 상관관계를 따졌을 때 **인간의 눈의 빛의 민감도와 거의 비슷**하다. 이를 **Perceptually Uniform** 이라고 한다.
* **Display Transfer Function** 은 디스플레이 버퍼의 이진적인 값들과 디스플레이에서 실제 출력되는 빛의 세기 간의 관계를 나타낸 것이다. 이 *DTF* 는 다른 말로 **EOTF** 라고 부르기도 한다. 이 *EOTF* 는 하드웨어, 모니터마다 각각 다를 수 있다.
  * **OETF** 라고 이미지 및 비디오 캡쳐 장치에서 표준화된 *DTF* 가 존재한다. `[672]`
* 개인용 컴퓨터에서 사용하는 *DTF* 는 **sRGB** 라고 하는 표준화된 컬러 영역에 의해 정의된다. GPU 을 제어하는 대부분의 API 는 텍스쳐 혹은 컬러 버퍼에 읽고 쓸 때 자동으로 *Gamma Correction* 을 해서 읽고 쓰도록 기능을 제공하고 있다. 텍스쳐 혹은 버퍼가 어떻게 색상 영역이 Decode 되고 Encode 되는지는 `Figure 5.39` 을 참고하자.
  * 우연히도 sRGB 의 Decode / Encode 함수의 곡선 그래프와 감마 값 $$ \gamma = 2.2 $$ 에 대한 De/Encode 함수의 곡선 그래프는 거의 비슷하다. 다만 이 감마 값은 장치마다 다를 수 있는데 대표적인 예로 모바일에서는 $$ \gamma = 2 $$ 이 경우도 있다고 한다. `[1666]`
* 감마 값 $$ \gamma $$ 에 대한 색상 영역의 Decode 및 Encode 함수는 다음과 같다.
  감마 값을 조정하지 않아서 생기는 문제는 `Figure 5.40` `Figure 5.41` `Figure 5.42` 을 볼 것.

$$
\begin{align}
y &= \text{encode} \ f^{-1}_{display}(x) = x^{1/\gamma} \\
y &= \text{decode} \ f_{display}(x) = x^{\gamma}
\end{align}
$$

