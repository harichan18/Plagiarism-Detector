#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>
#include "preprocessing.h"

/**
 * Hash Table entry/node with separate chaining.
 */
typedef struct HashEntry {
    char *word;               /* Dynamically allocated word string */
    int frequency;            /* Frequency/count of the word */
    struct HashEntry *next;   /* Pointer to next entry in bucket chain */
} HashEntry;

/**
 * Hash Table structure using separate chaining.
 */
typedef struct HashTable {
    HashEntry **buckets;      /* Array of bucket head pointers */
    size_t size;              /* Total number of buckets */
    size_t unique_count;      /* Number of unique words stored */
} HashTable;

/**
 * Creates a new hash table with the specified bucket size.
 *
 * @param size Number of buckets in the hash table.
 * @return Pointer to dynamically allocated HashTable, or NULL on error.
 */
HashTable *hash_table_create(size_t size);

/**
 * Computes hash index for a given word using the djb2 algorithm.
 *
 * @param word The string to hash.
 * @param size The size of the bucket array.
 * @return Computed bucket index.
 */
size_t hash_table_hash(const char *word, size_t size);

/**
 * Searches for a word in the hash table.
 *
 * @param table Pointer to the HashTable.
 * @param word The word to search for.
 * @param out_index Optional pointer to store the bucket index where found.
 * @return Pointer to HashEntry if found, or NULL if not found.
 */
HashEntry *hash_table_search(const HashTable *table, const char *word, size_t *out_index);

/**
 * Inserts a word with a specific frequency, or updates its frequency if already present.
 *
 * @param table Pointer to the HashTable.
 * @param word The word to insert.
 * @param frequency The frequency to set.
 * @return 1 on success, 0 on memory allocation failure.
 */
int hash_table_insert(HashTable *table, const char *word, int frequency);

/**
 * Increments the frequency of a word by 1, or inserts it with frequency 1 if new.
 *
 * @param table Pointer to the HashTable.
 * @param word The word whose count to increment.
 * @return 1 on success, 0 on failure.
 */
int hash_table_increment(HashTable *table, const char *word);

/**
 * Builds a word-frequency hash table from a TokenArray.
 *
 * @param tokens Pointer to TokenArray from Phase 1.
 * @param table_size Number of buckets to initialize.
 * @return Pointer to populated HashTable, or NULL on failure.
 */
HashTable *hash_table_build_from_tokens(const TokenArray *tokens, size_t table_size);

/**
 * Displays all word frequencies stored in the hash table.
 *
 * @param table Pointer to the HashTable.
 */
void hash_table_display(const HashTable *table);

/**
 * Finds up to top k most frequent words from the hash table.
 *
 * @param table Pointer to the HashTable.
 * @param top_entries Array of pointers to store top HashEntry references.
 * @param k Maximum number of top entries to retrieve.
 * @param out_count Pointer to store actual number of retrieved top entries.
 */
void hash_table_get_top_frequent(const HashTable *table, const HashEntry **top_entries, size_t k, size_t *out_count);

/**
 * Safely releases all memory allocated for the hash table, buckets, and nodes.
 *
 * @param table Pointer to HashTable to free.
 */
void hash_table_free(HashTable *table);

#endif /* HASHTABLE_H */
