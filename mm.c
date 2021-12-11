/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team 9",
    /* First member's full name */
    "Kim Dokyung",
    /* First member's email address */
    "dkkim0122@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

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

/* 항상 prologue block을 가리키는 정적 전역 변수 설정 */
static char* heap_listp;

/*===================================================================*/

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 메모리에서 4word 가져오고 이걸로 빈 가용 리스트 초기화 */
    // 먼저 4word 크기의 메모리를 불러온다. heap_listp가 그 첫 주소를 가리킨다.
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;

    // 맨 앞의 1워드는 정보 0인 미사용 패딩.
    // 그 다음의 prologue block은 header와 footer로만 이루어진 2word짜리 블록
    // 맨 뒤는 헤더만으로 구성된 크기 0의 epilogue block
    PUT(heap_listp, 0);  // Alignment padding. 더블 워드 경계로 정렬된 미사용 패딩.
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  // prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  // prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));      // epliogue header
    heap_listp += (2*WSIZE);  //정적 전역 변수는 늘 prologue block을 가리킨다.

    /* 그 후 CHUNKSIZE만큼 힙을 확장해 초기 가용 블록을 생성한다. */
    // 새 힙을 CHUNKSIZE 바이트만큼의 WORD 개수만큼 늘려준다.
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) //실패하면 -1 리턴
        return -1;

    return 0;
}

/*
    coalesce(bp) : 해당 가용 블록을 앞뒤 가용 블록과 연결하고 
    연결된 가용 블록의 주소를 리턴한다.
*/
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
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));  // HDRP로 해도 된다.
        PUT(FTRP(bp), PACK(size, 0));  // 해당 블록 footer
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 직후 블록 header
        bp = PREV_BLKP(bp); // 블록 포인터를 직전 블록으로 옮긴다.
    }

    // case 4 : 직전, 직후 블록 모두 가용
    else{
        size += GET_SIZE(FTRP(PREV_BLKP(bp)))
                + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));  // 직전 블록 header
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));  // 직후 블록 footer
        bp = PREV_BLKP(bp);  // 블록 포인터를 직전 블록으로 옮긴다.
    }

    // 최종 가용 블록의 주소를 리턴한다.
    return bp;
}

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
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));  // 새 에필로그 헤더

    /* 만약 이전 블록이 가용 블록이라면 연결시킨다. */
    return coalesce(bp);
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize;
    size_t extendsize;
    char* bp;

    // 가짜 요청 spurious request 무시
    if (size == 0)
        return NULL;

    // 요청 사이즈에 header와 footer를 위한 dword 공간(SIZE_T_SIZE)을 추가한 후 align해준다.
    newsize = ALIGN(size + SIZE_T_SIZE);  

    // 할당할 가용 리스트를 찾아 필요하다면 분할해서 할당한다!
    if ((bp = first_fit(newsize)) != NULL){  // first fit으로 추적한다.
        place(bp, newsize);  // 필요하다면 분할하여 할당한다.
        return bp;
    }

    // 만약 맞는 크기의 가용 블록이 없다면 새로 힙을 늘려서 
    extendsize = MAX(newsize, CHUNKSIZE);  // 둘 중 더 큰 값으로 사이즈를 정한다.
    // extend_heap()은 word 단위로 인자를 받으므로 WSIZE로 나눠준다.
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) 
        return NULL;
    // 새 힙에 메모리를 할당한다.
    place(bp, newsize);
    return bp;
}

/*
    first_fit : 힙의 맨 처음부터 탐색하여 요구하는 메모리 공간보다 큰 가용 블록의 주소를 반환한다.
*/
static void* first_fit(size_t newsize){

    void* bp;

    // 프롤로그 블록에서 에필로그 블록 전까지 블록 포인터 bp를 탐색한다.
    // 블록이 가용 상태이고 사이즈가 요구 사이즈보다 크다면 해당 블록 포인터를 리턴
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (newsize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }

    // 못 찾으면 NULL을 리턴한다.
    return NULL;
}

/*
    place(bp, size)
    : 요구 메모리를 할당할 수 있는 가용 블록을 할당한다. 이 때 분할이 가능하면 분할한다.
*/
static void place(void* bp, size_t newsize){
    // 현재 할당할 수 있는 후보 가용 블록의 주소
    size_t csize = GET_SIZE(HDRP(bp));

    // 분할이 가능한 경우
    // -> 남은 메모리가 최소한의 가용 블록을 만들 수 있는 4word(16byte)가 되느냐.
    // header & footer : 1word씩, payload : 1word, 정렬 위한 padding : 1word = 4words
    if ((csize - newsize) >= (2*DSIZE)){
        // 앞의 블록은 할당 블록으로
        PUT(HDRP(bp), PACK(newsize, 1));
        PUT(FTRP(bp), PACK(newsize, 1));
        // 뒤의 블록은 가용 블록으로 분할한다.
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-newsize, 0));
        PUT(FTRP(bp), PACK(csize-newsize, 0));
    }
    // 분할할 수 없다면 남은 부분은 padding한다.
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 해당 블록의 size를 알아내 header와 footer의 정보를 수정한다.
    size_t size = GET_SIZE(HDRP(ptr));

    // header와 footer를 설정
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    // 만약 앞뒤의 블록이 가용 상태라면 연결한다.
    coalesce(ptr);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














