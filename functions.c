#include <stdio.h>
#include <stdlib.h>

struct prefix {
    unsigned int ip;
    unsigned char len;
    struct prefix *next;
};

struct bucket {
    struct prefix *head;
};

struct hashtable {
    struct bucket buckets[256];
    int size;
};

void segment(struct prefix *head, struct hashtable *ht) {
    for(int i = 0; i < 256; i++) {
        ht->buckets[i].head = NULL;
    }
    
    struct prefix *current = head;
    int count = 0;
    while(current != NULL) {
        unsigned char index = current->ip >> 24;
        struct prefix *next = current->next;
        
        struct prefix **insert_pos = &(ht->buckets[index].head);
        while (*insert_pos != NULL && (*insert_pos)->ip < current->ip) {
            insert_pos = &((*insert_pos)->next);
        }
        
        // Insert the node
        current->next = *insert_pos;
        *insert_pos = current; // 
        count++;
        current = next;
    }
}

void prefix_insert(struct hashtable *ht, struct prefix *node_head) {
    static int insert_count = 0;  // counter for inserts
    insert_count++;
    
    unsigned char index = node_head->ip >> 24;
    struct prefix **current = &(ht->buckets[index].head);
    while(*current != NULL && (*current)->ip < node_head->ip) {
        current = &((*current)->next);
    }
    
    node_head->next = *current;
    *current = node_head;
    
}

void prefix_delete(struct hashtable *ht, unsigned int ip) {
    
    unsigned char index = ip >> 24;
    struct prefix **current = &(ht->buckets[index].head);
    
    while(*current != NULL) {
        if((*current)->ip == ip) {
            struct prefix *to_delete = *current;
            *current = (*current)->next;
            free(to_delete);
            return;
        }
        current = &((*current)->next);
    }
}

int search(struct hashtable *ht, unsigned int ip) {
    
    unsigned char index = ip >> 24;
    struct prefix *current = ht->buckets[index].head;
    
    while(current != NULL) {
        if(current->ip == ip) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}