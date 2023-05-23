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
#endif

#ifndef NUM_LEVELS
#define NUM_LEVELS  20ULL 
#endif


#define PAGE_SIZE (4096)

//#define DEBUG



/*log2 malato*/
static const unsigned int tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5};

static inline unsigned int log2_ (unsigned long value){
    return 63 - __builtin_clzl(value);
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return tab64[((unsigned long)((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
}

static inline int convert_to_level(unsigned long long size){
	unsigned long long tmp = (size - 1)/PAGE_SIZE + 1;
	return (int) log2_(tmp);	
}

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
