#ifndef KMP_H
#define KMP_H

#include <stddef.h>
#include "preprocessing.h"

#define DEFAULT_MIN_PHRASE_WORDS 3

/**
 * Dynamically resizable array of matching character positions.
 */
typedef struct MatchArray {
    size_t *positions; /* Array of 0-based match start positions */
    size_t count;      /* Number of matches found */
    size_t capacity;   /* Allocated capacity */
} MatchArray;

/**
 * Represents a matched phrase between two documents.
 */
typedef struct MatchingPhrase {
    char *phrase;              /* Text of the matching phrase */
    size_t doc1_pos;           /* Start token position in document 1 */
    size_t doc2_pos;           /* Start token position in document 2 */
    size_t word_count;         /* Number of consecutive matching words */
    const char *algorithm;     /* Algorithm name ("KMP" or "Rabin-Karp") */
} MatchingPhrase;

/**
 * Dynamically resizable array of MatchingPhrases.
 */
typedef struct PhraseMatchArray {
    MatchingPhrase *items;     /* Array of matching phrases */
    size_t count;              /* Number of phrase matches */
    size_t capacity;           /* Allocated capacity */
} PhraseMatchArray;

/* MatchArray memory management */
MatchArray *match_array_create(size_t initial_capacity);
int match_array_add(MatchArray *arr, size_t pos);
void match_array_free(MatchArray *arr);

/* KMP Algorithm functions */
int *kmp_build_lps(const char *pattern, size_t m);
MatchArray *kmp_search(const char *text, const char *pattern);

/* PhraseMatchArray memory management */
PhraseMatchArray *phrase_match_array_create(size_t initial_capacity);
int phrase_match_array_add(PhraseMatchArray *arr, const char *phrase, size_t doc1_pos, size_t doc2_pos, size_t word_count, const char *algorithm);
void phrase_match_array_free(PhraseMatchArray *arr);

/**
 * Finds maximal matching phrases (>= min_phrase_words) between two token sequences using KMP.
 * Avoids reporting overlapping sub-phrases.
 */
PhraseMatchArray *find_matching_phrases_kmp(const TokenArray *tokens1, const TokenArray *tokens2, size_t min_phrase_words);

#endif /* KMP_H */
