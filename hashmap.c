#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jenkins_hash.h"


typedef struct Mapping {
    char key[256];
    char value[256];
    void* next;
    void (*destruct)(void*);
} Mapping;
void destruct_mapping(void* mapping) {
    ((Mapping*) mapping)->next = NULL;
    free(mapping);
}


typedef struct Bucket {
    Mapping* mappings;
    int size;
    int index;
    void (*destruct)(void*);
} Bucket;
void destruct_bucket(void* bucket) {
    Mapping* current = ((Bucket*) bucket)->mappings;
    while (current) {
        Mapping* prev = current;
        current = current->next;
        prev->destruct(prev);
    }
    free(bucket);
}

static Mapping* create_new_mapping(char* key, char* value) {
    Mapping* new_mapping = malloc(sizeof(Mapping));
    strcpy(new_mapping->key, key);
    strcpy(new_mapping->value, value);
    new_mapping->next = NULL;
    new_mapping->destruct = destruct_mapping;

    return (void*) new_mapping;
}


typedef struct HashMap {
    int slots;
    int largest_bucket_size;
    int max_items;
    int items_count;
    Bucket** buckets;
    void (*init)(void*, int);
    unsigned int (*hashvalue)(void*, char*);
    void (*print)(void*);
    void (*put)(void*, char*, char*, int);
    void (*delete)(void*, char*);
    void (*expand)(void*);
    void (*destruct)(void*);
} HashMap;


unsigned int hashvalue(void* hashmap, char* key) {
    int len = strlen(key);
    unsigned int _hashvalue = (unsigned int) (jenkins_hash(key, len) % ((HashMap*) hashmap)->slots);

    return _hashvalue;
}


void print_hashmap_slots(void* hashmap) {
    Bucket** current = ((HashMap*) hashmap)->buckets;
    int slots = ((HashMap*) hashmap)->slots;
    int i = 0;
    int items = 0;
    while (i < slots) {
        Mapping* current_mapping = (*current)->mappings;
        if (!current_mapping) {
            i ++;
            current ++;
            continue;
        }
        printf("%d -> ", (*current)->index);
        while (current_mapping) {
            printf("(%s, %s), ", current_mapping->key, current_mapping->value);
            current_mapping = current_mapping->next;
            items ++;
        }
        printf("\n");
        i ++;
        current ++;
    }
    printf("printed %d items\n", items);
}


void double_hashmap_size(void*);


int COUNT = 0;
void put_value_into_hashmap(void* hashmap, char* key, char* value, int should_resize) {
    unsigned int slot_value = ((HashMap*) hashmap)->hashvalue(hashmap, key);
    Bucket* bucket = ((HashMap*) hashmap)->buckets[slot_value];
    Mapping* new_mapping = create_new_mapping(key, value);
    int bucket_size = 0;
    if (((HashMap*) hashmap)->items_count == ((HashMap*) hashmap)->max_items) {
        printf("hash map too large\n");
        /*((HashMap*) hashmap)->print(hashmap);*/
        exit(1);
    }
    if (!bucket->mappings) {
        bucket->mappings = new_mapping;
        bucket_size += 1;
    } else {
        Mapping* current = bucket->mappings;
        Mapping* prev = NULL;
        while (current) {
            if (strcmp(current->key, key) == 0) {
                strcpy(current->value, value);
                return;
            }
            bucket_size += 1;
            prev = current;
            current = current->next;
        }
        if (prev)
            prev->next = new_mapping;
    }
    ((HashMap*) hashmap)->items_count ++;
    if (((HashMap*) hashmap)->largest_bucket_size < bucket_size) {
        ((HashMap*) hashmap)->largest_bucket_size = bucket_size;
        if (((HashMap*) hashmap)->largest_bucket_size > 5 && should_resize) {
            ((HashMap*) hashmap)->expand(hashmap);
        }
    }
}


void delete_key_from_hashmap(void* hashmap, char* key) {
    unsigned int slot_value = ((HashMap*) hashmap)->hashvalue(hashmap, key);
    Bucket** bucket = ((HashMap*) hashmap)->buckets;
    if ((*bucket)->mappings) {
        // raise error
        return; 
    } else {
        Mapping* current = (*bucket)->mappings;
        Mapping* prev = NULL;
        while (current) {
            if (strcmp(current->key, key) == 0) {
                if (prev) {
                    prev->next = current->next;
                    free(current);
                } else {
                    (*bucket)->mappings = current->next;
                }
                return;
            }
            ((HashMap*) hashmap)->items_count --;
            prev = current;
            current = current->next;
        }
    }
}


void _destruct_buckets(Bucket** buckets, unsigned int slots) {
    for (int i = 0; i < slots; i ++) {
        buckets[i]->destruct(buckets[i]);
    }
    free(buckets);
}


void destruct_hash_map(void* hashmap) {
    Bucket** buckets = ((HashMap*) hashmap)->buckets;
    unsigned int slots = ((HashMap*) hashmap)->slots;
    _destruct_buckets(buckets, slots);
    free(hashmap);
}


void init_hashmap(void* hashmap, int initial_slots) {
    ((HashMap*) hashmap)->slots = initial_slots;
    ((HashMap*) hashmap)->buckets = malloc(sizeof(Bucket*) * initial_slots);
    Bucket** buckets = ((HashMap*) hashmap)->buckets;
    for (int i = 0; i < initial_slots; i ++) {
        buckets[i] = malloc(sizeof(Bucket));
        buckets[i]->index = i;
        buckets[i]->mappings = NULL;
        buckets[i]->destruct = destruct_bucket;
    }
    ((HashMap*) hashmap)->hashvalue = hashvalue;
    ((HashMap*) hashmap)->print = print_hashmap_slots;
    ((HashMap*) hashmap)->put = put_value_into_hashmap;
    ((HashMap*) hashmap)->delete = delete_key_from_hashmap;
    ((HashMap*) hashmap)->expand = double_hashmap_size;
    ((HashMap*) hashmap)->destruct = destruct_hash_map;
    ((HashMap*) hashmap)->largest_bucket_size = 0;
    ((HashMap*) hashmap)->max_items = 500000;
    ((HashMap*) hashmap)->items_count = 0;
}


void double_hashmap_size(void* hashmap) {
    HashMap* new_hashmap = (HashMap*) malloc(sizeof(HashMap));
    new_hashmap->init = init_hashmap;
    new_hashmap->init(new_hashmap, ((HashMap*) hashmap)->slots * 2);

    Bucket** current = ((HashMap*) hashmap)->buckets;
    int slots = ((HashMap*) hashmap)->slots;
    int i = 0;
    while (i < slots) {
        Mapping* current_mapping = (*current)->mappings;
        while (current_mapping) {
            new_hashmap->put(new_hashmap, current_mapping->key, current_mapping->value, 1);
            current_mapping = current_mapping->next;
        }
        i ++;
        current ++;
    }
    _destruct_buckets(((HashMap*) hashmap)->buckets, slots);
    ((HashMap*) hashmap)->buckets = new_hashmap->buckets;
    ((HashMap*) hashmap)->slots = new_hashmap->slots;
    ((HashMap*) hashmap)->largest_bucket_size = new_hashmap->largest_bucket_size;
    free(new_hashmap);
}


void operate(HashMap* hashmap) {
    for (int i = 0; i < 43872; i ++) {
        int length = snprintf(NULL, 0, "%d", i);
        char* str = malloc( length + 1 );
        snprintf( str, length + 1, "%d", i);
        hashmap->put(hashmap, str, "a", 1);
        free(str);
    }
}

int main() {
    HashMap* hashmap = (HashMap*) malloc(sizeof(HashMap));
    hashmap->init = init_hashmap;
    hashmap->init(hashmap, 2);
    operate(hashmap);
/*    hashmap->print(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    hashmap->print(hashmap);
    hashmap->destruct(hashmap);

    return 0;
}
