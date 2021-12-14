#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

---
<br>

## 목표 ##


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

- **void* malloc(size_t size)**
    
    > 성공 시, **적어도** size만큼의 메모리 블록들을 **초기화하지 않고** 리턴한다.
    > 
    - x86(32비트 모드)은 **8 byte(2 word)**, x86-64 컴퓨터는 16 byte(4 word)의 **정렬된 블록들을 기본 단위로 리턴한다.**
    - **mmap이나 munmap 또는 sbrk 함수를 이용해 명시적으로 힙 메모리를 할당하거나 반환한다.**
- **void free(void* p)**
    
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

$$U_{k} = {max_{i<=k} P_{i}}/{H_{k}}$$

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
		<img src = "\mnt\c\Users\Kim Dokyung\swjungle\project\malloc-lab\malloc-lab\image\Untitled(2).png" width="400" height="100"/>
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
	<img src = ".\image\Untitled(8).png" width="500" height="150"/>
</p>
<p align="center">
	<em>반환되었음에도 인접한 반환 블록이 서로 구분되어져 있다. 이럼 5word를 할당을 못한다.</em>
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

이를 이용해 기준 블록의 **헤더에서 한 칸만 뒤로 가면 이전 블록의 할당 여부를 알 수 있다!**

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

**명시적 가용 리스트 Implicit Free List**
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
- Segregated Free List 사용 시 블록들의 크기에 따라 Free List를 만들 수 있다. 이 때 Implicit List에서 활용되었던 것과 비슷하게 Linked List를 사용한다.