#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <pthread.h>
#include "nballoc.h"
#include "utils.h"




#define ROOT            (tree[1])

#define parent_idx_by_idx(n)   (n >> 1)
#define parent_ptr_by_idx(n)   (tree[parent_idx_by_idx(n)])

#define is_left_by_idx(n)      (1ULL & (~(n)))

#define level(n)        ( (overall_height) - (log2_(( (n)->mem_size) / (MIN_ALLOCABLE_BYTES )) ))
#define level_by_idx(n) ( 1 + (log2_(n)))


/***************************************************
*               LOCAL VARIABLES
***************************************************/

static node* volatile tree = NULL;
static node* volatile free_tree = NULL; 
static node* volatile real_tree = NULL; 
static node* volatile real_free_tree = NULL;
static unsigned long overall_memory_size;
static unsigned int number_of_nodes; 
unsigned int number_of_leaves; 
static void* volatile overall_memory = NULL;
static volatile unsigned long levels = NUM_LEVELS;

static unsigned int overall_height;
static unsigned int max_level;

volatile int init_phase = 0;

extern taken_list* takenn;

static void init_tree(unsigned long number_of_nodes);
static unsigned int alloc2(unsigned int, long long size);
static void internal_free_node2(unsigned int n, unsigned int upper_bound);

#ifdef DEBUG
nbint  *size_allocated;
unsigned long long *node_allocated;
#endif

__thread unsigned int tid=-1;
unsigned int partecipants=0;


void init(){
    void *tmp_overall_memory;
    void *tmp_real_tree;
    void *tmp_tree;
    void *tmp_real_free_tree;
    void *tmp_free_tree;
    bool first = false;
    number_of_nodes = (1<<levels) -1;
    
    overall_height = levels;
    
    number_of_leaves = 1 << (levels - 1);
    
    overall_memory_size = MIN_ALLOCABLE_BYTES * number_of_leaves;
    
    if(overall_memory_size < MAX_ALLOCABLE_BYTE){
		printf("Not enough levels\n");
		abort();
	}
	
	max_level = overall_height - log2_(MAX_ALLOCABLE_BYTE/MIN_ALLOCABLE_BYTES);
    
   if(init_phase ==  0 && __sync_bool_compare_and_swap(&init_phase, 0, 1)){

        tmp_overall_memory = mmap(NULL, overall_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        first = true;
        if(tmp_overall_memory == MAP_FAILED)
            abort();
        else if(!__sync_bool_compare_and_swap(&overall_memory, NULL, tmp_overall_memory))
                munmap(tmp_overall_memory, overall_memory_size);
           
        tmp_real_tree = mmap(NULL,64+(1+number_of_nodes)*sizeof(node), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        
        if(tmp_real_tree == MAP_FAILED)
            abort();
        else if(!__sync_bool_compare_and_swap(&real_tree, NULL, tmp_real_tree))
                munmap(tmp_real_tree, 64+(1+number_of_nodes)*sizeof(node));

        tmp_tree = tmp_real_tree;


        tmp_real_free_tree = mmap(NULL,64+(number_of_leaves)*sizeof(node), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        
        if(tmp_real_free_tree == MAP_FAILED)
            abort();
        else if(!__sync_bool_compare_and_swap(&real_free_tree, NULL, tmp_real_free_tree))
                munmap(tmp_real_free_tree, 64+(number_of_leaves)*sizeof(node));

        tmp_free_tree = tmp_real_free_tree; 


#ifdef DEBUG
    node_allocated = mmap(NULL, sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    size_allocated = mmap(NULL, sizeof(nbint), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      
    __sync_fetch_and_and(node_allocated,0);
    __sync_fetch_and_and(size_allocated,0);
    
    printf("Debug mode: ON\n");
#endif


        __sync_bool_compare_and_swap(&tree, NULL, tmp_tree);
        __sync_bool_compare_and_swap(&free_tree, NULL, tmp_free_tree);
        __sync_bool_compare_and_swap(&init_phase, 1, 2);
    }

    while(init_phase < 2);
    if(init_phase == 2)
        init_tree(number_of_nodes);
    else
        return;
    


    
    if(first){
        printf("dsa-wf: UMA Init complete\n");
        printf("\t Total Memory = %lu\n", overall_memory_size);
        printf("\t Levels = %u\n", overall_height);
        printf("\t Leaves = %u\n", (number_of_nodes+1)/2);
        printf("\t Min size %llu at level %u\n", MIN_ALLOCABLE_BYTES, overall_height);
        printf("\t Max size %llu at level %u\n", MAX_ALLOCABLE_BYTE, overall_height - log2_(MAX_ALLOCABLE_BYTE/MIN_ALLOCABLE_BYTES));
    }
}



void __attribute__ ((constructor(500))) premain(){
    init();
}


static void init_tree(unsigned long number_of_nodes){
    printf("NUM NODES %lu\n", number_of_nodes);
    int i=0;

    ROOT.mem_start = 0;
    ROOT.mem_size = overall_memory_size;
    ROOT.pos = 1;
    ROOT.val = (number_of_nodes+1)/2;;
    ROOT.num_leafs = (number_of_nodes+1)/2;

    for(i=2;i<=number_of_nodes;i++){
        tree[i].pos = i;
        node parent = parent_ptr_by_idx(i);
        tree[i].mem_size         = parent.mem_size / 2;
        tree[i].num_leafs        = parent.num_leafs / 2;
        tree[i].val              = parent.num_leafs / 2;
	}
}


void* bd_xx_malloc(size_t byte){
    unsigned int starting_node, actual;
    unsigned int leaf_position;

    if(tid == -1){
		tid = __sync_fetch_and_add(&partecipants, 1);
     }

    if( byte > MAX_ALLOCABLE_BYTE || byte > overall_memory_size)
        return NULL;
    
    byte = upper_power_of_two(byte);

    if( byte < MIN_ALLOCABLE_BYTES )
        byte = MIN_ALLOCABLE_BYTES;

    starting_node  = overall_memory_size / byte;   
    actual = alloc2(1, tree[starting_node].num_leafs);
    
    if(actual != 0){
        leaf_position = byte*(actual - overall_memory_size / byte)/MIN_ALLOCABLE_BYTES;
        free_tree[leaf_position].pos = actual;
        #ifdef DEBUG
            __sync_fetch_and_add(node_allocated,1);
            __sync_fetch_and_add(size_allocated,byte);
        #endif

        return ((char*) overall_memory) + leaf_position*MIN_ALLOCABLE_BYTES; 
    }        

 
    return NULL;
}




static unsigned int alloc2(unsigned int n, long long s){
    if(tree[n].num_leafs < s) return 0;



    if(__sync_add_and_fetch(&tree[n].val, -s) < 0){
        __sync_add_and_fetch(&tree[n].val, s);
        return 0;
    }

    if(s == tree[n].num_leafs) return n;

    unsigned int tmp = alloc2(n*2, s);
    if(tmp) return tmp;
    tmp = alloc2(n*2+1, s);
    if(tmp) return tmp;
    
     __sync_add_and_fetch(&tree[n].val, s);
    return 0;
}


void bd_xx_free(void* n){
    char * tmp = ((char*)n) - (char*)overall_memory;
    unsigned int pos = (unsigned long long) tmp;
    pos = pos / MIN_ALLOCABLE_BYTES;
    pos = free_tree[pos].pos;

    unsigned int child = pos*2;
    long long size = tree[pos].num_leafs;

    while(child<(number_of_nodes+1)){
        tree[child  ].val = tree[child  ].num_leafs;
        tree[child+1].val = tree[child+1].num_leafs;
        child *=2;
    }

    while(pos > 0){
        __sync_fetch_and_add(&tree[pos].val, size);
        pos /= 2;
    }

#ifdef DEBUG
	__sync_fetch_and_add(node_allocated,-1);
	__sync_fetch_and_add(size_allocated,-(n->mem_size));
#endif
}








