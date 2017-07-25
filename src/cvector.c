/*
 * File: cvector.c
 * Author: SWETHA REVANUR
 * ----------------------
 * Implementation of dynamically-allocated arrays in C.
 */

#include "cvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <search.h>

// a suggested value to use when given capacity_hint is 0
#define DEFAULT_CAPACITY 16

/* Type: struct CVectorImplementation
 * ----------------------------------
 * This definition completes the CVector type that was declared in
 * cvector.h. You fill in the struct with your chosen fields.
 */
typedef struct CVectorImplementation {
    void *data; // pointer to contiguous memory where data is in heap
    size_t size; // number of elements in CVector
    size_t capacity; // number of elements possible with current allocated memory
    size_t elemsz; // number of bytes required by each element
    CleanupElemFn clean; // cleanup function
} CVector;


/* Function: get_nth
 * -----------------
 * Purpose: Performs pointer arithmetic.
 * Parameters: pointer to a CVector, int index n
 * Return values: void * pointer to n
 */
void *get_nth(const CVector *cv, int n) {
    return (char *)cv->data + n*cv->elemsz;
}

/* Function: cvec_create
 * ---------------------
 * Purpose: Allocates a vector in the heap and initializes fields.
 * Parameters: size of each vector element, capacity hint, cleanup callback function
 * Return values: pointer to CVector
 */
CVector *cvec_create(size_t elemsz, size_t capacity_hint, CleanupElemFn fn) {
    // allocates CVector in heap
    CVector *cv = malloc(sizeof(CVector));
    
    // assert if elemsz is 0 and set struct field
    assert(elemsz != 0); // error message if false
    cv->elemsz = elemsz; // arrow notation does dot operation and dereferencing
    
    // check if capacity hint is 0 and handle with internal default
    if(capacity_hint == 0) capacity_hint = DEFAULT_CAPACITY;
    
    cv->size = 0; // no data yet
    cv->clean = fn;
    cv->data = calloc(capacity_hint, elemsz);
    cv->capacity = capacity_hint;
    
    // assert if allocation fails
    assert(cv->data != NULL);
    return cv;
}

/* Function: cvec_dispose
 * ----------------------
 * Purpose: Frees memory allocated in heap.
 * Parameters: pointer to CVector
 * Return values: void
 */
void cvec_dispose(CVector *cv) {
    if(cv->clean != NULL) {
        // iterates though CVector and calls cleanup function for each element
        for(int i = 0; i < cv->size; i++) {
            void *ptr = get_nth(cv, i);
            cv->clean(ptr);
            ptr = NULL;
        }
    }
    free(cv->data);
    // frees memory used for CVector storage
    free(cv);
}

/* Function: cvec_count
 * --------------------
 * Purpose: Gets number of elements in CVector
 * Paramaters: pointer to CVector
 * Return values: int count
 */
int cvec_count(const CVector *cv) { 
    return cv->size;
}

/* Function: cvec_nth
 * ------------------
 * Purpose: Performs pointer arithmetic to get nth index element in vector
 * Parameters: pointer to CVector and index of interest
 * Return values: pointer to nth index
 */
void *cvec_nth(const CVector *cv, int index) {
    // index out of bounds check
    assert(index >= 0 && index <= (cv->size - 1));
    return get_nth(cv, index);
}

/* Function: cvec_expand
 * ---------------------
 * Purpose: Double the capacity of CVector once filled.
 * Parameters: pointer to CVector
 * Return values: void
 */ 
void cvec_expand(CVector *cv) {
    // double the capacity
    cv->capacity = cv->capacity * 2;
    cv->data = realloc(cv->data, cv->elemsz * cv->capacity);
    // assert if allocation fails
    assert(cv->data != NULL);
}

/* Function: cvec_insert
 * ---------------------
 * Purpose: Inserts a passed value into a given index in the vector
 * Parameters: pointer to CVector, address of element to insert, index to insert at
 * Return values: void
 */
void cvec_insert(CVector *cv, const void *addr, int index) { 
    // index out of bounds check
    assert(index >= 0 && index <= cv->size);

    // check if capacity needs to be enlarged
    // keep buffer of 1, otherwise first arg of memmove might segfault on end insert
    if(cv->size == cv->capacity) {
        cvec_expand(cv);
    }

    // memmove used since src and dest can overlap (unlike with memcpy)
    // usage: memmove(dest addr, src addr, number of bytes to be moved)i
    memmove(get_nth(cv, index + 1), get_nth(cv, index), 
            (cv->elemsz) * (cv->size - index));

    // insert
    memcpy(get_nth(cv, index), addr, cv->elemsz);

    // increment cv->size
    (cv->size)++;
}

/* Function: cvec_append
 * ---------------------
 * Purpose: Appends a passed value to the end of the vector
 * Parameters: pointer to CVector and address of element to apped
 * Return values: void
 */
void cvec_append(CVector *cv, const void *addr) {
    // appending is the same as inserting at end
    cvec_insert(cv, addr, cvec_count(cv));
}

// implementation not required
void cvec_replace(CVector *cv, const void *addr, int index) {}

// implementation not required
void cvec_remove(CVector *cv, int index) {}

/* Function: cvec_search
 * ---------------------
 * Purpose: Search for an element of interest in a portion of the vector
 * Parameters: pointer to CVector, address of search key, 
 * compare callback function, start index, boolean indicating whether vector is sorted
 * Return values: index of matching element 
 */
int cvec_search(const CVector *cv, const void *key, CompareFn cmp, int start, bool sorted) { 
    // start index out of bounds check
    assert(start >= 0 && start <= cv->size);

    size_t num_searchelems = cvec_count(cv) - start;

    if(sorted) {
        // binary search
        char *ptr1 = (char *)bsearch(key, get_nth(cv, start), num_searchelems, cv->elemsz, cmp);
        if(ptr1 == NULL) return -1;
        return(ptr1 - (char *)(cv->data))/(cv->elemsz);
    } else {
        // linear search
        // third arg passed by reference
        char *ptr2 = (char *)lfind(key, get_nth(cv, start), &num_searchelems, cv->elemsz, cmp);
        if(ptr2 == NULL) return -1;
        return (ptr2 - (char *)(cv->data))/(cv->elemsz);
    }
    
}

/* Function: cvec_sort
 * -------------------
 * Purpose: Runs quicksort over the vector
 * Parameters: pointer to CVector, callback compare function
 * Return values: void
 */
void cvec_sort(CVector *cv, CompareFn cmp) { 
    qsort(cv->data, cvec_count(cv), cv->elemsz, cmp);
}

/* Function: cvec_first
 * --------------------
 * Purpose: Gets first element in vector
 * Parameters: pointer to CVector
 * Return values: pointer to first element
 */
void *cvec_first(const CVector *cv) { 
    // empty CVector
    if(cv->size == 0) return NULL; 
    else return cv->data;
}

/* Function: cvec_next
 * -------------------
 * Purpose: Gets next element in vector given a previous one.
 * Parameters: pointer to CVector, pointer to previous element
 * Return values: pointer to next element
 */
void *cvec_next(const CVector *cv, const void *prev) {
    // to handle reaching end of populated array
    if(prev == (char *)cv->data + cv->elemsz*(cvec_count(cv) - 1)) return NULL;
    return (char *)prev + cv->elemsz; // works because data is stored in contiguous memory 
}
