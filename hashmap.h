#ifndef _HASHMAP_H
#define _HASHMAP_H
typedef struct Mapping {
    char key[256];
    char value[256];
    void* next;
    void (*destruct)(void*);
} Mapping;


typedef struct Bucket {
    Mapping* mappings;
    int size;
    int index;
    void (*destruct)(void*);
} Bucket;


typedef struct HashMap {
    int slots;
    int largest_bucket_size;
    int max_items;
    int max_bucket_size;
    int items_count;
    int verbose;
    int debug;
    Bucket** buckets;
    void (*init)(void*, int, int, int, int, int);
    unsigned int (*hashvalue)(void*, char*);
    void (*print)(void*);
    void (*put)(void*, char*, char*, int);
    void (*delete)(void*, char*);
    void (*expand)(void*);
    void (*shrink)(void*);
    void (*destruct)(void*);
} HashMap;
#endif
