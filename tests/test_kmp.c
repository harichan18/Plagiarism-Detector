#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "kmp.h"

int main(void) {
    printf("Running KMP Unit Tests...\n");

    const char *text = "the quick brown fox jumps over the lazy dog";

    /* 1. Pattern in middle */
    MatchArray *m1 = kmp_search(text, "brown fox");
    assert(m1 != NULL);
    assert(m1->count == 1);
    assert(m1->positions[0] == 10);
    match_array_free(m1);

    /* 2. Pattern at beginning */
    MatchArray *m2 = kmp_search(text, "the quick");
    assert(m2 != NULL);
    assert(m2->count == 1);
    assert(m2->positions[0] == 0);
    match_array_free(m2);

    /* 3. Pattern at end */
    MatchArray *m3 = kmp_search(text, "lazy dog");
    assert(m3 != NULL);
    assert(m3->count == 1);
    assert(m3->positions[0] == 35);
    match_array_free(m3);

    /* 4. Pattern occurring multiple times */
    MatchArray *m4 = kmp_search(text, "the");
    assert(m4 != NULL);
    assert(m4->count == 2);
    assert(m4->positions[0] == 0);
    assert(m4->positions[1] == 31);
    match_array_free(m4);

    /* 5. Pattern not found */
    MatchArray *m5 = kmp_search(text, "elephant");
    assert(m5 != NULL);
    assert(m5->count == 0);
    match_array_free(m5);

    /* 6. Pattern equal to text */
    MatchArray *m6 = kmp_search("exact match", "exact match");
    assert(m6 != NULL);
    assert(m6->count == 1);
    assert(m6->positions[0] == 0);
    match_array_free(m6);

    /* 7. Empty pattern */
    MatchArray *m7 = kmp_search(text, "");
    assert(m7 != NULL);
    assert(m7->count == 0);
    match_array_free(m7);

    /* 8. Pattern longer than text */
    MatchArray *m8 = kmp_search("short", "much longer pattern");
    assert(m8 != NULL);
    assert(m8->count == 0);
    match_array_free(m8);

    /* 9. LPS Array computation test */
    int *lps = kmp_build_lps("aabaabaaa", 9);
    assert(lps != NULL);
    /* a=0, aa=1, aab=0, aaba=1, aabaa=2, aabaab=3, aabaaba=4, aabaabaa=5, aabaabaaa=2 */
    assert(lps[0] == 0);
    assert(lps[1] == 1);
    assert(lps[2] == 0);
    assert(lps[3] == 1);
    assert(lps[4] == 2);
    assert(lps[5] == 3);
    assert(lps[6] == 4);
    assert(lps[7] == 5);
    assert(lps[8] == 2);
    free(lps);

    /* 10. Phrase match array test */
    TokenArray *t1 = preprocess_tokenize("data structures are important in computer science");
    TokenArray *t2 = preprocess_tokenize("we know that data structures are important for our work");
    PhraseMatchArray *pma = find_matching_phrases_kmp(t1, t2, 3);
    assert(pma != NULL);
    assert(pma->count == 1);
    assert(strcmp(pma->items[0].phrase, "data structures are important") == 0);
    assert(pma->items[0].word_count == 4);
    assert(pma->items[0].doc1_pos == 0);
    assert(pma->items[0].doc2_pos == 3);
    phrase_match_array_free(pma);
    token_array_free(t1);
    token_array_free(t2);

    printf("All KMP Unit Tests Passed Successfully!\n");
    return 0;
}
