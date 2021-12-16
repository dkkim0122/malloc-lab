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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* 메모리 할당 시 기본적으로 header와 footer를 위해 필요한 더블워드만큼의 메모리 크기 */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
    기본 상수와 매크로
*/

/* 기본 단위인 word, double word, 새로 할당받는 힙의 크기 CHUNKSIZE를 정의한다 */
#define WSIZE          4       /* Word and header/footer size (bytes) */
#define DSIZE          8       /* Double word size (bytes) */
#define CHUNKSIZE      (1<<12) /* Extend heap by this amount : 4096bytes -> 4kib */
#define LISTLIMIT      20      /* 관리할 Seglist 내의 Free list 개수 */

#define MAX(x, y) ((x) > (y) ? (x) : (y))  // 최댓값 구하는 함수 매크로

/* header 및 footer 값(size + allocated) 리턴 */
#define PACK(size, alloc)   ((size) | (alloc))   

/* 주소 p에서의 word를 읽어오거나 쓰는 함수 */
#define GET(p)          (*(unsigned int*)(p))
#define PUT(p, val)     (*(unsigned int*)(p) = (val))

/* header or footer에서 블록의 size, allocated field를 읽어온다 */
#define GET_SIZE(p)     (GET(p) & ~0x7) 
#define GET_ALLOC(p)    (GET(p) & 0x1)    

/* 블록 포인터 bp를 인자로 받아 블록의 header와 footer의 주소를 반환한다 */
#define HDRP(bp)    ((char*)(bp) - WSIZE) 
#define FTRP(bp)    ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 블록 포인터 bp를 인자로 받아 이후, 이전 블록의 주소를 리턴한다 */
#define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE))) // (char*)(bp) + GET_SIZE(지금 블록의 헤더값)
#define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) // (char*)(bp) - GET_SIZE(이전 블록의 풋터값)

/* segregated list 내에서 SUCC와 PREC의 포인터를 반환 */
#define PRED_FREE(bp)   (*(char**)(bp))
#define SUCC_FREE(bp)   (*(char**)(bp + WSIZE))

/* global variable & functions */
static char* heap_listp = NULL; // 항상 prologue block을 가리키는 정적 전역 변수 설정
static void* segregated_lists[LISTLIMIT];  // 20개의 size class에 대한 free list들 생성.

static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t newsize);
static void insert_block(void* ptr, size_t size);
static void remove_block(void* ptr);

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 각각의 free list들에 대해 NULL로 초기화
    for (int class = 0; class < LISTLIMIT; class++){
        segregated_lists[class] = NULL;
    }

    /* 메모리에서 4words를 가져오고 이걸로 빈 가용 리스트 초기화 */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp, 0);  // Alignment padding. 더블 워드 경계로 정렬된 미사용 패딩.
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  // prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  // prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));      // epliogue header
    heap_listp += 2*WSIZE; // heap_listp를 탐색의 시작점으로 둔다. 


    /* 그 후 CHUNKSIZE만큼 힙을 확장해 초기 가용 블록을 생성한다. */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) //실패하면 -1 리턴
        return -1;

    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
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
    if ((bp = find_fit(asize)) != NULL){  // first fit으로 추적한다.
        place(bp, asize);  // 필요하다면 분할하여 할당한다.
        return bp;
    }

    // 만약 맞는 크기의 가용 블록이 없다면 새로 힙을 늘려서 새 힙에 메모리를 할당한다.
    extendsize = MAX(asize, CHUNKSIZE);  // 둘 중 더 큰 값으로 사이즈를 정한다.
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) 
        return NULL;
    place(bp, asize);
    return bp;
}

/*
    extend_heap : 워드 단위 메모리를 인자로 받아 힙을 늘려준다.
*/
static void* extend_heap(size_t words){ // 워드 단위로 받는다.
    char* bp;
    size_t size;
    
    /* 더블 워드 정렬에 따라 메모리를 mem_sbrk 함수를 이용해 할당받는다. */
    size = (words % 2) ? (words + 1) * WSIZE : (words) * WSIZE; // size를 짝수 word && byte 형태로 만든다.
    if ((long)(bp = mem_sbrk(size)) == -1) // 새 메모리의 첫 부분을 bp로 둔다. 주소값은 int로는 못 받아서 long으로 casting한 듯.
        return NULL;
    
    /* 새 가용 블록의 header와 footer를 정해주고 epilogue block을 가용 블록 맨 끝으로 옮긴다. */
    PUT(HDRP(bp), PACK(size, 0));  // 헤더. 할당 안 해줬으므로 0으로.
    PUT(FTRP(bp), PACK(size, 0));  // 풋터.
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 새 에필로그 헤더

    insert_block(bp, size);  // 새로운 가용 블록을 seglist에 넣어준다.

    /* 만약 이전 블록이 가용 블록이라면 연결시킨다. */
    return coalesce(bp);
}

/*
    insert_block(bp) : 새 free 블록을 size에 맞는 class를 찾아 해당 seglist에 집어넣는다.
*/
static void insert_block(void* bp, size_t size){
    int class = 0;

    /*
        insert_prt -> 내가 넣을 자리 -> search_ptr
    */
    void* search_ptr = bp;   // 내가 넣을 자리의 뒷 포인터
    void* insert_ptr = NULL;  // 내가 넣을 자리의 앞 포인터

    /*
        size에 맞는 seglist를 구한다.
        ex. size : 15 = 1111 / 42 = 101010
        >>= 1, list = 1: 111 / 10101
        >>= 1, list = 2: 11 / 1010
        >>= 1, list = 3: 1 / 101
        >>= 1, list = 4: 0 / 10   -> size 15는 segregated_lists[4]로.
        >>= 1, list = 5: 0 / 1
        >>= 1, list = 6: 0 / 0  -> size 42는 segregated_lists[6]으로.
    */
    while ((class < LISTLIMIT - 1) && (size > 1)){
        size >>= 1;
        class++;
    }

    /* 해당 free 블록을 둘 자리를 찾는다. */
    search_ptr = segregated_lists[class];
    // seglist의 맨 끝이거나, 사이즈가 나보다 큰 블록을 만나면(seglist의 블록들은 오름차순 정렬) 
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))){
        insert_ptr = search_ptr;
        search_ptr = SUCC_FREE(search_ptr);
    }

    /* 해당 free 블록을 seglist 내 앞 뒤 free block과 link시킨다. */
    // 해당 자리가 seglist 맨 뒤가 아니면
    if (search_ptr != NULL){  
        // case 1 : 해당 자리가 seglist 중간에 있어 앞뒤 블록이 있는 경우
        if (insert_ptr != NULL){  
            SUCC_FREE(bp) = PRED_FREE(search_ptr);
            PRED_FREE(bp) = insert_ptr;
            PRED_FREE(search_ptr) = bp;
            SUCC_FREE(insert_ptr) = bp;
        }
        // case 2 : 맨 앞이고, 뒤에 블록들이 있는 경우 
        else{
            SUCC_FREE(bp) = search_ptr;
            PRED_FREE(bp) = NULL;        // seglist의 맨 앞 블록은 PRED가 NULL을 가리킨다.
            PRED_FREE(search_ptr) = bp;
            segregated_lists[class] = bp; // seglist의 맨 앞을 bp로 바꾼다.
        }
    }
    // 해당 자리가 seglist의 맨 뒤면(가장 size가 큰 블록)
    else{
        // case 3 : 맨 뒤이고 그 앞에 블록들이 있는 경우
        if (insert_ptr != NULL){
            SUCC_FREE(bp) = NULL;       // seglist의 맨 뒷 블록은 SUCC가 NULL을 가리킨다.
            PRED_FREE(bp) = insert_ptr;
            SUCC_FREE(insert_ptr) = bp;
        }
        // case 4 : 내가 유일한 seglist 안의 블록일 때
        else{
            SUCC_FREE(bp) = NULL;
            PRED_FREE(bp) = NULL;
            segregated_lists[class] = bp;
        }
    }

    return;
}

/*
    remove_block(bp) : 해당 가용 블록을 할당시키거나 연결시키기 위해 seglist에서 제외시킨다.
*/
static void remove_block(void* bp){
    int class = 0;
    size_t size = GET_SIZE(HDRP(bp));

    /*
        사실 그냥 지우는 거는 class를 사용하지 않아도 된다.
        다만, bp가 seglist의 맨 앞에 있을 때, 해당 seglist에 접근하기 위해서 
        배열 포인터가 가리키는 주소를 바꿔줄 필요가 있기 때문에 size class를 찾아야 한다.
    */
    while ((class < LISTLIMIT - 1) && (size > 1)){
        size >>= 1;
        class++;
    }

    // 내가 지울 블록이 맨 뒤가 아닐 때
    if (SUCC_FREE(bp) != NULL){
        // case 1 : 내가 지울 블록이 seglist 중간에 있을 때
        if(PRED_FREE(bp) != NULL){
            PRED_FREE(SUCC_FREE(bp)) = PRED_FREE(bp);  // 내 뒤를 내 앞과
            SUCC_FREE(PRED_FREE(bp)) = SUCC_FREE(bp);  // 내 앞을 내 뒤와 연결
        }
        // case 2 : 내가 지울 블록이 seglist 맨 앞에 있을 때
        else{
            PRED_FREE(SUCC_FREE(bp)) = NULL;  // 내 뒤를 seglist 맨 앞으로 만들기
            segregated_lists[class] = SUCC_FREE(bp);
        }
    }
    // 내가 지울 블록이 맨 뒤일 때
    else{
        // case 3 : 내가 지울 블록이 seglist 맨 뒤일 때(앞에 블록들이 있을 때)
        if (PRED_FREE(bp) != NULL){
            SUCC_FREE(PRED_FREE(bp)) = NULL;  // 내 앞을 맨 뒤로 만들기
        }
        // case 4 : 내가 지울 블록이 seglist의 유일한 블록일 때
        else{
            segregated_lists[class] = NULL;
        }
    }

    return;
}

/*
    coalesce(bp) : 반환되거나 힙이 늘어나면서 생성된 가용 블록을 
    앞뒤 가용 블록과 연결하고 연결된 가용 블록의 주소를 리턴한다.
    앞귀 가용 블록을 seglist에서 뺀 다음, 연결된 새 가용 블록을 새로 seglist에 넣는다.
*/
static void* coalesce(void* bp){
    // 직전 블록의 footer, 직후 블록의 header를 보고 가용 여부를 확인.
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));  // 직전 블록 가용 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // 직후 블록 가용 여부
    size_t size = GET_SIZE(HDRP(bp));

    // case 1 : 직전, 직후 블록이 모두 할당 -> 해당 블록만 seglist에 넣어주면 된다.
    
    // case 2 : 직전 블록 할당, 직후 블록 가용
    if(prev_alloc && !next_alloc){
        remove_block(NEXT_BLKP(bp));    // free 상태였던 직후 블록을 seglist에서 제거한다.
        
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // case 3 : 직전 블록 가용, 직후 블록 할당
    else if(!prev_alloc && next_alloc){
        remove_block(PREV_BLKP(bp));    // 직전 블록을 seglist에서 제거한다.

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp); 
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));  
    }

    // case 4 : 직전, 직후 블록 모두 가용
    else if (!prev_alloc && !next_alloc) {
        remove_block(PREV_BLKP(bp));
        remove_block(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));  
        PUT(FTRP(bp), PACK(size, 0));  
    }

    // 연결된 새 가용 블록을 seglist에 추가한다.
    insert_block(bp, size);

    return bp;
}

/*
    first_fit : sizeclass의 맨 처음부터 탐색하여 해당 asize에 맞는 size class를 찾아내고,
    만약 해당 seglist에서 원하는 사이즈의 free 블록을 찾지 못하면 다음 size class의 seglist로 넘어간다.
*/
static void* find_fit(size_t asize){
    /* First-fit */
    void* bp;

    int class = 0;
    size_t searchsize = asize;

    // 맞는 size class에 해당하는 seglist를 찾는다.
    while(class < LISTLIMIT){
        // 맞는 seglist를 찾았을 때
        // size_class가 최대거나, 해당 asize에 맞는 class를 찾고 거기 free 블록이 있을 때
        if ((class == LISTLIMIT - 1) || ((searchsize <= 1) && (segregated_lists[class] != NULL))){
            bp = segregated_lists[class];

            // 해당 seglist를 first fit
            while ((bp != NULL) && (asize > GET_SIZE(HDRP(bp)))){
                bp = SUCC_FREE(bp);  // 요구 메모리보다 더 큰 사이즈를 할당해야 하므로 SUCC를 본다.
            }
            if (bp != NULL){
                return bp;
            }
        }
        searchsize >>= 1;
        class++;
    }

    // 못 찾으면 NULL을 리턴한다.
    return NULL;
}


/*
    place(bp, size) : 할당할 수 있는 free 블록을 seglist에서 없애준다. 
    할당 후 만약 분할이 되었다면, 새 가용 블록을 seglist에 넣어준다.
*/
static void place(void* bp, size_t asize){
    // 현재 할당할 수 있는 후보 가용 블록의 주소
    size_t csize = GET_SIZE(HDRP(bp));

    remove_block(bp); // 할당될 블록이므로 seglist에서 없애준다.

    // 분할이 가능한 경우
    if ((csize - asize) >= (2*DSIZE)){
        // 앞의 블록은 할당 블록으로
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // 뒤의 블록은 가용 블록으로 분할한다.
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert_block(bp, (csize - asize));

    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}



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


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
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

    // 원래 메모리 크기보다 적은 크기를 realloc하면 크기에 맞는 메모리만 할당되고 나머지는 안 된다. 
    if (size < copySize)
      copySize = size;

    memcpy(newptr, oldptr, copySize);  // newptr에 oldptr를 시작으로 copySize만큼의 메모리 값을 복사한다.
    mm_free(oldptr);  // 기존의 힙을 반환한다.
    return newptr;
}

