// R. Jesse Chaney
// rchaney@pdx.edu

#include "vikalloc.h"

// Returns the size of the structure, in bytes.
#define BLOCK_SIZE (sizeof(heap_block_t))

// Returns a pointer to the data within a block.
#define BLOCK_DATA(__curr) (((void *) __curr) + (BLOCK_SIZE))

// Returns a pointer to the structure containing the data
#define DATA_BLOCK(__data) ((heap_block_t *) (__data - BLOCK_SIZE))

// Returns 0 (false) if the block is NOT free, else 1 (true).
#define IS_FREE(__curr) ((__curr -> size) == 0)

#define PTR "0x%07lx"
#define PTR_T PTR "\t"

static heap_block_t *block_list_head = NULL;
static heap_block_t *block_list_tail = NULL;
static void *low_water_mark = NULL;
static void *high_water_mark = NULL;

// only used in next-fit algorithm
// ******************************************************
// ******************************************************
// This is the variable you'll use to keep track of where the
// next-fit algorithm left off when searching through the linked
// list of heap_block_t sturctures.
// ******************************************************
// ******************************************************
static heap_block_t *next_fit = NULL;

static uint8_t isVerbose = FALSE;
static vikalloc_fit_algorithm_t fit_algorithm = NEXT_FIT;
static FILE *vikalloc_log_stream = NULL;

static void init_streams(void) __attribute__((constructor));

static size_t min_sbrk_size = MIN_SBRK_SIZE;

static void 
init_streams(void)
{
    // Don't change this.
    vikalloc_log_stream = stderr;
}

size_t
vikalloc_set_min(size_t size)
{
    // Don't change this.
    if (0 == size) {
        // just return the current value
        return min_sbrk_size;
    }
    if (size < (BLOCK_SIZE + BLOCK_SIZE)) {
        // In the event that it is set to something silly small.
        size = MAX(BLOCK_SIZE + BLOCK_SIZE, SILLY_SBRK_SIZE);
    }
    min_sbrk_size = size;

    return min_sbrk_size;
}

void 
vikalloc_set_algorithm(vikalloc_fit_algorithm_t algorithm)
{
    // Don't change this.
    fit_algorithm = algorithm;
    if (isVerbose) {
        switch (algorithm) {
        case FIRST_FIT:
            fprintf(vikalloc_log_stream, "** First fit selected\n");
            break;
        case BEST_FIT:
            fprintf(vikalloc_log_stream, "** Best fit selected\n");
            break;
        case WORST_FIT:
            fprintf(vikalloc_log_stream, "** Worst fit selected\n");
            break;
        case NEXT_FIT:
            fprintf(vikalloc_log_stream, "** Next fit selected\n");
            break;
        default:
            fprintf(vikalloc_log_stream, "** Algorithm not recognized %d\n"
                    , algorithm);
            fit_algorithm = FIRST_FIT;
            break;
        }
    }
}

void
vikalloc_set_verbose(uint8_t verbosity)
{
    // Don't change this.
    isVerbose = verbosity;
    if (isVerbose) {
        fprintf(vikalloc_log_stream, "Verbose enabled\n");
    }
}

void
vikalloc_set_log(FILE *stream)
{
    // Don't change this.
    vikalloc_log_stream = stream;
}

void *
vikalloc(size_t size)
{
    heap_block_t *curr = NULL;
    heap_block_t *split = NULL;
    size_t var = 0;

    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry: size = %lu\n"
                , __LINE__, __FUNCTION__, size);
    }

    if (0 == size) {
//#error This is toooooo easy.
//#error You are almost done.
        return NULL;
    }

    // 01/21/2023
    if (block_list_head == NULL){
        // MAKES NEW VAR
        var = (((size + BLOCK_SIZE) / min_sbrk_size) + 1) * min_sbrk_size;
        // ALLOCATE NEW MEMORY
	    curr = sbrk(var);
        // FIXES WATER MARKS
        low_water_mark = curr;
        high_water_mark = var + low_water_mark;
        // SETS CURR POINTER
        curr->size = size;
        curr->capacity = var - BLOCK_SIZE;
        // SET HEAD AND TAIL POINTERS
        block_list_head = curr;
        block_list_tail = curr;
        // CURR PTRS
        curr->next = NULL;  // NULL STATEMT 1/23
        curr->prev = NULL;
        // SETS NEXT-FIT
        next_fit = block_list_head;   // CASE 1 OF NEXT-FIT

        return BLOCK_DATA(curr);    // ADDED RETURN STATEMENT 1/23
        }
    else {
        curr = next_fit;
        do {
            // CHECK IF FREE BLOCK
            if (IS_FREE(curr) && curr->capacity >= size){
                curr->size = size;
                return BLOCK_DATA(curr); // 1.23 added
            }
            // CHECKS SPLIT SCENARIO
            else if ((curr->capacity - curr->size) >= size + BLOCK_SIZE){
                // MAKES NEW BLOCK
                split = (void*) (BLOCK_DATA(curr) + curr->size);
                // ADJUST NEW BLOCK VARS
                split->size = size;
                split->capacity = curr->capacity - curr->size - BLOCK_SIZE;
                curr->capacity = curr->size;

                // CHECKS IF END OF LIST
                if (curr->next == NULL){
                    curr->next = split;
                    split->next = NULL;
                    split->prev = curr;
                    block_list_tail = split;
                }
                else {
                    split->next = curr->next;
                    split->next->prev = split;
                    curr->next = split;
                    split->prev = curr;
                }
                // ADJUST NEXT_FIT
                next_fit = split;
        
                return BLOCK_DATA(split); //1.23 added
            }
            // IF WE REACH END OF DLL
            if (curr->next == NULL){
                curr = block_list_head;
            }
            // IF CONTINUING IN LIST
            else {
                curr = curr->next;
            }
        } while (curr != next_fit);

        // IF MORE MEMORY NEEDS TO BE ALLOCATED W/SBRK()
        // ALLOCATE MEMORY
        var = (((size + BLOCK_SIZE) / min_sbrk_size) + 1) * min_sbrk_size;
	    curr = sbrk(var);
        // FIX HIGH WATER MARK
        high_water_mark += var;
        // FIX CURR AND TAIL POINTERS
        curr->size = size;
        curr->capacity = var - BLOCK_SIZE;
        block_list_tail->next = curr;
        curr->next = NULL;
        curr->prev = block_list_tail;
        block_list_tail = curr;

        return BLOCK_DATA(curr);
    }

//#error Lots and lots of stuff goes in here, like searching and splitting.
    
    if (isVerbose) {
        fprintf(vikalloc_log_stream, "<< %d: %s exit: size = %lu\n", __LINE__, __FUNCTION__, size);
    }

//#error You need to return a pointer to the data, not a pointer to the structure
    return BLOCK_DATA(curr); // Just a place holder.
}

void 
vikfree(void *ptr)
{
    heap_block_t *curr = NULL;

    if (ptr == NULL) {
        return;
    }

//#error Do something like
//#error            curr = SOME_COOL_MACRO(ptr);
//#error You should make sure that curr is not NULL before proceeding.
    curr = DATA_BLOCK(ptr);
    if (curr == NULL){
        return;
    }
    if (IS_FREE(curr)) {
        if (isVerbose) {
            fprintf(vikalloc_log_stream, "Block is already free: ptr = " PTR "\n"
                    , (long) (ptr - low_water_mark));
        }
        return;
    }

    curr->size = 0;
    next_fit = curr;
    
    // COALESCE UP 
    if (curr->next && IS_FREE(curr->next)) {
        if (curr->next == next_fit){  
            next_fit = curr; 
            }
        curr->capacity += curr->next->capacity + BLOCK_SIZE;

        if (curr->next->next == NULL){ // IF AT END OF LIST, TAIL = CURR
            block_list_tail = curr;
            curr->next = NULL;
        }
        else {
            curr->next = curr->next->next;
            curr->next->prev = curr;
        }
    }

    // COALESCE DOWN
    if (curr->prev && IS_FREE(curr->prev)){
        curr->prev->size = 1; // BYPASS LINE 242
        vikfree(BLOCK_DATA(curr->prev));
    }

//#error Remember to return the pointer to the data, NOT the pointer to the structure
    if (isVerbose) {
        fprintf(vikalloc_log_stream, "<< %d: %s exit: ptr = %p\n", __LINE__, __FUNCTION__, ptr);
    }

    return;
}

void 
vikalloc_reset(void)
{
    if (low_water_mark != NULL) {
        if (isVerbose) {
            fprintf(vikalloc_log_stream, "*** Resetting all vikalloc space ***\n");
        }
//#error You need to reset all the pointers and values back to initial values.
    brk(low_water_mark);
    block_list_head = NULL;
    block_list_tail = NULL;
    low_water_mark = NULL;
    high_water_mark = NULL;
    next_fit = NULL;
    }
}

void *
vikcalloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;

//#error This is alomst tooooooo easy
    if (nmemb == 0 || size == 0){
        return NULL;
    }
    
    ptr = vikalloc(nmemb *size);
    memset(ptr, 0, nmemb *size);
    
    return ptr;
}

void *
vikrealloc(void *ptr, size_t size)
{
    heap_block_t *curr = NULL;
    heap_block_t *temp = NULL;
    
//#error Remember, the user passes you a pointer to the data, NOT the structure.
//#error You need to return a pointer to data
    if (ptr == NULL){
        ptr = vikalloc(size);
        return ptr;
    }
    if (size == 0){
        return NULL;
    }

    curr = DATA_BLOCK(ptr);     // CHECK CHECK CHECK CHECK
    if (curr->capacity >= size){
        curr->size = size;
    }
    else{
        temp = vikalloc(size);
        memcpy(temp, ptr, size);
        vikfree(ptr);
        return temp;
    }

    // IF AVAILABLE IN CAPACITY PUT IT IN AS SIZE, OTHERWISE CALL VIKALLOC AND USE MEMCOPY TO COPY ODL DATA INTO NEW
    return ptr;
}

void *
vikstrdup(const char *s) {
//#error Can you do this in one line? I was dared to and did.
    return strcpy(vikalloc(strlen(s) + 1), s);
    }

// This is unbelievably ugly.
#include "vikalloc_dump.c"
