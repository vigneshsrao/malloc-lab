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
    "bi0s",
    /* First member's full name */
    "Vignesh S Rao",
    /* First member's email address */
    "vigneshsrao5@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

void* traverse();
void link_to_freelist(void* ptr);
void* coalesce(void* ptr);
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define PREV_IN_USE 1 /* Set if the previous chunk is not free */

void* heap_start;  /* Pointer to starting of the heap */
void* top;  /* Pointer to the top chunk */
void* freelist;

/* Macros for the chunk fields */
#define get_chunk(p) (p - (void *)ALIGNMENT)
#define prev_size(p) (*(size_t*)p)
#define chunk_size(p) (*(size_t *)((size_t)p + (size_t)WSIZE))
#define fd(p) (*(size_t*)((size_t)p + (size_t)2*WSIZE))
#define bk(p) (*(size_t*)((size_t)p + (size_t)3*WSIZE))
#define real_size(p) (chunk_size(p) & ~0x7)
#define next_chunk(p) ((size_t *)(p + real_size(p)))
#define prev_chunk(p) ((size_t *)(p - prev_size(p)))
#define ALLOC(p) (chunk_size(p) |= PREV_IN_USE)
#define GET_ALLOC(p) (chunk_size(p) & PREV_IN_USE)
#define UNALLOC(p) (chunk_size(p) &= ~0x7)

void mm_checkheap()
{
  void* chunk=heap_start;
  while(chunk!=top){
    printf("\np=%p\t",chunk);
    printf("prev_size_p=0x%x\t",prev_size(chunk));
    printf("size_p=0x%x",chunk_size(chunk));
    chunk+=chunk_size(chunk);
  }
  printf("\ntop=%p\t",chunk);
  printf("prev_size_top=0x%x\t",prev_size(chunk));
  printf("size_top=0x%x\n",chunk_size(chunk));
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //printf("\n------%d\n----",SIZE_T_SIZE);
    top=mem_sbrk(ALIGN(0x31000));
    heap_start=top;
    if(top==(void*)-1){
      printf("\n[ERROR] mem_sbrk failed\n");
      exit(1);
    }
    chunk_size(top) = (size_t)ALIGN(0x31000-2*WSIZE);
    chunk_size(top) |=PREV_IN_USE;
    ALLOC(next_chunk(top));
    freelist=mm_malloc(4*8*3);
    memset(freelist,0,96);
    fd(freelist)=(size_t)freelist;
    bk(freelist)=(size_t)freelist;
    //mm_checkheap();
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    if(((int)chunk_size(top))<newsize){
      printf("[ERROR] Out of memory !");
      exit(1);
    }
    void* p = top;
    size_t prev=real_size(top);
    top += newsize;
    chunk_size(top)=prev-newsize;
    chunk_size(p)=newsize;
    ALLOC(top);
    ALLOC(p);
    //mm_checkheap();
    return (void *)((char *)p + SIZE_T_SIZE);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  //printf("%zx\n",(size_t)&chunk_size(next_chunk(ptr-0x10)));
  ptr = (void*)get_chunk(ptr);
  ptr = coalesce(ptr);
  if (ptr==NULL){
    ALLOC(top);
    return;
  }
  UNALLOC(next_chunk(ptr));
  prev_size(next_chunk(ptr)) = real_size(ptr);
  link_to_freelist(ptr);

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

void* traverse()
{
    printf("%p\n",freelist);
    void *tmp=freelist;
    while((void*)fd(tmp)!=freelist){
      tmp=(void*)fd(tmp);
    }
    return tmp;
}

void link_to_freelist(void* ptr)
{
  fd(ptr)=(size_t)fd(freelist);
  bk(ptr)=(size_t)freelist;
  fd(freelist)=(size_t)ptr;
  bk((void*)fd(ptr))=(size_t)ptr;
}

void unlink_from_freelist(void* ptr)
{
  bk(fd(ptr))=bk(ptr);
  fd(bk(ptr))=fd(ptr);
}

void* coalesce(void* ptr)
{
  size_t prev_alloc=GET_ALLOC(ptr);
  //void* tmp=next_chunk(ptr);
  size_t next_alloc=GET_ALLOC(next_chunk((void*)next_chunk(ptr)));

  //printf("------%zx\n%zx----------%zx-----%p\n",prev_alloc,next_alloc,real_size(next_chunk(ptr)),ptr);
  // if(prev_alloc && next_alloc)
  //   return ptr;

  if(!prev_alloc && next_alloc){
    chunk_size(prev_chunk(ptr)) += real_size(ptr);
    unlink_from_freelist(prev_chunk(ptr));
    ptr=prev_chunk(ptr);
    prev_size(next_chunk(ptr))=chunk_size(prev_chunk(ptr));
  }

  if(next_chunk(ptr)==top){
    chunk_size(ptr)=real_size(ptr)+real_size(top);
    top=ptr;
    return NULL;
  }

  else if(prev_alloc && !next_alloc){
    unlink_from_freelist(next_chunk(ptr));
    chunk_size(ptr)=real_size(ptr)+real_size(next_chunk(ptr));
    prev_size(next_chunk(ptr))=real_size(ptr);
  }

  else if(!prev_alloc && !next_alloc){
    unlink_from_freelist(next_chunk(ptr));
    unlink_from_freelist(prev_chunk(ptr));
    chunk_size(ptr)=real_size(ptr)+real_size(next_chunk(ptr));
    chunk_size(prev_chunk(ptr)) += real_size(ptr);
    ptr=prev_chunk(ptr);
    prev_size(next_chunk(ptr))=real_size(ptr);
  }

  return ptr;
}
