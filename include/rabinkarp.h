#ifndef RABINKARP_H
#define RABINKARP_H

#include <stddef.h>
#include "kmp.h"

#define RABIN_KARP_BASE 256
#define RABIN_KARP_PRIME 1000000007ULL

/**
 * Computes the polynomial rolling hash of a string prefix of given length.
 *
 * @param str Input string.
 * @param len Length of string to hash.
 * @param prime Modulus prime number.
 * @return Computed hash value.
 */
unsigned long long rabinkarp_compute_hash(const char *str, size_t len, unsigned long long prime);

/**
 * Searches for all occurrences of pattern in text using the Rabin-Karp rolling hash algorithm.
 *
 * @param text The document text to search within.
 * @param pattern The pattern string to look for.
 * @return Pointer to MatchArray containing all 0-based matching starting positions.
 */
MatchArray *rabin_karp_search(const char *text, const char *pattern);

/**
 * Finds maximal matching phrases (>= min_phrase_words) between two token sequences using Rabin-Karp.
 *
 * @param tokens1 TokenArray of first document.
 * @param tokens2 TokenArray of second document.
 * @param min_phrase_words Minimum number of words to consider a match (default: 3).
 * @return Pointer to PhraseMatchArray containing unique matching phrases.
 */
PhraseMatchArray *find_matching_phrases_rabinkarp(const TokenArray *tokens1, const TokenArray *tokens2, size_t min_phrase_words);

#endif /* RABINKARP_H */
