#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include "arguments_parser.h"
#include "jenkins_hash.h"
#include "hashmap.h"


void destruct_mapping(void* mapping) {
    ((Mapping*) mapping)->next = NULL;
    free(mapping);
}


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
    int empty_slots = 0;
    while (i < slots) {
        Mapping* current_mapping = (*current)->mappings;
        if (!current_mapping) {
            i ++;
            current ++;
            empty_slots ++;
            continue;
        }
        if (((HashMap*) hashmap)->debug) {
            printf("%d -> ", (*current)->index);
        }
        while (current_mapping) {
            if (((HashMap*) hashmap)->debug) {
                printf("(%s, %s), ", current_mapping->key, current_mapping->value);
            }
            current_mapping = current_mapping->next;
            items ++;
        }
        if (((HashMap*) hashmap)->debug) {
            printf("\n");
        }
        i ++;
        current ++;
    }
    if (((HashMap*) hashmap)->verbose || ((HashMap*) hashmap)->debug) {
        printf("Total items: %d\n", items);
        printf("Slot vacancy: %f%\n", (((float) empty_slots)/(float) slots) * 100);
        printf("Slot ocupancy: %f%\n", (((float) (slots - empty_slots))/(float) slots) * 100);
    }
}


void double_hashmap_size(void*);
void shrink_hashmap_size(void*);


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
        if (((HashMap*) hashmap)->largest_bucket_size > ((HashMap*) hashmap)->max_bucket_size && should_resize) {
            ((HashMap*) hashmap)->expand(hashmap);
        }
    }
}


void delete_key_from_hashmap(void* hashmap, char* key) {
    unsigned int slot_value = ((HashMap*) hashmap)->hashvalue(hashmap, key);
    Bucket* bucket = ((HashMap*) hashmap)->buckets[slot_value];
    if (!bucket->mappings) {
        printf("Couldn't find key");
        return; 
    } else {
        Mapping* current = bucket->mappings;
        Mapping* prev = NULL;
        while (current) {
            if (strcmp(current->key, key) == 0) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    bucket->mappings = current->next;
                }
                free(current);
                break;
            }
            prev = current;
            current = current->next;
        }
    }
    ((HashMap*) hashmap)->items_count --;
    int item_count = ((HashMap*) hashmap)->items_count;
    int max_bucket_size = ((HashMap*) hashmap)->max_bucket_size;
    int slots = ((HashMap*) hashmap)->slots;
    int max_capacity = max_bucket_size * slots;
    if (item_count < (int)((float) max_capacity/(float) 4)) {
        if (((HashMap*) hashmap)->verbose) {
            printf("Shrinking table\n");
            printf("slots: %d\n", slots);
        }
        ((HashMap*) hashmap)->shrink(hashmap);
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


void init_hashmap(
        void* hashmap,
        int initial_slots,
        int max_items,
        int max_bucket_size,
        int verbose,
        int debug
) {
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
    ((HashMap*) hashmap)->shrink = shrink_hashmap_size;
    ((HashMap*) hashmap)->destruct = destruct_hash_map;
    ((HashMap*) hashmap)->largest_bucket_size = 0;
    ((HashMap*) hashmap)->max_items = max_items;
    ((HashMap*) hashmap)->items_count = 0;
    ((HashMap*) hashmap)->max_bucket_size = max_bucket_size;
    ((HashMap*) hashmap)->verbose = verbose;
    ((HashMap*) hashmap)->debug = debug;
}


void double_hashmap_size(void* hashmap) {
    HashMap* new_hashmap = (HashMap*) malloc(sizeof(HashMap));
    new_hashmap->init = init_hashmap;
    new_hashmap->init(
        new_hashmap,
        ((HashMap*) hashmap)->slots * 2,
        ((HashMap*) hashmap)->max_items,
        ((HashMap*) hashmap)->max_bucket_size,
        ((HashMap*) hashmap)->verbose,
        ((HashMap*) hashmap)->debug
    );

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


void shrink_hashmap_size(void* hashmap) {
    HashMap* new_hashmap = (HashMap*) malloc(sizeof(HashMap));
    new_hashmap->init = init_hashmap;
    new_hashmap->init(
        new_hashmap,
        ((HashMap*) hashmap)->slots / 2,
        ((HashMap*) hashmap)->max_items,
        ((HashMap*) hashmap)->max_bucket_size,
        ((HashMap*) hashmap)->verbose,
        ((HashMap*) hashmap)->debug
    );

    Bucket** current = ((HashMap*) hashmap)->buckets;
    int slots = ((HashMap*) hashmap)->slots;
    int i = 0;
    // Prevent table expansions during contractions for smooth
    // deletions.
    int should_resize = 0;
    while (i < slots) {
        Mapping* current_mapping = (*current)->mappings;
        while (current_mapping) {
            new_hashmap->put(new_hashmap, current_mapping->key, current_mapping->value, should_resize);
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


void operate(HashMap* hashmap, int max_items) {
    for (int i = 0; i < max_items; i ++) {
        int length = snprintf(NULL, 0, "%d", i);
        char* str = malloc( length + 1 );
        snprintf( str, length + 1, "%d", i);
        hashmap->put(hashmap, str, "a", 1);
        free(str);
    }
    for (int i = 0; i < max_items / 2; i ++) {
        int length = snprintf(NULL, 0, "%d", i);
        char* str = malloc( length + 1 );
        snprintf( str, length + 1, "%d", i);
        hashmap->delete(hashmap, str);
        free(str);
    }

}

int main(int argc, char *argv[]) {
    struct arguments _arguments = parse_arguments(argc, argv);
    HashMap* hashmap = (HashMap*) malloc(sizeof(HashMap));
    hashmap->init = init_hashmap;
    hashmap->init(
        hashmap,
        2,
        _arguments.max_items,
        _arguments.max_slots,
        _arguments.verbose,
        _arguments.debug
    );
    operate(hashmap, hashmap->max_items);
    hashmap->print(hashmap);
    hashmap->destruct(hashmap);

    return 0;
}
