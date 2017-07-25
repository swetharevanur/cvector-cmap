/*
 * File: cmap.c
 * Author: SWETHA REVANUR
 * ----------------------
 * Implementation of dynamically-allocated hashmaps in C.
 * Uses linked list structure.
 */

#include "cmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

// a suggested value to use when given capacity_hint is 0
#define DEFAULT_CAPACITY 1023

/* Type: struct CMapImplementation
 * -------------------------------
 * This definition completes the CMap type that was declared in
 * cmap.h. You fill in the struct with your chosen fields.
 */
typedef struct CMapImplementation {
    void **buckets; 
    size_t nbuckets;
    size_t valsz;
    int count;
    CleanupValueFn clean;
} CMap;


/* Function: hash
 * --------------
 * This function adapted from Eric Roberts' _The Art and Science of C_
 * It takes a string and uses it to derive a "hash code," which
 * is an integer in the range [0-nbuckets-1]. The hash code is computed
 * using a method called "linear congruence." A similar function using this
 * method is described on page 144 of Kernighan and Ritchie. The choice of
 * the value for the multiplier can have a significant effort on the
 * performance of the algorithm, but not on its correctness.
 * The computed hash value is stable, e.g. passing the same string and
 * nbuckets to function again will always return the same code.
 * The hash is case-sensitive.
 */
static int hash(const char *s, int nbuckets) {
   const unsigned long MULTIPLIER = 2630849305L; // magic number
   unsigned long hashcode = 0;
   for (int i = 0; s[i] != '\0'; i++)
      hashcode = hashcode * MULTIPLIER + s[i];
   return hashcode % nbuckets;
}

/* Function: get_next
 * ------------------
 * Purpose: Gets pointer to next blob.
 * Parameters: pointer to current blob.
 * Return values: pointer to next blob.
 */
void *get_next(const void *blob) {
    return *(void **)blob;
}

/* Function: set_next
 * ------------------
 * Purpose: Sets pointer to next blob
 * Parameters: pointer to current blob, address of next blob
 * Return values: void
 */
void set_next(void *blob, void *ptr_to_next) {
    *(void **)blob = ptr_to_next;
}

/* Function: get_key
 * -----------------
 * Purpose: Gets pointer to key in blob
 * Parameters: pointer to current blob
 * Return values: char * where key is stored
 */
char *get_key(void *blob) {
    // returns pointer to key
    return (char *)blob + sizeof(void *);
}

/* Function: set_key
 * ------------------
 * Purpose: Sets pointer to key in blob
 * Parameters: pointer to current blob, char * key to add
 * Return values: void
 */
void set_key(void *blob, const char *key) {
    // memcpy(get_key(blob), key, sizeof(key));
    strcpy(get_key(blob), key);
}

/* Function: get_value
 * -------------------
 * Purpose: Gets pointer to value field in blob
 * Parameters: pointer to current blob
 * Return values: pointer to value field in blob
 */
void *get_value(void *blob) {
    return (char *)blob + sizeof(void *) + strlen(get_key(blob)) + 1; 
}

/* Function: set_value
 * -------------------
 * Purpose: Sets value field in blob
 * Parameters: pointer to CMap, pointer to current blob, address of value to add
 * Return values: void
 */
void set_value(CMap *cm, void *blob, const void *value) {
    memcpy(get_value(blob), value, cm->valsz); 
}

/* Function: create_blob
 * ---------------------
 * Purpose: Creates a blob with a next pointer, key, and value
 * Parameters: pointer to CMap, address of key and value
 * Return values: pointer to blob
 */
void *create_blob(CMap *cm, const char *key, const void *value) {
    // ptr_to_next will always be NULL
    void *blob = malloc(sizeof(void *) + strlen(key) + 1 + cm->valsz); // +1 for null term
    // assert if allocation fails
    assert(blob != NULL);

    set_next(blob, NULL);
    set_key(blob, key);
    set_value(cm, blob, value);
    return blob;
}

/* Function: cmap_create
 * ---------------------
 * Purpose: Allocates memory for a map and initializes fields.
 * Parameters: size of map values, capacity, cleanup callback function
 * Return values: pointer to CMap
 */
CMap *cmap_create(size_t valuesz, size_t capacity_hint, CleanupValueFn fn) { 
    CMap *cm = malloc(sizeof(CMap));

    // assert if valuesz is 0
    assert(valuesz != 0);
    cm->valsz = valuesz;

    // check if capacity hint is 0 and handle with internal default
    if(capacity_hint == 0) capacity_hint = DEFAULT_CAPACITY;
    cm->nbuckets = capacity_hint;

    cm->count = 0;

    cm->clean = fn;
    // calloc because that everything is automatically NULL'd out (as in spec drawings)
    // you only know if you have a non-empty bucket if it's NULL
    cm->buckets = calloc(capacity_hint, sizeof(void *));

    // assert if allocation fails
    assert(cm->buckets != NULL);
    return cm;
}

/* Function: cmap_dispose
 * ----------------------
 * Purpose: Cleans up values and frees buckets and map.
 * Parameters: pointer to CMap
 * Return values: void
 */
void cmap_dispose(CMap *cm) { 
    // for each bucket
    for(int i = 0; i < cm->nbuckets; i++) {
        // traverse linked list and clean
        while(cm->buckets[i] != NULL) {
            void *blob = cm->buckets[i];
            cm->buckets[i] = get_next(blob);
            // call cleanup function on values
            if(cm->clean != NULL) {
                cm->clean(get_value(blob));
            }
            free(blob);
        }
    }
  
    free(cm->buckets);
    free(cm);
}

/* Function: cmap_count
 * --------------------
 * Purpose: Counts total number of keys in map
 * Parameters: pointer to CMap
 * Return values: total number of keys in map
 */
int cmap_count(const CMap *cm) { 
    // returns total number of keys
    return cm->count;
}

/* Function: cmap_put
 * ------------------
 * Purpose: Adds key to map. If key already exists, value is cleaned and updated.
 * Parameters: pointer to CMap, key to add, address of value
 * Return values: void
 */
void cmap_put(CMap *cm, const char *key, const void *addr) { 
    // hash the key to get bucket number
    int bucket_num = hash(key, cm->nbuckets);
    
    // loop through linked list to check if key already exists
    // starting point is pointer to first blob
    void *temp = cm->buckets[bucket_num];
    while(temp != NULL) {
        // check if key already exists
        if(strcmp(get_key(temp), key) == 0) {
            if(cm->clean != NULL) {
                // call cleanup function on old value
                cm->clean(get_value(temp));
            }
            // replace without incrementing count
            set_value(cm, temp, addr); 
            return;
        }
        temp = get_next(temp);
    }
    
    // if key is new create blob
    void *blob = create_blob(cm, key, addr);
    
    // add to front of linked list (instead of back for Big-O)
    void *start = cm->buckets[bucket_num];
    
    // if not first blob in bucket
    if(start != NULL) set_next(blob, start);
    cm->buckets[bucket_num] = blob;
    
    // update count
    (cm->count)++;
}

/* Function: cmap_get
 * ------------------
 * Purpose: Loops through linked list to find key in map
 * Parameters: pointer to CMap, key of interest
 * Return values: pointer to key of interest
 */
void *cmap_get(const CMap *cm, const char *key) { 
    int bucket_num = hash(key, cm->nbuckets);

    // loop through linked list to find key
    void *temp = cm->buckets[bucket_num];
    while(temp != NULL) {
        if(strncmp(get_key(temp), key, strlen(key)) == 0) {
            return get_value(temp);
        }
        temp = get_next(temp);
    }
    
    // if not found
    return NULL;
}

/* Function: cmap_first
 * --------------------
 * Purpose: Gets first non-NULL key in map. Jumps buckets if needed.
 * Parameters: pointer to CMap
 * Return values: first key
 */
const char *cmap_first(const CMap *cm) { 
    for(int i = 0; i < cm->nbuckets; i++) {
        void *bucket = cm->buckets[i];
        if(bucket != NULL) return get_key(bucket);
    }
    // nothing found
    return NULL;
}

/* Function: cmap_next
 * -------------------
 * Purpose: Gets next key in map given previous key. Jumps buckets if needed.
 * Parameters: pointer to CMap and previous key
 * Return values: next valid key
 */
const char *cmap_next(const CMap *cm, const char *prevkey) { 
    // if there's another blob in the bucket
    void *blob = (char *)prevkey - sizeof(void *);
    if(get_next(blob) != NULL) { 
        return get_key(get_next(blob));
    }

    int start_bucket = hash(prevkey, cm->nbuckets);

    // to jump buckets
    for(int i = start_bucket + 1; i < cm->nbuckets; i++) {
        if(cm->buckets[i] == NULL) { 
            continue;
        }
        return get_key(cm->buckets[i]);
    }
    
    return NULL; 
}
