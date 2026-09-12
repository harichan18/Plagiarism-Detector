#include "rabinkarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_MATCH_CAPACITY 8
#define INITIAL_PHRASE_CAPACITY 8

unsigned long long rabinkarp_compute_hash(const char *str, size_t len, unsigned long long prime) {
    if (str == NULL || len == 0 || prime == 0) {
        return 0;
    }

    unsigned long long hash = 0;
    for (size_t i = 0; i < len; i++) {
        hash = (hash * RABIN_KARP_BASE + (unsigned char)str[i]) % prime;
    }
    return hash;
}

MatchArray *rabin_karp_search(const char *text, const char *pattern) {
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

    const unsigned long long d = RABIN_KARP_BASE;
    const unsigned long long q = RABIN_KARP_PRIME;

    /* Compute h = d^(m-1) % q */
    unsigned long long h = 1;
    for (size_t i = 0; i < m - 1; i++) {
        h = (h * d) % q;
    }

    /* Compute initial hash value of pattern and first window of text */
    unsigned long long p_hash = 0;
    unsigned long long t_hash = 0;

    for (size_t i = 0; i < m; i++) {
        p_hash = (d * p_hash + (unsigned char)pattern[i]) % q;
        t_hash = (d * t_hash + (unsigned char)text[i]) % q;
    }

    /* Slide the window over the text */
    for (size_t i = 0; i <= n - m; i++) {
        /* Check hash values of current window and pattern */
        if (p_hash == t_hash) {
            /* Character-by-character verification to eliminate hash collisions */
            int match = 1;
            for (size_t j = 0; j < m; j++) {
                if (text[i + j] != pattern[j]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                match_array_add(matches, i);
            }
        }

        /* Calculate hash value for next window: Remove leading char, add trailing char */
        if (i < n - m) {
            unsigned long long leading = ((unsigned char)text[i] * h) % q;
            t_hash = (t_hash + q - leading) % q;
            t_hash = (t_hash * d + (unsigned char)text[i + m]) % q;
        }
    }

    return matches;
}

static char *build_phrase_from_tokens_rk(const TokenArray *tokens, size_t start, size_t count) {
    if (tokens == NULL || count == 0 || start + count > tokens->count) {
        return NULL;
    }

    size_t total_len = 0;
    for (size_t i = start; i < start + count; i++) {
        total_len += strlen(tokens->items[i].word) + 1;
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

PhraseMatchArray *find_matching_phrases_rabinkarp(const TokenArray *tokens1, const TokenArray *tokens2, size_t min_phrase_words) {
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
            char *phrase = build_phrase_from_tokens_rk(tokens1, i, best_len);
            if (phrase != NULL) {
                phrase_match_array_add(results, phrase, i, best_j, best_len, "Rabin-Karp");
                free(phrase);
            }
            i += best_len;
        } else {
            i++;
        }
    }

    return results;
}
