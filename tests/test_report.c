#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "document.h"
#include "preprocessing.h"
#include "hashtable.h"
#include "kmp.h"
#include "rabinkarp.h"
#include "similarity.h"
#include "report.h"

static void test_report_identical(void) {
    Document doc1 = {"test_ref.txt", "data structures are important in computer science.", 50, 7, 1};
    Document doc2 = {"test_comp.txt", "data structures are important in computer science.", 50, 7, 1};

    char *c1 = preprocess_text(doc1.text);
    char *c2 = preprocess_text(doc2.text);

    TokenArray *t1 = preprocess_tokenize(c1);
    TokenArray *t2 = preprocess_tokenize(c2);

    SentenceArray *s1 = preprocess_extract_sentences(c1);
    SentenceArray *s2 = preprocess_extract_sentences(c2);

    HashTable *ht1 = hash_table_build_from_tokens(t1, 0);
    HashTable *ht2 = hash_table_build_from_tokens(t2, 0);

    PlagiarismReport *rep = report_generate(&doc1, t1, s1, ht1, &doc2, t2, s2, ht2);
    assert(rep != NULL);
    assert(rep->is_identical == 1);
    assert(fabs(rep->sim_result.overall_similarity - 100.0) < 1e-5);
    assert(rep->highlighted_ref_text != NULL);
    assert(strstr(rep->highlighted_ref_text, ">>> MATCH <<<") != NULL);

    report_free(rep);
    hash_table_free(ht1);
    hash_table_free(ht2);
    sentence_array_free(s1);
    sentence_array_free(s2);
    token_array_free(t1);
    token_array_free(t2);
    free(c1);
    free(c2);

    printf("  [PASS] Report generation for identical documents\n");
}

static void test_report_different(void) {
    Document doc1 = {"apples.txt", "apples oranges bananas grapes.", 30, 4, 1};
    Document doc2 = {"cars.txt", "cars trains airplanes ships.", 28, 4, 1};

    char *c1 = preprocess_text(doc1.text);
    char *c2 = preprocess_text(doc2.text);

    TokenArray *t1 = preprocess_tokenize(c1);
    TokenArray *t2 = preprocess_tokenize(c2);

    SentenceArray *s1 = preprocess_extract_sentences(c1);
    SentenceArray *s2 = preprocess_extract_sentences(c2);

    HashTable *ht1 = hash_table_build_from_tokens(t1, 0);
    HashTable *ht2 = hash_table_build_from_tokens(t2, 0);

    PlagiarismReport *rep = report_generate(&doc1, t1, s1, ht1, &doc2, t2, s2, ht2);
    assert(rep != NULL);
    assert(rep->is_identical == 0);
    assert(fabs(rep->sim_result.overall_similarity - 0.0) < 1e-5);
    assert(rep->phrases != NULL);
    assert(rep->phrases->count == 0);

    report_free(rep);
    hash_table_free(ht1);
    hash_table_free(ht2);
    sentence_array_free(s1);
    sentence_array_free(s2);
    token_array_free(t1);
    token_array_free(t2);
    free(c1);
    free(c2);

    printf("  [PASS] Report generation for completely different documents\n");
}

static void test_duplicate_phrase_merge(void) {
    TokenArray *t1 = preprocess_tokenize("data structures are important in computer science");
    TokenArray *t2 = preprocess_tokenize("data structures are important for our analysis");

    PhraseMatchArray *dedup = find_and_deduplicate_phrases(t1, t2, 3);
    assert(dedup != NULL);
    /* Should have exactly 1 match entry, labeled "KMP + Rabin-Karp" */
    assert(dedup->count == 1);
    assert(strcmp(dedup->items[0].phrase, "data structures are important") == 0);
    assert(strcmp(dedup->items[0].algorithm, "KMP + Rabin-Karp") == 0);

    phrase_match_array_free(dedup);
    token_array_free(t1);
    token_array_free(t2);

    printf("  [PASS] Duplicate phrase deduplication (KMP + Rabin-Karp)\n");
}

static void test_report_file_export(void) {
    Document doc1 = {"orig.txt", "data structures are important for software systems.", 51, 7, 1};
    Document doc2 = {"copy.txt", "data structures are important for software systems.", 51, 7, 1};

    char *c1 = preprocess_text(doc1.text);
    char *c2 = preprocess_text(doc2.text);

    TokenArray *t1 = preprocess_tokenize(c1);
    TokenArray *t2 = preprocess_tokenize(c2);

    SentenceArray *s1 = preprocess_extract_sentences(c1);
    SentenceArray *s2 = preprocess_extract_sentences(c2);

    HashTable *ht1 = hash_table_build_from_tokens(t1, 0);
    HashTable *ht2 = hash_table_build_from_tokens(t2, 0);

    PlagiarismReport *rep = report_generate(&doc1, t1, s1, ht1, &doc2, t2, s2, ht2);
    assert(rep != NULL);

    const char *test_path = "reports/test_export_report.txt";
    int res = report_save_to_file(rep, test_path, 1);
    assert(res == 1);

    /* Verify file exists and has content */
    FILE *f = fopen(test_path, "r");
    assert(f != NULL);
    char buf[256];
    assert(fgets(buf, sizeof(buf), f) != NULL);
    fclose(f);

    /* Test overwrite rejection when allow_overwrite == 0 */
    int no_over = report_save_to_file(rep, test_path, 0);
    assert(no_over == -1);

    report_free(rep);
    hash_table_free(ht1);
    hash_table_free(ht2);
    sentence_array_free(s1);
    sentence_array_free(s2);
    token_array_free(t1);
    token_array_free(t2);
    free(c1);
    free(c2);

    printf("  [PASS] Report file export and overwrite guard\n");
}

int main(void) {
    printf("Running Plagiarism Report Unit Tests...\n");
    test_report_identical();
    test_report_different();
    test_duplicate_phrase_merge();
    test_report_file_export();
    printf("All Plagiarism Report Unit Tests Passed Successfully!\n");
    return 0;
}
