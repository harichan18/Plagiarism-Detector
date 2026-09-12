#include "kmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_MATCH_CAPACITY 8
#define INITIAL_PHRASE_CAPACITY 8

MatchArray *match_array_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = INITIAL_MATCH_CAPACITY;
    }

    MatchArray *arr = (MatchArray *)malloc(sizeof(MatchArray));
    if (arr == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for MatchArray.\n");
        return NULL;
    }

    arr->count = 0;
    arr->capacity = initial_capacity;
    arr->positions = (size_t *)malloc(arr->capacity * sizeof(size_t));
    if (arr->positions == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for MatchArray positions.\n");
        free(arr);
        return NULL;
    }

    return arr;
}

int match_array_add(MatchArray *arr, size_t pos) {
    if (arr == NULL) {
        return 0;
    }

    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity * 2;
        size_t *new_pos = (size_t *)realloc(arr->positions, new_cap * sizeof(size_t));
        if (new_pos == NULL) {
            fprintf(stderr, "Error: Memory reallocation failed for MatchArray.\n");
            return 0;
        }
        arr->positions = new_pos;
        arr->capacity = new_cap;
    }

    arr->positions[arr->count++] = pos;
    return 1;
}

void match_array_free(MatchArray *arr) {
    if (arr == NULL) {
        return;
    }
    if (arr->positions != NULL) {
        free(arr->positions);
        arr->positions = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
    free(arr);
}

int *kmp_build_lps(const char *pattern, size_t m) {
    if (pattern == NULL || m == 0) {
        return NULL;
    }

    int *lps = (int *)malloc(m * sizeof(int));
    if (lps == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for LPS array.\n");
        return NULL;
    }

    lps[0] = 0;
    int len = 0;
    size_t i = 1;

    while (i < m) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }

    return lps;
}

MatchArray *kmp_search(const char *text, const char *pattern) {
    if (text == NULL || pattern == NULL) {
        return NULL;
    }

    MatchArray *matches = match_array_create(INITIAL_MATCH_CAPACITY);
    if (matches == NULL) {
        return NULL;
    }

    size_t m = strlen(pattern);
    size_t n = strlen(text);

    if (m == 0 || m > n) {
        return matches;
    }

    int *lps = kmp_build_lps(pattern, m);
    if (lps == NULL) {
        match_array_free(matches);
        return NULL;
    }

    size_t i = 0; /* Index for text */
    size_t j = 0; /* Index for pattern */

    while (i < n) {
        if (pattern[j] == text[i]) {
            i++;
            j++;
        }

        if (j == m) {
            match_array_add(matches, i - j);
            j = (size_t)lps[j - 1];
        } else if (i < n && pattern[j] != text[i]) {
            if (j != 0) {
                j = (size_t)lps[j - 1];
            } else {
                i++;
            }
        }
    }

    free(lps);
    return matches;
}

PhraseMatchArray *phrase_match_array_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = INITIAL_PHRASE_CAPACITY;
    }

    PhraseMatchArray *arr = (PhraseMatchArray *)malloc(sizeof(PhraseMatchArray));
    if (arr == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for PhraseMatchArray.\n");
        return NULL;
    }

    arr->count = 0;
    arr->capacity = initial_capacity;
    arr->items = (MatchingPhrase *)malloc(arr->capacity * sizeof(MatchingPhrase));
    if (arr->items == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for PhraseMatchArray items.\n");
        free(arr);
        return NULL;
    }

    return arr;
}

int phrase_match_array_add(PhraseMatchArray *arr, const char *phrase, size_t doc1_pos, size_t doc2_pos, size_t word_count, const char *algorithm) {
    if (arr == NULL || phrase == NULL) {
        return 0;
    }

    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity * 2;
        MatchingPhrase *new_items = (MatchingPhrase *)realloc(arr->items, new_cap * sizeof(MatchingPhrase));
        if (new_items == NULL) {
            fprintf(stderr, "Error: Memory reallocation failed for PhraseMatchArray.\n");
            return 0;
        }
        arr->items = new_items;
        arr->capacity = new_cap;
    }

    size_t phrase_len = strlen(phrase);
    char *phrase_copy = (char *)malloc(phrase_len + 1);
    if (phrase_copy == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for phrase copy.\n");
        return 0;
    }
    memcpy(phrase_copy, phrase, phrase_len + 1);

    arr->items[arr->count].phrase = phrase_copy;
    arr->items[arr->count].doc1_pos = doc1_pos;
    arr->items[arr->count].doc2_pos = doc2_pos;
    arr->items[arr->count].word_count = word_count;
    arr->items[arr->count].algorithm = algorithm;
    arr->count++;

    return 1;
}

void phrase_match_array_free(PhraseMatchArray *arr) {
    if (arr == NULL) {
        return;
    }

    if (arr->items != NULL) {
        for (size_t i = 0; i < arr->count; i++) {
            if (arr->items[i].phrase != NULL) {
                free(arr->items[i].phrase);
                arr->items[i].phrase = NULL;
            }
        }
        free(arr->items);
        arr->items = NULL;
    }

    arr->count = 0;
    arr->capacity = 0;
    free(arr);
}

static char *build_phrase_from_tokens(const TokenArray *tokens, size_t start, size_t count) {
    if (tokens == NULL || count == 0 || start + count > tokens->count) {
        return NULL;
    }

    size_t total_len = 0;
    for (size_t i = start; i < start + count; i++) {
        total_len += strlen(tokens->items[i].word) + 1; /* word + space or null */
    }

    char *phrase = (char *)malloc(total_len + 1);
    if (phrase == NULL) {
        return NULL;
    }

    phrase[0] = '\0';
    for (size_t i = start; i < start + count; i++) {
        strcat(phrase, tokens->items[i].word);
        if (i + 1 < start + count) {
            strcat(phrase, " ");
        }
    }

    return phrase;
}

PhraseMatchArray *find_matching_phrases_kmp(const TokenArray *tokens1, const TokenArray *tokens2, size_t min_phrase_words) {
    if (tokens1 == NULL || tokens2 == NULL) {
        return NULL;
    }

    if (min_phrase_words == 0) {
        min_phrase_words = DEFAULT_MIN_PHRASE_WORDS;
    }

    PhraseMatchArray *results = phrase_match_array_create(INITIAL_PHRASE_CAPACITY);
    if (results == NULL) {
        return NULL;
    }

    if (tokens1->count < min_phrase_words || tokens2->count < min_phrase_words) {
        return results;
    }

    size_t i = 0;
    while (i + min_phrase_words <= tokens1->count) {
        size_t best_len = 0;
        size_t best_j = 0;

        for (size_t j = 0; j + min_phrase_words <= tokens2->count; j++) {
            size_t k = 0;
            while (i + k < tokens1->count && j + k < tokens2->count &&
                   strcmp(tokens1->items[i + k].word, tokens2->items[j + k].word) == 0) {
                k++;
            }

            if (k > best_len) {
                best_len = k;
                best_j = j;
            }
        }

        if (best_len >= min_phrase_words) {
            char *phrase = build_phrase_from_tokens(tokens1, i, best_len);
            if (phrase != NULL) {
                phrase_match_array_add(results, phrase, i, best_j, best_len, "KMP");
                free(phrase);
            }
            /* Advance i past this entire match to prevent overlapping sub-phrases */
            i += best_len;
        } else {
            i++;
        }
    }

    return results;
}
