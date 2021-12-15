**Malloc Lab**
===

**목표**
===


> 자신만의 malloc, realloc, free를 구현해본다.
> 

💡 WEEK06: 시스템 콜, 데이터 세그먼트, 메모리 단편화, sbrk/mmap
<br>

`implicit` 방법부터 시작(후에 시간이 되면 explicit, seglist, buddy system까지도 구현)


<br>

**구현 함수**
===

## **mm.c**

```c
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
```

## **Goal**

- implicit 방법을 이용해 `./mdriver` 가 정상 작동하도록 코드를 완성하기

<br>
<br>

**핵심 주제**  
===  
<br>

**시스템 콜**
===

> 시스템 콜은 응용 프로그램이 **OS(특히 커널)가 제공하는 서비스에 접근하기 위한** 상호작용이다.
> 
<br>

**운영 체제의 두 가지 모드**
---

💡 운영 체제, 특히 커널은 메모리나 하드웨어 등 중요한 컴퓨터 시스템 자원을 관리한다. 따라서 운영체제는 사용자가 중요 자원에 접근할 수 없도록 두 가지 모드를 구분한다.

<br>

- **유저 모드**
    
    > 사용자가 응용 프로그램(사용자 코드)을 실행하는 모드
    > 
    
    유저(사용자)가 접근할 수 있는 영역을 제한적으로 두고, **메모리나 하드웨어 등 프로그램의 자원에 함부로 침범하지 못하는** 모드이다.
    
- **커널 모드**
    
    > 운영체제 코드를 실행한다. **모든 컴퓨터 하드웨어 자원에 접근할 수 있다.**
    > 
    
    컴퓨터 환경에 치명적 영향을 끼칠 수 있는 명령을 **특권 명령**이라고 하는데, 특권 명령은 오로지 커널 모드에서만 실행된다. 따라서 유저 모드에서는 상대적으로 안전하게 작업할 수 있는 것이다. 이를 **Dual-Mode Operation**이라 한다.
    
<br>

만약 응용 프로그램이 하드웨어 등 시스템 자원을 활용하고 싶다면, **무조건 시스템 콜을 OS에 보내 커널의 서비스를 이용해야 한다.**

만약 시스템 콜이 호출되면 프로그램은 유저 모드에서 커널 모드로 전환(Context Switching)**되고, 명령 수행이 끝나면 다시 유저 모드로 복귀**(Context Switching)한다.

<br>

**동적 메모리 할당**
===

> 추가적인 가상 메모리를 런타임에 획득할 필요가 있을 때 동적 메모리 할당기를 이용해 **힙heap**에서 동적 메모리를 할당받는다.
> 

💡 할당기는 힙을 다양한 크기의 **블록들의 집합**으로 관리한다.


<br>

**왜 동적 메모리 할당을 활용하는가?**
---

💡 프로그램을 실행시키기 전에는 자료 구조의 크기를 알 수 없는 경우들이 많기 때문

![동적 메모리 할당의 예](./image/Untitled.png)



<br>

**동적 메모리 할당기의 유형**
---

- **명시적 할당기 Explicit Allocator**
    
    > Application이 **직접 메모리를 할당하고 또 반환한다.**
    > 
    
    C 표준 라이브러리의 **malloc 패키지(malloc()과 free())**
    
- **묵시적 할당기 Implicit Allocator**
    
    > Application이 메모리를 할당하지만 **반환은 할당기가 자동으로 처리한다.**
    > 
    
    Garbage Collector라고도 부르며, 자바 같은 상위수준 언어들은 자동으로 사용하지 않은 Allocated block들을 반환하는 방식을 사용하는데, 이를 **garbage collection**이라고 한다.
    
<br>

**malloc 패키지와 skbr, mmap**
---

- __void* malloc(size_t size)__
    
    > 성공 시, **적어도** size만큼의 메모리 블록들을 **초기화하지 않고** 리턴한다.
    > 
    - x86(32비트 모드)은 **8 byte(2 word)**, x86-64 컴퓨터는 16 byte(4 word)의 **정렬된 블록들을 기본 단위로 리턴한다.**
    - **mmap이나 munmap 또는 sbrk 함수를 이용해 명시적으로 힙 메모리를 할당하거나 반환한다.**

- __void free(void* p)__
    
    > 할당된 블록의 시작을 가리키는 주소를 인자로 받아 반환한다.
    > 
    - 주소 p는 **무조건 malloc, calloc, realloc에서 먼저 할당받은 상태여야** 한다.
- **calloc()**
    
    > 0으로 초기화된 블록들을 동적할당 받는다.
    > 
- **realloc()**
    
    > 사전에 할당받은 블록들의 사이즈를 줄이거나 늘린다.
    > 
- **mmap 함수(memory mapping)**
    
    > 커널에 새 가상 메모리 영역을 생성해줄 것을 요청하는 함수.
    > 
    
    ```python
    #include <unistd.h>
    #include <sys/mman.h>
    
    void* mmap(void* start, size_t length, int prot, int flags,
    					int fd, off_t offset);
    ```
    
	<p align="center">
		<img src = "https://user-images.githubusercontent.com/93521799/146009225-4793d73b-2b2e-421f-bf68-d7b5cebc0902.png" width="400" height="300"/>
	</p>



- **sbrk() : space brak함수**
    
    > 힙의 크기를 늘리거나 줄이는 함수
    > 
    
    ```python
    #include <unistd.h>
    
    void* sbrk(intput_t incr);
    ```
    
    커널의 brk 포인터(힙의 맨 끝을 가리키는 포인터)에 incr만큼의 크기를 더해서 힙을 늘이거나 줄인다.
    
    성공 시 **이전의 `brk` 값 리턴**
    
    실패 시 **-1 리턴** 밑 `errno = ENOMEM`으로 설정
    
    `incr`가 0이면 **현재의 `bkr`의 값**을 리턴

	<br>

**할당기 처리량 Throughput과 메모리 최고 이용도 Peak Memory Utilization**
===

**할당기 처리량**
---

> 이 할당기가 얼마나 많은 요청을 처리하는가?
> 

**단위 시간 당 완료되는 요청의 수**를 의미한다. 1초에 500번의 할당 연산과 500변의 반환 연산을 할 수 있다면 초당 1000연산의 할당기가 된다.

<br>

**메모리 최고 이용도**
---

> 힙을 얼마나 효율적으로 사용하고 있는가?
> 

**처리량** R0, R1, R2, ... , Rn-1의 총 n번의 할당과 반환 요청에 대해

- **Hk** : k+1번의 요청 이후의 힙 크기. 항상 증가한다고 가정한다.
- **Pk** : k+1번의 요청까지에서의 payload의 합. Allocate면 증가, free면 감소한다.
- **Uk** : 첫 번째 k 요청에 대한 메모리 최고 이용도.
- **maxPi** : k보다 낮은 모든 i에 대해 지금까지의 payload 합 중 가장 큰 값. 최대한으로 많이 할당한 데이터의 값.

Uk를 다음과 같은 수식으로 만들 수 있다. 

**메모리 최고 이용도 = (최대한으로 많이 할당한 데이터 크기)/(힙 크기)**

<p align = center>
    <img src="https://render.githubusercontent.com/render/math?math=U_{k} = {max_{i<=k} P_{i}}/{H_{k}}">
</p>

만약 해당 할당기의 **Uk가 1**이면, 다른 overhead 없이 순수하게 payload로만 이루어진 힙을 지닌다는 의미이다. 즉, 이루어질 수는 없겠지만, **할당기가 가질 수 있는 가장 이상적인 힙 이용도**가 되겠다. 

할당기가 메모리 공간을 마구 낭비하면서 빠른 연산을 수행하는 것은 쉽다. 할당기의 처리량은 높아지겠지만 메모리 최고 이용도는 낮아질 것이다. 이는 좋은 할당기라 보기 힘들다. 

중요한 것은 **할당기의 처리량과 메모리 최고 이용도 사이의 적절한 균형을 찾는 일**이다.

<br>

**메모리 단편화**
===

> **가용 메모리가 할당 요청을 만족시키지 못했을 때** 나타나는, 나쁜 힙 이용도의 원인이 되는 현상.
> 
- **내부 단편화 Internal Fragmentation**
    
    💡 내부 단편화는 **할당된 블록이 데이터 자체보다 클 때** 일어난다.
    
    
	<p align="center">
		<img src = ".\image\Untitled(2).png" width="400" height="100"/>
	</p>
    
    원인으로는 정렬 제한사항을 만족하기 위해 패딩이 일어났을 경우 등이 있다.
    
    어차피 할당된 블록의 크기와 데이터 차이의 합으로 정량화할 수 있으므로, 어렵지 않다.
    
- **외부 단편화 External Fragmentation**
    
    💡 힙 전체로 봤을 때는 할당 요청을 만족할 만한 메모리 공간이 있지만, **각각의 가용 블록들이 이 요청을 처리할 만큼 크지 않을 때.**
    
    
    <p align="center">
		<img src = ".\image\Untitled(3).png" width="500" height="250"/>
	</p>
	<p align="center">
	<em>6 워드의 블록 할당을 요청 받았을 때, 힙 전체는 7 워드의 메모리 공간이 존재하지만 단일 가용 블록은 5 워드, 2 워드로 요청을 처리할 수 없다.</em>
	</p>
    


   	 
    
    외부 단편화는 미래에 얼마만큼의 메모리를 할당받을 것인지에 따라 달라진다. 즉, **예측이 불가능하다.** 따라서 할당기들은 많은 수의 작은 가용 블록들보다는 **적은 수의 큰 가용 블록들을 유지하는 것을 선호한다.**
    
<br>

**할당기 구현 이슈**
---

이제 어떻게 하면 할당기를 구현할 수 있을지 알아보자.

- **가용 블록 구성 :** 사용할 수 있는 가용 블록들을 어떻게 추적할 수 있는가?
- **배치 :**  또한 할당하기 위한 가용 블록을 어떻게 선택할 것인가?
- **분할 :** 가용 블록보다 더 작은 블록을 할당했을 때, 나머지 가용 블록들을 어떻게 처리할 것인가?
- **반환 :** 전달받은 주소로 얼마나 많은 메모리를 반환해 주어야 하는가?
- **연결 :** 방금 반환된 블록을 어떻게 할 것인가?

위와 같은 기능들을 우선은 묵시적 가용 리스트 블록 구조 안에서 살펴본다.

### **가용 블록 구성 :** 가용 블록들을 추적하는 방법(힙의 구조)

- **Implicit List**
    
    > 가용 블록들을 추적하기 위해 모든 블록들의 헤더(해당 블록이 사용할 수 있는지를 저장)를 방문한다.
    > 
- **Explicit List**
    
    > 가용  블록 내 다음 가용 블록을 가리키는 포인터가 있어, 한 가용 블록에서 다음 가용 블록의 위치를 명시적으로 알 수 있다.
    > 
- **Segregated Free List**
    
    > 할당이 해제되어있는 블록들을 사이즈 별로 각각의 연결 리스트를 만들어 관리한다.
    > 
- **Blocks sorted by size**
    
    > 균형 트리(RB트리 등)을 이용해 가용 리스트들을 크기 순대로 정렬한다.
    > 

<br>

**묵시적 가용 리스트 Implicit List**
===

> 블록 경계를 구분하고 할당된 블록과 가용 블록을 구분할 수 있다.
> 

<p align="center">
	<img src = ".\image\Untitled(4).png" width="500" height="250"/>
</p>
<p align="center">
<em>묵시적 가용 블록의 구조</em>
</p>


- **헤더 필드(1 word)**
    
    블록 맨 앞의 word 하나에 블록의 전체 크기와 할당 여부를 저장한다. 이를 **헤더**라고 한다.
    
    헤더 필드를 위해서는 모든 할당 블록이 **1word만큼의 메모리 공간을 추가로 요구**한다. 만약 할당기가 4word의 메모리를 할당한다면, 5word의 가용 블록을 할당해야 한다. 그리고 헤더 바로 뒤의 **payload의 맨 첫 주소를 반환**한다.
    
	<p align="center">
		<img src = ".\image\Untitled(5).png" width="500" height="150"/>
	</p>
	<p align="center">
		<em>헤더 필드를 위해서는 모든 할당 블록이 1word만큼의 메모리 공간을 추가로 요구한다.</em>
	</p>
    
    
		
	💡 만약 블록들이 **정렬되어 있다면,** **사이즈를 나타내는 데이터의 0번째 자리에 블록의 할당 여부를 flag로 저장**한다. 따라서 사이즈와 할당 여부를 모두 1word에 나타낼 수 있다.


	예를 들어 2word 정렬이라면 사이즈는 늘 8byte의 배수일 것이므로 1000(8), 11000(24) 등이다. 맨 뒷자리는 늘 0이므로, 이 자리를 할당 여부를 저장하는 데 사용할 수 있다.
		
- **묵시적 가용 리스트로 이루어진 힙**
    
	<p align="center">
		<img src = ".\image\Untitled(6).png" width="600" height="250"/>
	</p>


<br>

**배치 : Implicit List에서 Free block 탐색**
---

- **First fit**
    
    > **리스트의 처음부터 탐색을 시작**해, 조건에 맞는 **맨 첫 블록에 할당**한다.
    > 
    
    ```c
    p = start;
    while ((p < end) &&  // 1. 리스트 끝까지 가기 전까지
    			 ((*p & 1) || (*p <= len)))  // 2. 이미 할당되어 있거나 크기가 작으면
    	p = p + (*p & -2);  // 3. 다음 블록으로 패스한다(mask out low bit).
    ```
    
- **Next fit**
    
    > 이전 검색이 종료된 지점부터 검색을 다시 시작한다.
    > 
- **Best fit**
    
    > 
    > 

<br>

**분할 : 가용 블록을 할당하기**
---

만약 가용 블록이 할당기가 요구하는 사이즈보다 크다면, 

> 할당기가 가용 블록을 **할당한 블록과 새 가용 블록 두 부분으로** 나눈다.
> 

<p align="center">
	<img src = ".\image\Untitled(7).png" width="450" height="150"/>
</p>


```c
void addblock(ptr p, int len) {
	int newsize = ((len + 1) >> 1) << 1;  // 짝수로 반올림해준다.
	int oldsize = *p & -2;  // 원래 가용 블록의 사이즈
	*p = newsize | 1;       // 새 가용 블록의 헤더를 만든다. 할당되었으니까 1!
  if (newsize < oldsize)
		*(p + newsize) = oldsize - newsize;  // 새 가용 블록의 헤더를 설정해준다.
    // 할당 안 되었으므로 그냥 사이즈끼리 빼면 맨 끝 숫자는 0.
}
```

<br>

**반환**
---

> 그냥 블록 헤더의 allocated flag를 0으로 만들어주면 된다.
> 

```c
void free_block(ptr p) 
	*p = *p & -2;
```

하지만 이런 경우 **false fragmentation**이 일어날 수도 있다. 이를 막기 위해 **연결 Coalescing**을 이용한다.

💡 False fragmentation :  **공간이 충분함에도 불구하고** 할당기가 그 공간을 찾을 수 없는 현상.


<p align="center">
	<img src = ".\image\Untitled(8).png" width="500" height="150"/>
</p>
<p align="center">
	<em>반환되었음에도 인접한 반환 블록이 서로 구분되어져 있다. 이럼 5word를 할당을 못한다.</em>
</p>

<br>

**연결 Coalescing**
---

> **인접해 있는 가용 블록들을 서로 연결**한다.
> 

<p align="center">
	<img src = ".\image\Untitled(9).png" width="500" height="150"/>
</p>


<br>

**반환한 블록과 그 다음 블록을 연결하기**
```c
void free_block(ptr p) {
	*p = *p & -2;  // 1.기존 블록을 반환한다. (*p = 0000 0101 -> 0000 0100, p = 0000 1000)
	next = p + *p;  // 2. 다음 블록은 기존 블록에 그 크기를 더한 값 (next = 0000 1100)
	if ((*next & 1) == 0)  //  3. 만약 다음 블록이 가용 블록이라면 (next* = 0000 0010)
		*p = *p + * next;    // 4. 연결시켜준다(블록의 크기를 늘린다!) (*p = 0000 0110, 크기가 6)
```

그런데 **이전 블록**과는 어떻게 연결하지?

<br>

**Boundary Tags**
---

> **헤더의 정보**(블록의 크기 & 할당 여부)**를 복사해서 블록의 맨 끝에** 둔다(footer).
> 

이를 이용해 기준 블록의 헤더에서 한 칸만 뒤로 가면 이전 블록의 할당 여부를 알 수 있다. **연결 연산을 상수 시간에 처리할 수 있다!**

<p align="center">
	<img src = ".\image\Untitled(10).png" width="500" height="150"/>
</p>
<p align="center">
	<em>하지만 1word만큼의 메모리가 더 필요하긴 하다.</em>
</p>



<p align="center">
	<img src = ".\image\Untitled(11).png" width="500" height="100"/>
</p>
<p align="center">
	<em>Boundary Tag를 이용해 이전 블록의 정보를 쉽게 알 수 있다.</em>
</p>


<br>

**Constant Time Coalescing : Boundary Tag를 이용한 가용 블록의 연결 → O(1)**
---

> 반환할 블록 A의 이전과 이후 블록 B, C를 검사해, 만약 가용 블록이라면 A의 할당 여부를 0으로 바꾸고 B 혹은 C에 연결한다(크기를 더해준다).
> 

이전 이후 블록의 할당 여부에 따라 4가지 케이스로 구분할 수 있다.
<p align="center">
	<img src = ".\image\Untitled(12).png" width="500" height="100"/>
</p>

<p align="center">
	<img src = ".\image\Untitled(13).png" width="450" height="300"/>
</p>

<br>

**Boundary Tag의 단점과 개선 방안**
---

💡 결국 새로운 메모리를 header와 footer에 할당해야 하므로, 메모리 오버헤드가 발생한다.


- **오버헤드** : 어떤 처리를 하기 위해 필요한 **간접적인** 시간 or 메모리

그렇다면 이를 해결해주기 위해서 해 줄 수 있는 조치는?

> **오로지 가용 블록일 때만 footer를 사용한다.** 만약 할당 블록일 경우에는 Coalescing을 할 필요가 없으므로 footer가 필요가 없거든.
> 

근데 내 이전 블록이 가용 블록이라 footer가 없으면, 어떻게 이 블록이 가용 블록인지 아닌지를 알 수 있을까?

> 헤더에서 해당 블록의 할당 여부를 저장하는 자릿수 그 다음 자리에 이전 블록의 할당 여부를 저장한다.
>

<br>
<br>

**명시적 가용 리스트 Explicit Free List**
===

> **가용 블록들끼리 포인터로 연결**한다. 다음 가용 블록의 위치를 명시적으로 알 수 있다.
> 

<p align="center">
	<img src = ".\image\Untitled5.png" width="500" height="100"/>
</p>

<br>

**개요**
---

> 가용 블록은 헤더와 풋터 말고는 데이터를 필요로 하지 않으므로, **가용 블록 안의 payload 자리에 다음 가용 블록과 이전 가용 블록의 주소를 저장**한다.
> 

<p align="center">
	<img src = ".\image\Untitled6.png" width="400" height="250"/>
</p>
<p align="center">
	<em>할당 블록은 Implicit List 방식과 동일하다. 대신 가용 블록 안에 포인터가 들어간다.</em>
</p>



💡 하나의 더블 연결 리스트를 만들어 가용 블록들을 연결한다.
- **다음next** 가용 블록은 어디서나 있을 수 있다(주소 상으로 인접한 **직전predecessor, 직후successor 블록이 아닐수도!** 심지어는 **주소 순이 아닐수도** 있다**.**)
- **연결을 위해 boundary tag는 필요하다.**

<p align="center">
	<img src = ".\image\Untitled7.png" width="400" height="250"/>
</p>
<p align="center">
	<em>Free list의 논리적, 물리적 나열</em>
</p>


**할당**
---

> 해당 가용 리스트를 **분할**한 다음 **새 가용 블록**을 기존 가용 블록과 링크되었던 **이전prev, 다음next 블록과 링크**한다.
> 

<p align="center">
	<img src = ".\image\Untitled8.png" width="400" height="250"/>
</p>

**가용**
--

> 새로 반환된 가용 블록을 **free list 어디에 링크해야 할까?**
> 


**LIFO(Last In First Out) 방안**

- **Free list의 처음에** 새 반환 블록을 넣는다.
- 간단하고 O(1)의 시간 복잡도를 갖지만, Adress ordered 방법보다 fragmentation 발생 확률이 높다.

**Adress Ordered 방안**

- **Free List를 주소 순으로** 링크한다.
    
    즉, **(이전 블록의 주소) < (현재 블록의 주소) < (다음 블록의 주소)**
    
- Fragmentation 확률은 낮아지지만 탐색 시간이 필요하다. RB Tree 같은 탐색 시간이 빠른(**O(logN)**) 자료구조를 사용한다 해도 Segregate 방법이나 Best fit보다는 느리다고 한다.

<br>

**LIFO 방안을 이용한 반환**
--

**Case 1 : 반환 블록 양 쪽이 할당 블록**

> Free List의 맨 앞을 반환 블록으로 만들어준다.
> 
- Free List의 Root의 Next를 반환 블록과 연결
- 기존의 Root와 연결되었던 블록을 반환 블록의 Next와 연결

<p align="center">
	<img src = ".\image\Untitled9.png" width="400" height="250"/>
</p>

<br>

**Case 2 : 반환 블록 직전이 할당 블록, 직후가 가용 블록** 

- 직후 블록을 Free List에서 떼어낸 다음, 반환 블록과 연결
- 이 새 블록을 Free List의 Root와 연결

<p align="center">
	<img src = ".\image\Untitled10.png" width="400" height="250"/>
</p>

**Case 3 : 반환 블록 직전이 가용 블록, 직후가 할당 블록** 

- 직전 블록을 Free List에서 떼어낸 다음, 반환 블록과 연결
- 이 새 블록을 Free List의 Root와 연결

<p align="center">
	<img src = ".\image\Untitled11.png" width="400" height="250"/>
</p>

**Case 4 : 반환 블록 직전, 직후가 가용 블록**

- 직전 블록과 직후 블록을 각각 Free List에서 떼어낸 다음, 반환 블록과 연결
- 이 새 블록을 Free List의 Root와 연결
<p align="center">
	<img src = ".\image\Untitled12.png" width="400" height="250"/>
</p>

<br>

**Implicit Free List와의 비교**
---

- First Fit 할당 시, 전체 리스트 개수가 아닌 **가용 리스트의 개수에 비례**하도록 **할당 시간을 줄일 수** 있다.
- 링크를 위해 **2 word의 추가적인 메모리가 더 필요**하다.
- Segregated Free List 사용 시 블록들의 크기에 따라 Free List를 만들 수 있다. 이 때 Explicit List에서 활용되었던 것과 비슷하게 Linked List를 사용한다.

<br>
<br>

**malloc 구현**
===

**memlib.c**
---

> 기존의 `malloc` 패키지 대신 힙 메모리 초기화와 sbkr함수를 정의한 패키지
> 

### **변수**

- `mem_start_brk` :  힙의 첫 바이트를 가리키는 변수
- `mem_brk` : 힙의 마지막 바이트를 가리키는 변수. 여기다가 sbrk 함수로 새로운 사이즈의 힙을 할당받는다.
    - 할당된 가상 메모리 : `mem_start_brk` 와 `mem_brk` 사이
    - 미할당된 가상 메모리 : `mem_brk` 이후
- `mem_max_addr` : 힙의 최대 크기(20MB)의 주소를 가리키는 변수

### **함수**

- **void mem_init(void)**
    
    > 최대 힙 메모리 공간을 할당받고 초기화해준다.
    > 
    - 먼저 20MB만큼의 `MAX_HEAP`을 malloc으로 동적할당 해 온다. 만약 메모리를 불러오는 데 실패했다면 에러 메세지를 띄우고 프로그램을 종료한다.
    - 그 시작점을 `mem_start_brk`라 한다.
    - 아직 힙이 비어 있으므로 `mem_brk`도 `mem_start_brk`와 같다.
    
    ```c
    void mem_init(void)
    {
        /* allocate the storage we will use to model the available VM */
        // config.h에 정의되어 있음, #define MAX_HEAP (20*(1<<20)) : 20971520bytes == 20 MB
        if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
    	fprintf(stderr, "mem_init_vm: malloc error\n");
    	exit(1);
        }
    
        mem_max_addr = mem_start_brk + MAX_HEAP;  /* max legal heap address */
        mem_brk = mem_start_brk;                  /* heap is empty initially */
    }
    ```
    
- __void* mem_sbrk(int incr)__
    
    > **byte 단위로 필요 메모리 크기를 입력**받아 그 크기만큼 힙을 늘려주고, **새 메모리의 시작 지점을 리턴**한다.
    > 
    - 힙을 줄이려 하거나 최대 힙 크기를 넘어버리면 -1을 리턴한다.
    - `old_brk`의 주소를 리턴하는 이유 : 새 메모리의 시작부터 사용해야 하므로.
    
    ```c
    /* 
     * mem_sbrk - simple model of the sbrk function. Extends the heap 
     *    by incr bytes and returns the start address of the new area. In
     *    this model, the heap cannot be shrunk.
     */
    void *mem_sbrk(int incr) // 바이트 형태로 입력받는다.
    {
        char *old_brk = mem_brk;  // 힙 늘이기 전의 끝 포인터를 저장한다.
    
        // 힙이 줄어들거나 최대 힙 사이즈를 벗어난다면
        // 메모리 부족으로 에러처리 하고 -1을 리턴한다.
        if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
    	errno = ENOMEM;  // 메모리 부족 에러 처리
    	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
    	return (void *)-1;  // 리턴값이 void*여야 해서 형변환 해준 듯
        }
        
        mem_brk += incr; // mem_brk에 incr만큼 더함으로써 힙을 늘려주었다.
        return (void *)old_brk;  // 이전 brk를 리턴한다. 왜? 이 새로운 메모리를 처음부터 사용해야 하니까.
    ```
    

**mm.c : Implicit Allocator. First-Fit, Next-fit**
===

**함수**
---

- **가용 리스트 조작을 위한 기본 상수 및 매크로 정의**
    
    ```c
    /* single word (4) or double word (8) alignment */
    #define ALIGNMENT 8
    
    /* rounds up to the nearest multiple of ALIGNMENT */
    // size보다 큰 가장 가까운 ALIGNMENT의 배수로 만들어준다 -> 정렬!
    // size = 7 : (00000111 + 00000111) & 11111000 = 00001110 & 11111000 = 00001000 = 8
    // size = 13 : (00001101 + 00000111) & 11111000 = 00010000 = 16
    // 1 ~ 7 bytes : 8 bytes
    // 8 ~ 16 bytes : 16 bytes
    // 17 ~ 24 bytes : 24 bytes
    #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
    
    /* 메모리 할당 시 기본적으로 header와 footer를 위해 필요한 더블워드만큼의 메모리 크기 */
    // size_t : 해당 시스템에서 어떤 객체나 값이 포함할 수 있는 최대 크기의 데이터
    #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
    
    /*
        기본 상수와 매크로
    */
    
    /* 기본 단위인 word, double word, 새로 할당받는 힙의 크기 CHUNKSIZE를 정의한다 */
    #define WSIZE       4       /* Word and header/footer size (bytes) */
    #define DSIZE       8       /* Double word size (bytes) */
    #define CHUNKSIZE   (1<<12) /* Extend heap by this amount : 4096bytes -> 4kib */
    
    #define MAX(x, y) ((x) > (y) ? (x) : (y))  // 최댓값 구하는 함수 매크로
    
    /* header 및 footer 값(size + allocated) 리턴 */
    // 더블워드 정렬로 인해 size의 오른쪽 3~4자리는 비어 있다. 
    //이 곳에 0(freed), 1(allocated) flag를 삽입한다.
    #define PACK(size, alloc)   ((size) | (alloc))   
    
    /* 주소 p에서의 word를 읽어오거나 쓰는 함수 */
    // 포인터 p가 가리키는 곳의 값을 리턴하거나 val을 저장
    #define GET(p)          (*(unsigned int*)(p))
    #define PUT(p, val)     (*(unsigned int*)(p) = (val))
    
    /* header or footer에서 블록의 size, allocated field를 읽어온다 */
    // & ~0x7 => 0x7:0000 0111 ~0x7:1111 1000이므로 ex. 1011 0111 & 1111 1000 = 1011 0000 : size 176bytes
    // & 0x1 => ex. 1011 0111 | 0000 0001 = 1 : Allocated!
    #define GET_SIZE(p)     (GET(p) & ~0x7) 
    #define GET_ALLOC(p)    (GET(p) & 0x1)    
    
    /* 블록 포인터 bp를 인자로 받아 블록의 header와 footer의 주소를 반환한다 */
    // 포인터가 char* 형이므로, 숫자를 더하거나 빼면 그 만큼의 바이트를 뺀 것과 같다.
    // WSIZE 4를 뺀다는 것은 주소가 4byte(1 word) 뒤로 간다는 뜻. bp의 1word 뒤는 헤더.
    #define HDRP(bp)    ((char*)(bp) - WSIZE) 
    #define FTRP(bp)    ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
    
    /* 블록 포인터 bp를 인자로 받아 이후, 이전 블록의 주소를 리턴한다 */
    // NEXT : 지금 블록의 bp에 블록의 크기(char*이므로 word단위)만큼을 더한다.
    // PREV : 지금 블록의 bp에 이전 블록의 footer에서 참조한 이전 블록의 크기를 뺀다.
    #define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE))) // (char*)(bp) + GET_SIZE(지금 블록의 헤더값)
    #define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) // (char*)(bp) - GET_SIZE(이전 블록의 풋터값)
    
    /* define searching method for find suitable free blocks to allocate*/
    #define NEXT_FIT  // define하면 next_fit, 안 하면 first_fit으로 탐색
    
    /* global variable & functions */
    static char* heap_listp; // 항상 prologue block을 가리키는 정적 전역 변수 설정
    
    #ifdef NEXT_FIT
        static void* last_freep;  // next_fit 사용 시 마지막으로 탐색한 가용 블록
    #endif
    
    static void* extend_heap(size_t words);
    static void* coalesce(void* bp);
    static void* find_fit(size_t asize);
    static void place(void* bp, size_t newsize);
    
    int mm_init(void);
    void *mm_malloc(size_t size);
    void mm_free(void *bp);
    void *mm_realloc(void *ptr, size_t size);
    ```
    
<br>

- **주의 : static 변수와 함수는 맨 처음 먼저 선언해주어야 한다.**
    
    ```c
    /* global variable & functions */
    static char* heap_listp; // 항상 prologue block을 가리키는 정적 전역 변수 설정
    
    #ifdef NEXT_FIT
        static void* last_freep;  // next_fit 사용 시 마지막으로 탐색한 가용 블록
    #endif
    
    static void* extend_heap(size_t words);
    static void* coalesce(void* bp);
    static void* find_fit(size_t asize);
    static void place(void* bp, size_t newsize);
    
    int mm_init(void);
    void *mm_malloc(size_t size);
    void mm_free(void *bp);
    void *mm_realloc(void *ptr, size_t size);
    ```
<br>

- **extern int mm_init(void)**
    
    > 최초의 가용 블록(4words)을 가지고 힙을 생성하고 할당기를 초기화한다. 성공하면 0, 실패하면 -1을 리턴한다.
    > 
    
    <p align="center">
	<img src = ".\image\Untitled(14).png" width="350" height="150"/>
    </p>
    <p align="center">
    </p>
    
    ```c
    int mm_init(void)
    {
        /* 메모리에서 4word 가져오고 이걸로 빈 가용 리스트 초기화 */
        if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
            return -1;
    
        PUT(heap_listp, 0);  // Alignment padding. 더블 워드 경계로 정렬된 미사용 패딩.
        PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  // prologue header
        PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  // prologue footer
        PUT(heap_listp + (3*WSIZE), PACK(0, 1));      // epliogue header
        heap_listp += (2*WSIZE);  //정적 전역 변수는 늘 prologue block을 가리킨다.
    
        #ifdef NEXT_FIT
            last_freep = heap_listp;
        #endif
    
        /* 그 후 CHUNKSIZE만큼 힙을 확장해 초기 가용 블록을 생성한다. */
        if (extend_heap(CHUNKSIZE / WSIZE) == NULL) //실패하면 -1 리턴
            return -1;
    
        return 0;
    }
    ```
    
    - **미사용 패딩을 사용하는 이유**
        
        > `prologue block` 다음에 오는 **실제 필요한 데이터를 더블 워드 정렬해주기 위해.**
        > 
        
        <p align="center">
        <img src = ".\image\Untitled(15).png" width="500" height="300"/>
        </p>
        <p align="center">
            <em>미사용 패딩을 사용하여야 프롤로그 블록 뒤의 실제 데이터 블록들이 8의 배수로 정렬될 수 있다.</em>
        </p>

<br>
        

- __static void* extend_heap(size_t words)__
    
    > 워드 단위 메모리를 인자로 받아 힙을 늘려준다.
    > 
    - `mem_sbrk()` 함수를 이용해 새로 힙의 크기를 늘린다. 이 때, 더블 워드 정렬에 맞게 할당 받는다.
    - **새 가용 블록의 헤더와 풋터**를 정해주고 `epilogue block`을 새로 정해준다.
    - 이전 블록의 가용 여부를 조사해 **연결**한다.
    
    ```c
    /*
        extend_heap : 워드 단위 메모리를 인자로 받아 힙을 늘려준다.
    */
    static void* extend_heap(size_t words){ // 워드 단위로 받는다.
        char* bp;
        size_t size;
        
        /* 더블 워드 정렬에 따라 메모리를 mem_sbrk 함수를 이용해 할당받는다. */
        // Double Word Alignment : 늘 짝수 개수의 워드를 할당해주어야 한다.
        size = (words % 2) ? (words + 1) * WSIZE : (words) * WSIZE; // size를 짝수 word && byte 형태로 만든다.
        if ((long)(bp = mem_sbrk(size)) == -1) // 새 메모리의 첫 부분을 bp로 둔다. 주소값은 int로는 못 받아서 long으로 casting한 듯.
            return NULL;
        
        /* 새 가용 블록의 header와 footer를 정해주고 epilogue block을 가용 블록 맨 끝으로 옮긴다. */
        PUT(HDRP(bp), PACK(size, 0));  // 헤더. 할당 안 해줬으므로 0으로.
        PUT(FTRP(bp), PACK(size, 0));  // 풋터.
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 새 에필로그 헤더
    
        /* 만약 이전 블록이 가용 블록이라면 연결시킨다. */
        return coalesce(bp);
    }
    ```
<br>

- __extern void mm_free(void* bp)__

    > 해당 주소의 블록을 반환한다. 그냥 헤더와 풋터의 정보를 바꾸고 앞뒤 가용 블록과 연결시켜주면 된다.
    > 

    ```c
    /*
        * mm_free - Freeing a block does nothing.
        */
    void mm_free(void *bp)
    {
        // 해당 블록의 size를 알아내 header와 footer의 정보를 수정한다.
        size_t size = GET_SIZE(HDRP(bp));

        // header와 footer를 설정
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        // 만약 앞뒤의 블록이 가용 상태라면 연결한다.
        coalesce(bp);
    }
    ```
<br>

- __static void* coalesce(void* bp)__
    
    > 해당 가용 블록을 앞뒤 가용 블록과 연결하고 연결된 가용 블록의 주소를 리턴한다.
    > 
    
    ```c
    static void* coalesce(void* bp){
        // 직전 블록의 footer, 직후 블록의 header를 보고 가용 여부를 확인.
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));  // 직전 블록 가용 여부
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // 직후 블록 가용 여부
        size_t size = GET_SIZE(HDRP(bp));
    
        // case 1 : 직전, 직후 블록이 모두 할당
        // 해당 블록만.
        if (prev_alloc && next_alloc)
            return bp;
    
        // case 2 : 직전 블록 할당, 직후 블록 가용
        else if(prev_alloc && !next_alloc){
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0)); // 이미 여기서 footer가 직후 블록의 footer로 변경된다.
            PUT(FTRP(bp), PACK(size, 0)); // 직후 블록 footer 변경 
            // 블록 포인터는 변경할 필요 없다.
        }
    
        // case 3 : 직전 블록 가용, 직후 블록 할당
        else if(!prev_alloc && next_alloc){
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(FTRP(bp), PACK(size, 0));  // 해당 블록 footer
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 직후 블록 header
            bp = PREV_BLKP(bp); // 블록 포인터를 직전 블록으로 옮긴다.
        }
    
        // case 4 : 직전, 직후 블록 모두 가용
        else{
            size += GET_SIZE(HDRP(PREV_BLKP(bp)))
                    + GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));  // 직전 블록 header
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));  // 직후 블록 footer
            bp = PREV_BLKP(bp);  // 블록 포인터를 직전 블록으로 옮긴다.
        }
    
    		// next-fit 사용 시, 추적 포인터를 연결이 끝난 블록의 블록 포인터로 변경한다.
        #ifdef NEXT_FIT
            last_freep = bp;
        #endif
    
        // 최종 가용 블록의 주소를 리턴한다.
        return bp;
    }
    ```
<br>

- __extern void* mm_malloc (size_t size)__
    
    > 동적 할당할 메모리 크기를 받아 알맞은 크기의 블록을 할당한다. 만약 후보가 없으면 힙을 CHUNKSIZE만큼 늘려 할당 받는다. 그리고 할당 받은 블록의 포인터를 반환한다.
    > 
    
    ```c
    void *mm_malloc(size_t size)
    {
        size_t asize;       // Adjusted block size
        size_t extendsize;  // Amount for extend heap if there is no fit
        char* bp;
    
        // 가짜 요청 spurious request 무시
        if (size == 0)
            return NULL;
    
        // 요청 사이즈에 header와 footer를 위한 dword 공간(SIZE_T_SIZE)을 추가한 후 align해준다.
        asize = ALIGN(size + SIZE_T_SIZE);  
    
        // 할당할 가용 리스트를 찾아 필요하다면 분할해서 할당한다!
        if ((bp = find_fit(asize)) != NULL){  // 알맞은 가용 블록의 주소를 리턴한다.
            place(bp, asize);  // 필요하다면 분할하여 할당한다.
            return bp;
        }
    
        // 만약 맞는 크기의 가용 블록이 없다면 새로 힙을 늘려서 
        extendsize = MAX(asize, CHUNKSIZE);  // 둘 중 더 큰 값으로 사이즈를 정한다.
        // extend_heap()은 word 단위로 인자를 받으므로 WSIZE로 나눠준다.
        if ((bp = extend_heap(extendsize / WSIZE)) == NULL) 
            return NULL;
        // 새 힙에 메모리를 할당한다.
        place(bp, asize);
        return bp;
    }
    ```
<br>

- __static void* find_fit(size_t asize)__
    
    > 힙을 탐색하여 요구하는 메모리 공간보다 큰 가용 블록의 주소를 반환한다.
    > 
    - first-fit
    - next-fit
        
        next-fit의 기준이 되는 탐색 시작점 `last_freep`를 잘 잡는 것이 포인트이다. 
        
        - 맨 처음 힙을 초기화할 때 힙의 시작 지점, 즉 프롤로그를 가리키는 포인터 `heap_listp`를 가리키게 한다.
        - 연결 시, 만약 직전의 가용 블록과 **연결됐을 경우** `last_freep` 역시 **직전 블록의 블록 포인터를 가리켜야** 한다.
        - next-fit 탐색 시, `last_freep`의 시작부터 힙 끝까지 알맞은 가용 블록을 찾지 못했다면, **맨 앞에서부터도 탐색하는 것이 효율적**이다.
    
    ```c
    static void* find_fit(size_t asize){
    /* Next-fit */
        #ifdef NEXT_FIT
            void* bp;
            void* old_last_freep = last_freep;
    
            // 이전 탐색이 종료된 시점에서부터 다시 시작한다.
            for (bp = last_freep; GET_SIZE(HDRP(bp)); bp = NEXT_BLKP(bp)){
                if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
                    return bp;
            }
    
            /*
            만약 끝까지 찾았는데도 안 나왔으면 처음부터 찾아본다.
            이 구문이 없으면 바로 extend_heap을 하는데, 
            이럼 앞에 있는 가용 블록들을 사용하지 못할 수 있어 메모리 낭비이다.
            */
            for (bp = heap_listp; bp < old_last_freep; bp = NEXT_BLKP(bp)){
                if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
                    return bp;
            }
    
            last_freep = bp;  // 다시 탐색을 마친 시점으로 last_freep를 돌린다.
    
            return NULL;
    
    		/* first-fit */
        #else
            void* bp;
    
            // 프롤로그 블록에서 에필로그 블록 전까지 블록 포인터 bp를 탐색한다.
            // 블록이 가용 상태이고 사이즈가 요구 사이즈보다 크다면 해당 블록 포인터를 리턴
            for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
                if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
                    return bp;
                }
            }
    
            // 못 찾으면 NULL을 리턴한다.
            return NULL;
        #endif
    
    }
    ```
    
<br>

- __static void place(void* bp, size_t asize)__
    
    > 요구 메모리를 할당할 수 있는 가용 블록을 할당한다. 이 때 분할이 가능하면 분할한다.
    > 
    
    ```c
    static void place(void* bp, size_t asize){
        // 현재 할당할 수 있는 후보 가용 블록의 주소
        size_t csize = GET_SIZE(HDRP(bp));
    
        // 분할이 가능한 경우
        // -> 남은 메모리가 최소한의 가용 블록을 만들 수 있는 4word(16byte)가 되느냐.
        // header & footer : 1word씩, payload : 1word, 정렬 위한 padding : 1word = 4words
        if ((csize - asize) >= (2*DSIZE)){
            // 앞의 블록은 할당 블록으로
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            // 뒤의 블록은 가용 블록으로 분할한다.
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize-asize, 0));
            PUT(FTRP(bp), PACK(csize-asize, 0));
        }
        // 분할할 수 없다면 남은 부분은 padding한다.
        else{
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
        }
    }
    ```
<br>


- __void *mm_realloc(void *ptr, size_t size)__
    
    > 입력받은 사이즈의 크기만큼 기존의 힙을 늘려준다. 만약 입력 사이즈가 기존 힙의 크기보다 작으면 그 나머지 메모리는 잘린다.
    > 
    
    ```c
    void *mm_realloc(void *ptr, size_t size)
    {
        void *oldptr = ptr;  // 크기를 조절하고 싶은 힙의 시작 포인터
        void *newptr;        // 크기 조절 뒤의 새 힙의 시작 포인터
        size_t copySize;     // 복사할 힙의 크기
        
        newptr = mm_malloc(size);
        if (newptr == NULL)
          return NULL;
    
        // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
        copySize = GET_SIZE(HDRP(oldptr));
    
        // 원래 메모리 크기보다 적은 크기를 realloc하면 
        // 크기에 맞는 메모리만 할당되고 나머지는 안 된다. 
        if (size < copySize)
          copySize = size;
    
        memcpy(newptr, oldptr, copySize);  // newptr에 oldptr를 시작으로 copySize만큼의 메모리 값을 복사한다.
        mm_free(oldptr);  // 기존의 힙을 반환한다.
        return newptr;
    }
    ```
<br>
<br>

**mm.c : Explicit Allocator, First-fit**
===

- **int mm_init(void)**

    ```c
    int mm_init(void)
    {
        /* 메모리에서 6words를 가져오고 이걸로 빈 가용 리스트 초기화 */
        /* padding, prol_header, prol_footer, PREC, SUCC, epil_header */
        if ((heap_listp = mem_sbrk(6*WSIZE)) == (void*)-1)
            return -1;

        PUT(heap_listp, 0);  // Alignment padding. 더블 워드 경계로 정렬된 미사용 패딩.
        PUT(heap_listp + (1*WSIZE), PACK(MINIMUM, 1));  // prologue header
        PUT(heap_listp + (2*WSIZE), NULL);  // prologue block안의 PREC 포인터 NULL로 초기화
        PUT(heap_listp + (3*WSIZE), NULL);  // prologue block안의 SUCC 포인터 NULL로 초기화
        PUT(heap_listp + (4*WSIZE), PACK(MINIMUM, 1));  // prologue footer
        PUT(heap_listp + (5*WSIZE), PACK(0, 1));      // epliogue header

        free_listp = heap_listp + 2*WSIZE; // free_listp를 탐색의 시작점으로 둔다. 

        #ifdef NEXT_FIT
            last_freep = heap_listp;
        #endif

        /* 그 후 CHUNKSIZE만큼 힙을 확장해 초기 가용 블록을 생성한다. */
        if (extend_heap(CHUNKSIZE / WSIZE) == NULL) //실패하면 -1 리턴
            return -1;

        return 0;
    }
    ```
    <p align="center">
    <img src = ".\image\capture.png" width="500" height="200"/>
    </p>
    <p align="center">
        <em>Explicit Allocator의 초기 힙은 6 words의 메모리를 가진다.</em>
    </p>

<br>

- __static void* coalesce(void* bp)__
    
    > 만약 반환 블록의 직전, 직후 중 free 블록이 있다면, 그 블록을 free list에서 없애준 후 반환 블록과 연결한다. 그 후 연결된 새 블록을 free list의 첫 부분에 넣는다.
    > 

    ```c
    static void* coalesce(void* bp){
        // 직전 블록의 footer, 직후 블록의 header를 보고 가용 여부를 확인.
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));  // 직전 블록 가용 여부
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // 직후 블록 가용 여부
        size_t size = GET_SIZE(HDRP(bp));

        // case 1 : 직전, 직후 블록이 모두 할당 -> 해당 블록만 free list에 넣어주면 된다.

        // case 2 : 직전 블록 할당, 직후 블록 가용
        if(prev_alloc && !next_alloc){
            removeBlock(NEXT_BLKP(bp));    // free 상태였던 직후 블록을 free list에서 제거한다.
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
        }

        // case 3 : 직전 블록 가용, 직후 블록 할당
        else if(!prev_alloc && next_alloc){
            removeBlock(PREV_BLKP(bp));    // 직전 블록을 free list에서 제거한다.
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            bp = PREV_BLKP(bp); 
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));  
        }

        // case 4 : 직전, 직후 블록 모두 가용
        else if (!prev_alloc && !next_alloc) {
            removeBlock(PREV_BLKP(bp));
            removeBlock(NEXT_BLKP(bp));
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
            bp = PREV_BLKP(bp);
            PUT(HDRP(bp), PACK(size, 0));  
            PUT(FTRP(bp), PACK(size, 0));  
        }

        // 연결된 새 가용 블록을 free list에 추가한다.
        putFreeBlock(bp);

        #ifdef NEXT_FIT
            last_freep = bp;
        #endif

        return bp;
    }
    ```

<br>

- __void putFreeBlock(void* bp)__
    
    > 새 free 블록을 free list의 처음에 추가한다.
    > 
    
    포인터 `free_listp`는 free list의 첫 주소를 가리키므로, `free_listp`가 가리키는 free list 안의 블록과 PRED, SUCC 링크를 진행한다.
    
    <p align="center">
    <img src = ".\image\capture3.png" width="650" height="450"/>
    </p>
    <p align="center">
        <em>Free list에 새로운 가용 블록을 리스트의 맨 처음에 추가한다.</em>
    </p>
    
    
    
    ```c
    void putFreeBlock(void* bp){
        SUCC_FREEP(bp) = free_listp;
        PREC_FREEP(bp) = NULL;
        PREC_FREEP(free_listp) = bp;
        free_listp = bp;
    }
    ```
<br>

- __void removeBlock(void *bp)__
    
    > 할당되거나 연결되는 가용 블록을 free list에서 없앤다.
    > 
    
    free list 상에서 해당 블록의 이전, 이후 블록을 서로 이어준다.
    
    ```c
    void removeBlock(void *bp){
        // free list의 첫번째 블록을 없앨 때
        if (bp == free_listp){
            PREC_FREEP(SUCC_FREEP(bp)) = NULL;
            free_listp = SUCC_FREEP(bp);
        }
        // free list 안에서 없앨 때
        else{
            SUCC_FREEP(PREC_FREEP(bp)) = SUCC_FREEP(bp);
            PREC_FREEP(SUCC_FREEP(bp)) = PREC_FREEP(bp);
        }
    }
    ```

<br>

- __static void* find_fit(size_t asize)__
    
    > First fit 방식으로 구현한다. Free list에서 유일한 할당 블록은 리스트 맨 뒤의 프롤로그 블록이므로, 할당 블록을 만나면 모든 list 노드들을 다 확인한 것으로 볼 수 있다(종료조건).
    > 
    
    ```c
    static void* find_fit(size_t asize){
        /* First-fit */
        void* bp;
    
        // free list의 맨 뒤는 프롤로그 블록이다. Free list에서 유일하게 할당된 블록이므로 얘를 만나면 탐색 종료.
        for (bp = free_listp; GET_ALLOC(HDRP(bp)) != 1; bp = SUCC_FREEP(bp)){
            if(asize <= GET_SIZE(HDRP(bp))){
                return bp;
            }
        }
    
        // 못 찾으면 NULL을 리턴한다.
        return NULL;
    
    }
    ```

<br>

- __static void place(void* bp, size_t asize)__
    
    > 할당할 수 있는 free 블록을 free list에서 없애준다. 할당 후 만약 분할이 되었다면, 새 가용 리스트를 free list에 넣어준다.
    > 
    
    앞에서의 putFreeBlock이 새롭게 만들어진 free 블록을 free list에 넣는 작업이라면, place()는 직접적으로 힙에 할당 메모리를 집어넣는 작업이다.
    
    ```c
    static void place(void* bp, size_t asize){
        // 현재 할당할 수 있는 후보 가용 블록의 주소
        size_t csize = GET_SIZE(HDRP(bp));
    
        // 할당될 블록이므로 free list에서 없애준다.
        removeBlock(bp);
    
        // 분할이 가능한 경우
        if ((csize - asize) >= (2*DSIZE)){
            // 앞의 블록은 할당 블록으로
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            // 뒤의 블록은 가용 블록으로 분할한다.
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize-asize, 0));
            PUT(FTRP(bp), PACK(csize-asize, 0));
    
            // free list 첫번째에 분할된 블럭을 넣는다.
            putFreeBlock(bp);
        }
        else{
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
        }
    }
    ```