#ifndef __NB_ALLOC__
#define __NB_ALLOC__
/****************************************************
				ALLOCATOR PARAMETES
****************************************************/

#ifndef MIN_ALLOCABLE_BYTES
#define MIN_ALLOCABLE_BYTES 8ULL 
#endif
#ifndef MAX_ALLOCABLE_BYTE
#define MAX_ALLOCABLE_BYTE  16384ULL 

#ifndef NUM_LEVELS
#define NUM_LEVELS  20ULL 
#endif


#define PAGE_SIZE (4096)

//#define DEBUG

typedef long long nbint; 


typedef struct _node{
    volatile nbint val; 
    unsigned int mem_start; 
    unsigned int mem_size;
    unsigned int pos; 
    unsigned int num_leafs;
    char pad[40];
} node;


typedef struct _taken_list_elem{
    struct _taken_list_elem* next;
    node* elem;
}taken_list_elem;

typedef struct _taken_list{
    struct _taken_list_elem* head;
    unsigned number;
}taken_list;


extern __thread unsigned myid;
extern unsigned int number_of_leaves;

void  bd_xx_free(void* n);
void* bd_xx_malloc(size_t pages);

#ifdef DEBUG
extern unsigned long long *node_allocated; 
extern nbint *size_allocated;
#endif


#endif
