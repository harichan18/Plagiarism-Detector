#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hashtable.h"

int main(void) {
    printf("Running Hash Table Unit Tests...\n");

    /* 1. Test creation */
    HashTable *ht = hash_table_create(5);
    assert(ht != NULL);
    assert(ht->size == 5);
    assert(ht->unique_count == 0);

    /* 2. Test insertion & frequency increment */
    assert(hash_table_increment(ht, "apple") == 1);
    assert(hash_table_increment(ht, "apple") == 1);
    assert(ht->unique_count == 1);

    size_t idx = 0;
    HashEntry *e = hash_table_search(ht, "apple", &idx);
    assert(e != NULL);
    assert(strcmp(e->word, "apple") == 0);
    assert(e->frequency == 2);

    /* 3. Test collisions: insert multiple words into small table (5 buckets) */
    assert(hash_table_increment(ht, "banana") == 1);
    assert(hash_table_increment(ht, "cherry") == 1);
    assert(hash_table_increment(ht, "date") == 1);
    assert(hash_table_increment(ht, "elderberry") == 1);
    assert(hash_table_increment(ht, "fig") == 1);
    assert(hash_table_increment(ht, "grape") == 1);
    assert(ht->unique_count == 7);

    /* Verify every word can be found despite inevitable collisions */
    const char *words[] = {"apple", "banana", "cherry", "date", "elderberry", "fig", "grape"};
    for (int i = 0; i < 7; i++) {
        HashEntry *found = hash_table_search(ht, words[i], NULL);
        assert(found != NULL);
        assert(strcmp(found->word, words[i]) == 0);
    }

    /* 4. Test non-existent word */
    assert(hash_table_search(ht, "watermelon", NULL) == NULL);

    /* 5. Test top frequent extraction */
    assert(hash_table_increment(ht, "banana") == 1);
    assert(hash_table_increment(ht, "banana") == 1); /* banana count: 3 */
    /* apple: 2, banana: 3, others: 1 */

    const HashEntry *top[3];
    size_t top_count = 0;
    hash_table_get_top_frequent(ht, top, 3, &top_count);
    assert(top_count == 3);
    assert(strcmp(top[0]->word, "banana") == 0);
    assert(top[0]->frequency == 3);
    assert(strcmp(top[1]->word, "apple") == 0);
    assert(top[1]->frequency == 2);

    /* 6. Test memory cleanup */
    hash_table_free(ht);

    printf("All Hash Table Unit Tests Passed Successfully!\n");
    return 0;
}
