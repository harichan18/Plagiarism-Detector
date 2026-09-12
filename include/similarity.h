#ifndef SIMILARITY_H
#define SIMILARITY_H

#include <stddef.h>
#include "document.h"
#include "preprocessing.h"
#include "hashtable.h"
#include "kmp.h"

#define WORD_WEIGHT 0.40
#define SENTENCE_WEIGHT 0.30
#define PHRASE_WEIGHT 0.30

/**
 * Detailed similarity analysis report between two documents.
 */
typedef struct SimilarityResult {
    double word_similarity;        /* Jaccard similarity of vocabulary (0 - 100%) */
    double sentence_similarity;    /* Jaccard similarity of unique sentences (0 - 100%) */
    double phrase_similarity;      /* Ratio of matched phrase words to max document words (0 - 100%) */
    double overall_similarity;     /* Weighted combination score (0 - 100%) */

    size_t total_words_doc1;       /* Total word count in Document 1 */
    size_t total_words_doc2;       /* Total word count in Document 2 */
    size_t common_words;           /* Count of unique vocabulary words shared by both */

    size_t total_sentences_doc1;   /* Total sentence count in Document 1 */
    size_t total_sentences_doc2;   /* Total sentence count in Document 2 */
    size_t matching_sentences;     /* Count of unique identical sentences shared by both */

    size_t matching_phrase_count;  /* Number of maximal matching phrases found */
    size_t matching_phrase_words;  /* Total words across all matching phrases */
} SimilarityResult;

/**
 * Computes symmetric Jaccard word similarity between two document HashTables.
 */
double similarity_compute_word_jaccard(const HashTable *ht1, const HashTable *ht2, size_t *out_common);

/**
 * Computes symmetric Jaccard sentence similarity between two SentenceArrays.
 */
double similarity_compute_sentence(const SentenceArray *s1, const SentenceArray *s2, size_t *out_matching);

/**
 * Computes phrase similarity from matched phrases relative to max document word length.
 */
double similarity_compute_phrase(const PhraseMatchArray *phrases, size_t words1, size_t words2, size_t *out_matched_words);

/**
 * Performs full multi-metric similarity analysis between two documents.
 */
SimilarityResult similarity_analyze(const Document *doc1,
                                   const TokenArray *t1,
                                   const SentenceArray *s1,
                                   const HashTable *ht1,
                                   const Document *doc2,
                                   const TokenArray *t2,
                                   const SentenceArray *s2,
                                   const HashTable *ht2,
                                   const PhraseMatchArray *phrases);

/**
 * Returns descriptive similarity category for a given percentage score.
 */
const char *similarity_get_level(double overall_similarity);

#endif /* SIMILARITY_H */
