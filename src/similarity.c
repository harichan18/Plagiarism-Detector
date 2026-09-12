#include "similarity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double similarity_compute_word_jaccard(const HashTable *ht1, const HashTable *ht2, size_t *out_common) {
    if (out_common != NULL) {
        *out_common = 0;
    }

    if (ht1 == NULL || ht2 == NULL || ht1->unique_count == 0 || ht2->unique_count == 0) {
        return 0.0;
    }

    size_t common_count = 0;

    for (size_t i = 0; i < ht1->size; i++) {
        HashEntry *curr = ht1->buckets[i];
        while (curr != NULL) {
            if (hash_table_search(ht2, curr->word, NULL) != NULL) {
                common_count++;
            }
            curr = curr->next;
        }
    }

    if (out_common != NULL) {
        *out_common = common_count;
    }

    size_t union_count = ht1->unique_count + ht2->unique_count - common_count;
    if (union_count == 0) {
        return 0.0;
    }

    double jaccard = ((double)common_count / (double)union_count) * 100.0;
    if (jaccard > 100.0) jaccard = 100.0;
    if (jaccard < 0.0) jaccard = 0.0;

    return jaccard;
}

/* Helper to check if a sentence text exists in an array of strings */
static int str_in_list(const char **list, size_t count, const char *str) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(list[i], str) == 0) {
            return 1;
        }
    }
    return 0;
}

double similarity_compute_sentence(const SentenceArray *s1, const SentenceArray *s2, size_t *out_matching) {
    if (out_matching != NULL) {
        *out_matching = 0;
    }

    if (s1 == NULL || s2 == NULL || s1->count == 0 || s2->count == 0) {
        return 0.0;
    }

    /* Extract unique sentences from s1 */
    const char **u1 = (const char **)malloc(s1->count * sizeof(const char *));
    size_t u1_count = 0;
    if (u1 == NULL) return 0.0;

    for (size_t i = 0; i < s1->count; i++) {
        if (!str_in_list(u1, u1_count, s1->items[i].text)) {
            u1[u1_count++] = s1->items[i].text;
        }
    }

    /* Extract unique sentences from s2 */
    const char **u2 = (const char **)malloc(s2->count * sizeof(const char *));
    size_t u2_count = 0;
    if (u2 == NULL) {
        free(u1);
        return 0.0;
    }

    for (size_t i = 0; i < s2->count; i++) {
        if (!str_in_list(u2, u2_count, s2->items[i].text)) {
            u2[u2_count++] = s2->items[i].text;
        }
    }

    /* Count common unique sentences */
    size_t matching_count = 0;
    for (size_t i = 0; i < u1_count; i++) {
        if (str_in_list(u2, u2_count, u1[i])) {
            matching_count++;
        }
    }

    if (out_matching != NULL) {
        *out_matching = matching_count;
    }

    size_t union_sentences = u1_count + u2_count - matching_count;
    free(u1);
    free(u2);

    if (union_sentences == 0) {
        return 0.0;
    }

    double score = ((double)matching_count / (double)union_sentences) * 100.0;
    if (score > 100.0) score = 100.0;
    if (score < 0.0) score = 0.0;

    return score;
}

double similarity_compute_phrase(const PhraseMatchArray *phrases, size_t words1, size_t words2, size_t *out_matched_words) {
    if (out_matched_words != NULL) {
        *out_matched_words = 0;
    }

    size_t max_words = (words1 > words2) ? words1 : words2;
    if (max_words == 0 || phrases == NULL || phrases->count == 0) {
        return 0.0;
    }

    char *covered1 = (char *)calloc(words1 > 0 ? words1 : 1, sizeof(char));
    char *covered2 = (char *)calloc(words2 > 0 ? words2 : 1, sizeof(char));

    if (covered1 == NULL || covered2 == NULL) {
        if (covered1) free(covered1);
        if (covered2) free(covered2);
        return 0.0;
    }

    for (size_t i = 0; i < phrases->count; i++) {
        size_t p1 = phrases->items[i].doc1_pos;
        size_t p2 = phrases->items[i].doc2_pos;
        size_t wc = phrases->items[i].word_count;

        for (size_t k = 0; k < wc && p1 + k < words1; k++) {
            covered1[p1 + k] = 1;
        }
        for (size_t k = 0; k < wc && p2 + k < words2; k++) {
            covered2[p2 + k] = 1;
        }
    }

    size_t count1 = 0;
    for (size_t i = 0; i < words1; i++) {
        if (covered1[i]) count1++;
    }

    size_t count2 = 0;
    for (size_t i = 0; i < words2; i++) {
        if (covered2[i]) count2++;
    }

    free(covered1);
    free(covered2);

    size_t unique_covered = (count1 > count2) ? count1 : count2;

    if (out_matched_words != NULL) {
        *out_matched_words = unique_covered;
    }

    double score = ((double)unique_covered / (double)max_words) * 100.0;
    if (score > 100.0) score = 100.0;
    if (score < 0.0) score = 0.0;

    return score;
}

SimilarityResult similarity_analyze(const Document *doc1,
                                   const TokenArray *t1,
                                   const SentenceArray *s1,
                                   const HashTable *ht1,
                                   const Document *doc2,
                                   const TokenArray *t2,
                                   const SentenceArray *s2,
                                   const HashTable *ht2,
                                   const PhraseMatchArray *phrases) {
    SimilarityResult res;
    memset(&res, 0, sizeof(SimilarityResult));

    res.total_words_doc1 = (t1 != NULL) ? t1->count : (doc1 != NULL ? doc1->word_count : 0);
    res.total_words_doc2 = (t2 != NULL) ? t2->count : (doc2 != NULL ? doc2->word_count : 0);
    res.total_sentences_doc1 = (s1 != NULL) ? s1->count : (doc1 != NULL ? doc1->sentence_count : 0);
    res.total_sentences_doc2 = (s2 != NULL) ? s2->count : (doc2 != NULL ? doc2->sentence_count : 0);

    /* 1. Word similarity */
    res.word_similarity = similarity_compute_word_jaccard(ht1, ht2, &res.common_words);

    /* 2. Sentence similarity */
    res.sentence_similarity = similarity_compute_sentence(s1, s2, &res.matching_sentences);

    /* 3. Phrase similarity */
    res.matching_phrase_count = (phrases != NULL) ? phrases->count : 0;
    res.phrase_similarity = similarity_compute_phrase(phrases, res.total_words_doc1, res.total_words_doc2, &res.matching_phrase_words);

    /* 4. Weighted overall score */
    res.overall_similarity = (res.word_similarity * WORD_WEIGHT) +
                             (res.sentence_similarity * SENTENCE_WEIGHT) +
                             (res.phrase_similarity * PHRASE_WEIGHT);

    if (res.overall_similarity > 100.0) res.overall_similarity = 100.0;
    if (res.overall_similarity < 0.0) res.overall_similarity = 0.0;

    return res;
}

const char *similarity_get_level(double overall_similarity) {
    if (overall_similarity >= 80.0) {
        return "Very High Similarity";
    } else if (overall_similarity >= 60.0) {
        return "High Similarity";
    } else if (overall_similarity >= 40.0) {
        return "Moderate Similarity";
    } else if (overall_similarity >= 20.0) {
        return "Low Similarity";
    } else {
        return "Very Low Similarity";
    }
}
