#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    int i = 0;
    int key_sum = 0;
    while (*key != '\0') {
        key_sum += (int)(*key);
        key ++;
    }
    unsigned int _hashvalue = (unsigned int) (key_sum % ((HashMap*) hashmap)->slots);

    return _hashvalue;
}


void print_hashmap_slots(void* hashmap) {
    Bucket** current = ((HashMap*) hashmap)->buckets;
    int slots = ((HashMap*) hashmap)->slots;
    int i = 0;
    while (i < slots) {
        printf("%d -> ", (*current)->index);
        Mapping* current_mapping = (*current)->mappings;
        while (current_mapping) {
            printf("(%s, %s), ", current_mapping->key, current_mapping->value);
            current_mapping = current_mapping->next;
        }
        printf("\n");
        i ++;
        current ++;
    }
}


void double_hashmap_size(void*);


int COUNT = 0;
void put_value_into_hashmap(void* hashmap, char* key, char* value, int should_resize) {
    unsigned int slot_value = ((HashMap*) hashmap)->hashvalue(hashmap, key);
    Bucket* bucket = ((HashMap*) hashmap)->buckets[slot_value];
    Mapping* new_mapping = create_new_mapping(key, value);
    int bucket_size = 0;
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
    if (((HashMap*) hashmap)->largest_bucket_size < bucket_size) {
        ((HashMap*) hashmap)->largest_bucket_size = bucket_size;
        if (((HashMap*) hashmap)->largest_bucket_size > 2 && should_resize) {
            printf("%d\n", ((HashMap*) hashmap)->largest_bucket_size);
            /*((HashMap*) hashmap)->expand(hashmap);*/
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
            prev = current;
            current = current->next;
        }
    }
}


void destruct_hash_map(void* hashmap) {
    Bucket** buckets = ((HashMap*) hashmap)->buckets;
    unsigned int slots = ((HashMap*) hashmap)->slots;
    for (int i = 0; i < slots; i ++) {
        buckets[i]->destruct(buckets[i]);
    }
    free(buckets);
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
}


void double_hashmap_size(void* hashmap) {
    HashMap* new_hashmap = (HashMap*) malloc(sizeof(HashMap));
    new_hashmap->init = init_hashmap;
    new_hashmap->init(new_hashmap, ((HashMap*) hashmap)->slots * 2);

    Bucket** current = ((HashMap*) hashmap)->buckets;
    int slots = ((HashMap*) hashmap)->slots;
    int i = 0;
    printf("here: %d\n", new_hashmap->slots);
    printf("here: %d\n", slots);
    if(COUNT ++ > 10) {
        exit(1);
    }
    while (i < slots) {
        Mapping* current_mapping = (*current)->mappings;
        int count = 0;
        while (current_mapping) {
            new_hashmap->put(new_hashmap, current_mapping->key, current_mapping->value, 0);
            current_mapping = current_mapping->next;
            if (count ++ > 10000) {
                /*exit(1);*/
            }
        }
        i ++;
        (*current)++;
    }

    ((HashMap*) hashmap)->buckets = new_hashmap->buckets;
    ((HashMap*) hashmap)->slots = new_hashmap->slots;
    ((HashMap*) hashmap)->largest_bucket_size = new_hashmap->largest_bucket_size;
}


void operate(HashMap* hashmap) {
    for (int i = 0; i < 10; i ++) {
        int length = snprintf(NULL, 0, "%d", i);
        char* str = malloc( length + 1 );
        snprintf( str, length + 1, "%d", i);
        hashmap->put(hashmap, str, "a", 1);
        free(str);
    }
/*    hashmap->put(hashmap, "1", "a");*/
    /*hashmap->put(hashmap, "2", "b");*/
    /*hashmap->put(hashmap, "3", "c");*/
    /*hashmap->put(hashmap, "4", "d");*/
    /*hashmap->put(hashmap, "5", "e");*/
    /*hashmap->put(hashmap, "6", "e");*/
    /*hashmap->put(hashmap, "7", "e");*/
    /*hashmap->put(hashmap, "8", "e");*/
    /*hashmap->put(hashmap, "9", "e");*/
    /*hashmap->put(hashmap, "1", "f");*/
    /*hashmap->put(hashmap, "10", "g");*/
    /*hashmap->put(hashmap, "11", "h");*/
    /*hashmap->delete(hashmap, "5");*/
    /*hashmap->delete(hashmap, "5");*/
    /*hashmap->delete(hashmap, "10");*/
}

int main() {
    HashMap* hashmap = (HashMap*) malloc(sizeof(HashMap));
    hashmap->init = init_hashmap;
    hashmap->init(hashmap, 3);
    operate(hashmap);
    /*hashmap->print(hashmap);*/
/*    hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    /*hashmap->expand(hashmap);*/
    hashmap->print(hashmap);
    hashmap->destruct(hashmap);

    return 0;
}