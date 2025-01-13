// Discription @ https://hackmd.io/@rkhuncle/PD1-hw10
#include <stdio.h>
#include <stdlib.h>
#define MAX_CLOCK_CYCLES 10000
#define COMPRESS_FACTOR 100
#define TIME_OF_EXAMINATIONS 1000000

struct prefix {
    unsigned int ip;
    unsigned char len;
    struct prefix *next;
};
struct prefix *input(const char *filename, int *total_prefixes);

unsigned long long int insert_cycles_freq[MAX_CLOCK_CYCLES] = {0};
unsigned long long int delete_cycles_freq[MAX_CLOCK_CYCLES] = {0};
unsigned long long int search_cycles_freq[MAX_CLOCK_CYCLES] = {0};


struct bucket {
    struct prefix *head;
};

struct hashtable {
    struct bucket buckets[256];
    int size;
};


static inline unsigned long long int rdtsc_64bits();
void segment(struct prefix *head, struct hashtable *ht);
void prefix_insert(struct hashtable *ht, struct prefix *p);
void prefix_delete(struct hashtable *ht, unsigned int ip);
int search(struct hashtable *ht, unsigned int ip);


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

static inline unsigned long long int rdtsc_64bits()//64-bit
{
   unsigned long long int x;
   unsigned a, d;

   __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

   return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}

/* The command line arguments have the following format:
./
pA 
directory_path/routing_table.txt 
directory_path/inserted_prefixes.txt 
directory_path/delete_prefixes.txt 
directory_path/trace_file.txt 
*/

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <routing_table> <insert_prefixes> <delete_prefixes> <trace_file>\n", argv[0]);
        return 1;
    }
    for(int i = 1; i <= TIME_OF_EXAMINATIONS; i++){
        printf("\n=== Examination %d/%d ===\n", i, TIME_OF_EXAMINATIONS);
        int total_prefixes = 0;
        struct prefix *routing_table = input(argv[1], &total_prefixes);
        struct hashtable ht = {0};
        segment(routing_table, &ht);
        int insert_count = 0;
        struct prefix *insert_list = input(argv[2], &total_prefixes);
        struct prefix *current = insert_list;
        
        printf("Processing insertions... (%d/%d)\n", i, TIME_OF_EXAMINATIONS);
        int loop_count = 0;
        struct prefix *prev = NULL;
        while(current != NULL) {
            loop_count++;
            struct prefix *next = current->next;
            
            unsigned long long begin = rdtsc_64bits();
            prefix_insert(&ht, current);
            unsigned long long end = rdtsc_64bits();
            
            if ((end - begin) < MAX_CLOCK_CYCLES) {
                insert_cycles_freq[end - begin]++;
            }
            
            prev = current;
            current = next;
            
        }
        struct prefix *delete_list = input(argv[3], &total_prefixes);
        current = delete_list;
        
        printf("Processing deletions... (%d/%d)\n", i, TIME_OF_EXAMINATIONS);
        while(current != NULL) {
            unsigned long long begin = rdtsc_64bits();
            prefix_delete(&ht, current->ip);
            unsigned long long end = rdtsc_64bits();
            if ((end - begin) < MAX_CLOCK_CYCLES) {
                delete_cycles_freq[end - begin]++;
            }
            current = current->next;
        }

        struct prefix *search_list = input(argv[4], &total_prefixes);
        current = search_list;
        
        printf("Processing searches... (%d/%d)\n", i, TIME_OF_EXAMINATIONS);
        while(current != NULL) {
            unsigned long long begin = rdtsc_64bits();
            search(&ht, current->ip);
            unsigned long long end = rdtsc_64bits();
            if ((end - begin) < MAX_CLOCK_CYCLES) {
                search_cycles_freq[end - begin]++;
            }
            current = current->next;
            }
        printf("=== Examination %d complete (%d remaining) ===\n", 
               i, TIME_OF_EXAMINATIONS - i);
    }
    
        unsigned long long int compress_delete[MAX_CLOCK_CYCLES/COMPRESS_FACTOR + 1] = {0};
        unsigned long long int compress_insert[MAX_CLOCK_CYCLES/COMPRESS_FACTOR + 1] = {0};
        unsigned long long int compress_search[MAX_CLOCK_CYCLES/COMPRESS_FACTOR + 1] = {0};

        for (int i = 0; i < MAX_CLOCK_CYCLES; i++) {
        compress_delete[i/COMPRESS_FACTOR] += delete_cycles_freq[i];
        compress_insert[i/COMPRESS_FACTOR] += insert_cycles_freq[i];
        compress_search[i/COMPRESS_FACTOR] += search_cycles_freq[i];
        if(i == MAX_CLOCK_CYCLES - 1){
            compress_delete[i/COMPRESS_FACTOR]/=TIME_OF_EXAMINATIONS;
            compress_insert[i/COMPRESS_FACTOR]/=TIME_OF_EXAMINATIONS;
            compress_search[i/COMPRESS_FACTOR]/=TIME_OF_EXAMINATIONS;
        }
        }

        // Write compressed data to files
        FILE *compressed_delete = fopen("compressed_delete.txt", "w");
        FILE *compressed_insert = fopen("compressed_insert.txt", "w");
        FILE *compressed_search = fopen("compressed_search.txt", "w");

        for (int i = 0; i < MAX_CLOCK_CYCLES/COMPRESS_FACTOR; i++) {
        if (compress_delete[i] > 0) {
            fprintf(compressed_delete, "%d\t%llu\n", i*COMPRESS_FACTOR, compress_delete[i]);
        }
        if (compress_insert[i] > 0) {
            fprintf(compressed_insert, "%d\t%llu\n", i*COMPRESS_FACTOR, compress_insert[i]);
        }
        if (compress_search[i] > 0) {
            fprintf(compressed_search, "%d\t%llu\n", i*COMPRESS_FACTOR, compress_search[i]);
        }
        }

        fclose(compressed_delete);
        fclose(compressed_insert);
        fclose(compressed_search);
/*
    printf("\nWriting results to files...\n");
    FILE *result_delete = fopen("delete.txt", "w");
    FILE *result_insert = fopen("insert.txt", "w");
    FILE *result_search = fopen("search.txt", "w");
    for (int i = 0; i < MAX_CLOCK_CYCLES; i++) {
        if (delete_cycles_freq[i] > 0) {
            fprintf(result_delete, "%d\t%llu\n", i, delete_cycles_freq[i]);
        }
        if (insert_cycles_freq[i] > 0) {
            fprintf(result_insert, "%d\t%llu\n", i, insert_cycles_freq[i]);
        }
        if (delete_cycles_freq[i] > 0) {
            fprintf(result_search, "%d\t%llu\n", i, search_cycles_freq[i]);
        }
    }
    fclose(result_delete);
    fclose(result_insert);
    fclose(result_search);
*/
    return 0;
}

struct prefix *input(const char *filename, int *total_prefixes) {
    FILE *file = fopen(filename, "r");
    struct prefix *head = NULL;
    struct prefix *current = NULL;
    *total_prefixes = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        unsigned ip1, ip2, ip3, ip4, len;
        int fields;
        fields = sscanf(line, "%u.%u.%u.%u/%u", &ip1, &ip2, &ip3, &ip4, &len);
        if (fields == 4) {
            if (ip4 == 0) {
                if (ip3 == 0) {
                    if (ip2 == 0) len = 8;     // x.0.0.0
                    else len = 16;              // x.y.0.0
                }
                else len = 24;                  // x.y.z.0
            }
            else len = 32;                      // x.y.z.w
        }
    
        struct prefix *new_prefix = malloc(sizeof(struct prefix));
        new_prefix->ip = (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
        new_prefix->len = len;
        new_prefix->next = NULL;
        
        if (!head) {
            head = new_prefix;
            current = new_prefix;
        } else {
            current->next = new_prefix;
            current = new_prefix;
        }
        (*total_prefixes)++;
    }
    fclose(file);
    return head;
}