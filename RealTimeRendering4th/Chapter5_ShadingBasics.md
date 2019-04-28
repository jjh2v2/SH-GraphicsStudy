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

