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

