#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "document.h"
#include "preprocessing.h"
#include "hashtable.h"
#include "kmp.h"
#include "similarity.h"

static void test_identical_documents(void) {
    const char *raw = "Data structures are important. Algorithms help solve problems.";

    char *c1 = preprocess_text(raw);
    char *c2 = preprocess_text(raw);

    TokenArray *t1 = preprocess_tokenize(c1);
    TokenArray *t2 = preprocess_tokenize(c2);

    SentenceArray *s1 = preprocess_extract_sentences(c1);
    SentenceArray *s2 = preprocess_extract_sentences(c2);

    HashTable *ht1 = hash_table_build_from_tokens(t1, 0);
    HashTable *ht2 = hash_table_build_from_tokens(t2, 0);

    PhraseMatchArray *matches = find_matching_phrases_kmp(t1, t2, 3);

    SimilarityResult res = similarity_analyze(NULL, t1, s1, ht1, NULL, t2, s2, ht2, matches);

    /* For identical documents, all should be 100.0% */
    assert(fabs(res.word_similarity - 100.0) < 1e-5);
    assert(fabs(res.sentence_similarity - 100.0) < 1e-5);
    assert(fabs(res.phrase_similarity - 100.0) < 1e-5);
    assert(fabs(res.overall_similarity - 100.0) < 1e-5);
    assert(strcmp(similarity_get_level(res.overall_similarity), "Very High Similarity") == 0);

    phrase_match_array_free(matches);
    hash_table_free(ht1);
    hash_table_free(ht2);
    sentence_array_free(s1);
    sentence_array_free(s2);
    token_array_free(t1);
    token_array_free(t2);
    free(c1);
    free(c2);
    printf("  [PASS] Identical documents test (100%% similarity)\n");
}

static void test_different_documents(void) {
    const char *raw1 = "Apples oranges bananas grapes.";
    const char *raw2 = "Cars airplanes submarines trains.";

    char *c1 = preprocess_text(raw1);
    char *c2 = preprocess_text(raw2);

    TokenArray *t1 = preprocess_tokenize(c1);
    TokenArray *t2 = preprocess_tokenize(c2);

    SentenceArray *s1 = preprocess_extract_sentences(c1);
    SentenceArray *s2 = preprocess_extract_sentences(c2);

    HashTable *ht1 = hash_table_build_from_tokens(t1, 0);
    HashTable *ht2 = hash_table_build_from_tokens(t2, 0);

    PhraseMatchArray *matches = find_matching_phrases_kmp(t1, t2, 3);

    SimilarityResult res = similarity_analyze(NULL, t1, s1, ht1, NULL, t2, s2, ht2, matches);

    /* Disjoint documents should produce 0.0% without divide-by-zero */
    assert(fabs(res.word_similarity - 0.0) < 1e-5);
    assert(fabs(res.sentence_similarity - 0.0) < 1e-5);
    assert(fabs(res.phrase_similarity - 0.0) < 1e-5);
    assert(fabs(res.overall_similarity - 0.0) < 1e-5);
    assert(strcmp(similarity_get_level(res.overall_similarity), "Very Low Similarity") == 0);

    phrase_match_array_free(matches);
    hash_table_free(ht1);
    hash_table_free(ht2);
    sentence_array_free(s1);
    sentence_array_free(s2);
    token_array_free(t1);
    token_array_free(t2);
    free(c1);
    free(c2);
    printf("  [PASS] Completely different documents test (0%% similarity)\n");
}

static void test_symmetry(void) {
    const char *rawA = "Data structures are important in computer science. Fast sorting is great.";
    const char *rawB = "In software design, data structures are important for reliable systems.";

    char *cA = preprocess_text(rawA);
    char *cB = preprocess_text(rawB);

    TokenArray *tA = preprocess_tokenize(cA);
    TokenArray *tB = preprocess_tokenize(cB);

    SentenceArray *sA = preprocess_extract_sentences(cA);
    SentenceArray *sB = preprocess_extract_sentences(cB);

    HashTable *htA = hash_table_build_from_tokens(tA, 0);
    HashTable *htB = hash_table_build_from_tokens(tB, 0);

    PhraseMatchArray *matchesAB = find_matching_phrases_kmp(tA, tB, 3);
    PhraseMatchArray *matchesBA = find_matching_phrases_kmp(tB, tA, 3);

    SimilarityResult resAB = similarity_analyze(NULL, tA, sA, htA, NULL, tB, sB, htB, matchesAB);
    SimilarityResult resBA = similarity_analyze(NULL, tB, sB, htB, NULL, tA, sA, htA, matchesBA);

    /* Compare A->B vs B->A: must match within tolerance */
    assert(fabs(resAB.word_similarity - resBA.word_similarity) < 1e-5);
    assert(fabs(resAB.sentence_similarity - resBA.sentence_similarity) < 1e-5);
    assert(fabs(resAB.phrase_similarity - resBA.phrase_similarity) < 1e-5);
    assert(fabs(resAB.overall_similarity - resBA.overall_similarity) < 1e-5);

    phrase_match_array_free(matchesAB);
    phrase_match_array_free(matchesBA);
    hash_table_free(htA);
    hash_table_free(htB);
    sentence_array_free(sA);
    sentence_array_free(sB);
    token_array_free(tA);
    token_array_free(tB);
    free(cA);
    free(cB);
    printf("  [PASS] Symmetry test (A->B == B->A)\n");
}

static void test_categories(void) {
    assert(strcmp(similarity_get_level(0.0), "Very Low Similarity") == 0);
    assert(strcmp(similarity_get_level(15.5), "Very Low Similarity") == 0);
    assert(strcmp(similarity_get_level(20.0), "Low Similarity") == 0);
    assert(strcmp(similarity_get_level(35.0), "Low Similarity") == 0);
    assert(strcmp(similarity_get_level(40.0), "Moderate Similarity") == 0);
    assert(strcmp(similarity_get_level(55.0), "Moderate Similarity") == 0);
    assert(strcmp(similarity_get_level(60.0), "High Similarity") == 0);
    assert(strcmp(similarity_get_level(75.0), "High Similarity") == 0);
    assert(strcmp(similarity_get_level(80.0), "Very High Similarity") == 0);
    assert(strcmp(similarity_get_level(100.0), "Very High Similarity") == 0);
    printf("  [PASS] Similarity category thresholds test\n");
}

static void test_boundary_empty(void) {
    SimilarityResult res = similarity_analyze(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    assert(fabs(res.word_similarity - 0.0) < 1e-5);
    assert(fabs(res.sentence_similarity - 0.0) < 1e-5);
    assert(fabs(res.phrase_similarity - 0.0) < 1e-5);
    assert(fabs(res.overall_similarity - 0.0) < 1e-5);
    assert(strcmp(similarity_get_level(res.overall_similarity), "Very Low Similarity") == 0);
    printf("  [PASS] Boundary empty NULL test (no crash, 0.0%%)\n");
}

int main(void) {
    printf("Running Document Similarity Engine Unit Tests...\n");
    test_identical_documents();
    test_different_documents();
    test_symmetry();
    test_categories();
    test_boundary_empty();
    printf("All Document Similarity Engine Unit Tests Passed Successfully!\n");
    return 0;
}
