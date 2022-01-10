#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define HASHMAP_SIZE 100

struct value
{
    char* val;
    char data_type;
    int size;
    long long expiry_timestamp;
};

struct map_entry
{
    char *key;
    struct value *val;
    struct map_entry *next;
};

int generate_hash(char *key)
{
    int hash = 0;
    int i = 0;
    while (key[i] != '\0')
    {
        hash += key[i++];
        hash %= HASHMAP_SIZE;
    }
    return hash;
}

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

void insert(struct map_entry **hash_map, char *key, struct value *val)
{
    int index = generate_hash(key);
    struct map_entry *current_entry;
    current_entry = (struct map_entry*) malloc(sizeof(struct map_entry));
    current_entry->key = key;
    current_entry->val = val;
    if (hash_map[index] == NULL)
    {
        hash_map[index] = current_entry;
        return;
    }

    struct map_entry *temp = hash_map[index];
    while (1)
    {
        if (strcmp(temp->key, key) == 0)
        {
            temp->val = val;
            return;
        }
        if (temp->next == NULL)
            break;
        temp = temp->next;
    }
    temp->next = current_entry;
}

int remove_entry(struct map_entry **hash_map, char *key)
{
    int index = generate_hash(key);
    struct map_entry *temp = hash_map[index];
    if (temp == NULL)
        return 0;
    if (strcmp(temp->key, key) == 0)
    {
        free(temp);
        hash_map[index] = NULL;
        return 1;
    }
    while (temp->next != NULL)
    {
        if (strcmp(temp->next->key, key) == 0)
        {
            struct map_entry *next = temp->next;
            temp->next = temp->next->next;
            free(next);
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

struct value* fetch(struct map_entry** hash_map, char* key){
    int index = generate_hash(key);
    struct map_entry *temp = hash_map[index];
    while(temp != NULL)
    {
        if(strcmp(temp->key, key)==0)
        {
            printf("%lld  %lld\n", temp->val->expiry_timestamp, current_timestamp());
            if(temp->val->expiry_timestamp != NULL && temp->val->expiry_timestamp < current_timestamp()){
                printf("-> %lld", temp->val->expiry_timestamp);
                remove_entry(hash_map, key);
                return NULL;
            }
            return temp->val;
        }
        temp = temp->next;
    }
}

void display(struct map_entry **hash_map)
{
    printf("{ ");
    for (int i = 0; i < HASHMAP_SIZE; i++)
    {
        struct map_entry *temp = hash_map[i];
        while (1)
        {
            if (temp == NULL)
            {
                break;
            }
            printf("%s : \"%s\", ", temp->key, temp->val->val);
            temp = temp->next;
        }
    }
    printf("}");
}

struct map_entry** create_hash_map(){
    struct map_entry **hash_map;
    hash_map = (struct map_entry**) malloc(HASHMAP_SIZE*sizeof(struct map_entry*));
    return hash_map;
}