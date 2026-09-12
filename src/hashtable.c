#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TABLE_SIZE 101

HashTable *hash_table_create(size_t size) {
    if (size == 0) {
        size = DEFAULT_TABLE_SIZE;
    }

    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    if (table == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for HashTable structure.\n");
        return NULL;
    }

    table->size = size;
    table->unique_count = 0;
    table->buckets = (HashEntry **)calloc(size, sizeof(HashEntry *));
    if (table->buckets == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for HashTable buckets.\n");
        free(table);
        return NULL;
    }

    return table;
}

size_t hash_table_hash(const char *word, size_t size) {
    if (word == NULL || size == 0) {
        return 0;
    }

    /* djb2 string hashing algorithm */
    size_t hash = 5381;
    while (*word) {
        unsigned char c = (unsigned char)*word++;
        hash = ((hash << 5) + hash) + (size_t)c; /* hash * 33 + c */
    }

    return hash % size;
}

HashEntry *hash_table_search(const HashTable *table, const char *word, size_t *out_index) {
    if (table == NULL || table->buckets == NULL || word == NULL) {
        return NULL;
    }

    size_t index = hash_table_hash(word, table->size);
    HashEntry *curr = table->buckets[index];

    while (curr != NULL) {
        if (strcmp(curr->word, word) == 0) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

int hash_table_insert(HashTable *table, const char *word, int frequency) {
    if (table == NULL || table->buckets == NULL || word == NULL) {
        return 0;
    }

    size_t index = 0;
    HashEntry *existing = hash_table_search(table, word, &index);
    if (existing != NULL) {
        existing->frequency = frequency;
        return 1;
    }

    /* Allocate new node */
    HashEntry *new_entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (new_entry == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for HashEntry.\n");
        return 0;
    }

    size_t word_len = strlen(word);
    new_entry->word = (char *)malloc(word_len + 1);
    if (new_entry->word == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for word in HashEntry.\n");
        free(new_entry);
        return 0;
    }
    memcpy(new_entry->word, word, word_len + 1);

    new_entry->frequency = frequency;

    /* Insert at head of bucket chain */
    index = hash_table_hash(word, table->size);
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
    table->unique_count++;

    return 1;
}

int hash_table_increment(HashTable *table, const char *word) {
    if (table == NULL || word == NULL) {
        return 0;
    }

    HashEntry *existing = hash_table_search(table, word, NULL);
    if (existing != NULL) {
        existing->frequency++;
        return 1;
    }

    return hash_table_insert(table, word, 1);
}

HashTable *hash_table_build_from_tokens(const TokenArray *tokens, size_t table_size) {
    if (tokens == NULL) {
        return NULL;
    }

    if (table_size == 0) {
        /* Reasonable prime bucket size based on token count */
        table_size = (tokens->count > 0) ? (tokens->count * 2 + 1) : DEFAULT_TABLE_SIZE;
        if (table_size < DEFAULT_TABLE_SIZE) {
            table_size = DEFAULT_TABLE_SIZE;
        }
    }

    HashTable *table = hash_table_create(table_size);
    if (table == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < tokens->count; i++) {
        if (tokens->items[i].word != NULL) {
            if (!hash_table_increment(table, tokens->items[i].word)) {
                fprintf(stderr, "Warning: Failed to insert token '%s' into hash table.\n", tokens->items[i].word);
            }
        }
    }

    return table;
}

void hash_table_display(const HashTable *table) {
    if (table == NULL || table->buckets == NULL) {
        return;
    }

    for (size_t i = 0; i < table->size; i++) {
        HashEntry *curr = table->buckets[i];
        while (curr != NULL) {
            printf("%s -> %d\n", curr->word, curr->frequency);
            curr = curr->next;
        }
    }
}

void hash_table_get_top_frequent(const HashTable *table, const HashEntry **top_entries, size_t k, size_t *out_count) {
    if (out_count != NULL) {
        *out_count = 0;
    }
    if (table == NULL || top_entries == NULL || k == 0 || table->unique_count == 0) {
        return;
    }

    /* Collect pointers to all unique entries */
    const HashEntry **all_entries = (const HashEntry **)malloc(table->unique_count * sizeof(const HashEntry *));
    if (all_entries == NULL) {
        fprintf(stderr, "Error: Memory allocation failed during top words extraction.\n");
        return;
    }

    size_t count = 0;
    for (size_t i = 0; i < table->size; i++) {
        HashEntry *curr = table->buckets[i];
        while (curr != NULL && count < table->unique_count) {
            all_entries[count++] = curr;
            curr = curr->next;
        }
    }

    /* Selection pass to pick the top k most frequent entries */
    size_t limit = (k < count) ? k : count;
    for (size_t i = 0; i < limit; i++) {
        size_t max_idx = i;
        for (size_t j = i + 1; j < count; j++) {
            if (all_entries[j]->frequency > all_entries[max_idx]->frequency) {
                max_idx = j;
            }
        }
        /* Swap */
        const HashEntry *temp = all_entries[i];
        all_entries[i] = all_entries[max_idx];
        all_entries[max_idx] = temp;

        top_entries[i] = all_entries[i];
    }

    if (out_count != NULL) {
        *out_count = limit;
    }

    free(all_entries);
}

void hash_table_free(HashTable *table) {
    if (table == NULL) {
        return;
    }

    if (table->buckets != NULL) {
        for (size_t i = 0; i < table->size; i++) {
            HashEntry *curr = table->buckets[i];
            while (curr != NULL) {
                HashEntry *next = curr->next;
                if (curr->word != NULL) {
                    free(curr->word);
                    curr->word = NULL;
                }
                free(curr);
                curr = next;
            }
        }
        free(table->buckets);
        table->buckets = NULL;
    }

    table->size = 0;
    table->unique_count = 0;
    free(table);
}
