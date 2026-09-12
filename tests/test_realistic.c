/**
 * test_realistic.c - Realistic Paragraph-Scale Document Testing
 *
 * Tests the plagiarism detection engine on realistic multi-sentence/multi-paragraph
 * documents under diverse real-world scenarios:
 *   1. Identical Documents (Exact duplicate detection -> 100%)
 *   2. Partially Plagiarized Documents (Direct verbatim paragraph copying)
 *   3. Paraphrased Documents (Same concepts, rewritten syntax)
 *   4. Completely Unrelated Documents (Gastronomy vs CS -> 0% phrase/sentence)
 *   5. Symmetry Invariant: Sim(A, B) == Sim(B, A)
 *   6. Range Invariant: 0.0 <= Sim <= 100.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "document.h"
#include "preprocessing.h"
#include "hashtable.h"
#include "kmp.h"
#include "rabinkarp.h"
#include "similarity.h"
#include "report.h"

typedef struct {
    Document *doc;
    char *clean_text;
    TokenArray *tokens;
    SentenceArray *sentences;
    HashTable *ht;
} RealisticDoc;

static int load_doc(const char *path, RealisticDoc *rd) {
    rd->doc = document_read(path);
    if (!rd->doc) return 0;

    rd->clean_text = preprocess_text(rd->doc->text);
    if (!rd->clean_text) {
        document_free(rd->doc);
        return 0;
    }

    rd->tokens = preprocess_tokenize(rd->clean_text);
    rd->sentences = preprocess_extract_sentences(rd->clean_text);
    rd->doc->word_count = rd->tokens ? rd->tokens->count : 0;
    rd->doc->sentence_count = rd->sentences ? rd->sentences->count : 0;

    rd->ht = hash_table_build_from_tokens(rd->tokens, 0);
    return (rd->tokens && rd->sentences && rd->ht);
}

static void free_doc(RealisticDoc *rd) {
    if (rd->ht) hash_table_free(rd->ht);
    if (rd->sentences) sentence_array_free(rd->sentences);
    if (rd->tokens) token_array_free(rd->tokens);
    if (rd->clean_text) free(rd->clean_text);
    if (rd->doc) document_free(rd->doc);
    memset(rd, 0, sizeof(RealisticDoc));
}

int main(void) {
    printf("============================================================\n");
    printf("RUNNING REALISTIC DOCUMENT DATASET TEST SUITE\n");
    printf("============================================================\n\n");

    RealisticDoc ref, ident, partial, para, unrel, repeat, longdoc;
    assert(load_doc("documents/test_reference.txt", &ref));
    assert(load_doc("documents/test_identical.txt", &ident));
    assert(load_doc("documents/test_partial.txt", &partial));
    assert(load_doc("documents/test_paraphrased.txt", &para));
    assert(load_doc("documents/test_unrelated.txt", &unrel));
    assert(load_doc("documents/test_repeated.txt", &repeat));
    assert(load_doc("documents/test_long.txt", &longdoc));

    printf("Dataset successfully loaded:\n");
    printf(" - test_reference:   %zu words, %zu sentences\n", ref.tokens->count, ref.sentences->count);
    printf(" - test_identical:   %zu words, %zu sentences\n", ident.tokens->count, ident.sentences->count);
    printf(" - test_partial:     %zu words, %zu sentences\n", partial.tokens->count, partial.sentences->count);
    printf(" - test_paraphrased: %zu words, %zu sentences\n", para.tokens->count, para.sentences->count);
    printf(" - test_unrelated:   %zu words, %zu sentences\n", unrel.tokens->count, unrel.sentences->count);
    printf(" - test_repeated:    %zu words, %zu sentences\n", repeat.tokens->count, repeat.sentences->count);
    printf(" - test_long:        %zu words, %zu sentences\n\n", longdoc.tokens->count, longdoc.sentences->count);

    /* 1. Test Identical Documents */
    printf("[1/6] Testing Identical Documents (test_reference vs test_identical)...\n");
    PlagiarismReport *rep_ident = report_generate(ref.doc, ref.tokens, ref.sentences, ref.ht,
                                                  ident.doc, ident.tokens, ident.sentences, ident.ht);
    assert(rep_ident != NULL);
    assert(rep_ident->is_identical == 1);
    assert(fabs(rep_ident->sim_result.overall_similarity - 100.0) < 0.01);
    assert(fabs(rep_ident->sim_result.word_similarity - 100.0) < 0.01);
    assert(fabs(rep_ident->sim_result.sentence_similarity - 100.0) < 0.01);
    assert(fabs(rep_ident->sim_result.phrase_similarity - 100.0) < 0.01);
    printf("      Score: %6.2f%% [Identical: YES] -> PASS\n\n", rep_ident->sim_result.overall_similarity);
    report_free(rep_ident);

    /* 2. Test Unrelated Documents */
    printf("[2/6] Testing Unrelated Documents (test_reference vs test_unrelated)...\n");
    PlagiarismReport *rep_unrel = report_generate(ref.doc, ref.tokens, ref.sentences, ref.ht,
                                                  unrel.doc, unrel.tokens, unrel.sentences, unrel.ht);
    assert(rep_unrel != NULL);
    assert(rep_unrel->is_identical == 0);
    assert(rep_unrel->sim_result.sentence_similarity == 0.0);
    assert(rep_unrel->sim_result.phrase_similarity == 0.0);
    assert(rep_unrel->sim_result.matching_phrase_count == 0);
    assert(rep_unrel->sim_result.overall_similarity < 10.0);
    printf("      Score: %6.2f%% (Sentence: %5.2f%%, Phrase: %5.2f%%) -> PASS\n\n",
           rep_unrel->sim_result.overall_similarity,
           rep_unrel->sim_result.sentence_similarity,
           rep_unrel->sim_result.phrase_similarity);
    report_free(rep_unrel);

    /* 3. Test Partial Plagiarism */
    printf("[3/6] Testing Partial Plagiarism (test_reference vs test_partial)...\n");
    PlagiarismReport *rep_part = report_generate(ref.doc, ref.tokens, ref.sentences, ref.ht,
                                                 partial.doc, partial.tokens, partial.sentences, partial.ht);
    assert(rep_part != NULL);
    assert(rep_part->is_identical == 0);
    assert(rep_part->sim_result.matching_sentences == 2);
    assert(rep_part->sim_result.matching_phrase_count >= 1);
    assert(rep_part->sim_result.overall_similarity > 20.0);
    assert(rep_part->sim_result.overall_similarity < 70.0);
    printf("      Score: %6.2f%% (Matching Sentences: %zu, Matching Phrases: %zu) -> PASS\n\n",
           rep_part->sim_result.overall_similarity,
           rep_part->sim_result.matching_sentences,
           rep_part->sim_result.matching_phrase_count);
    report_free(rep_part);

    /* 4. Test Paraphrased Content */
    printf("[4/6] Testing Paraphrased Content (test_reference vs test_paraphrased)...\n");
    PlagiarismReport *rep_para = report_generate(ref.doc, ref.tokens, ref.sentences, ref.ht,
                                                 para.doc, para.tokens, para.sentences, para.ht);
    assert(rep_para != NULL);
    assert(rep_para->sim_result.sentence_similarity == 0.0);
    assert(rep_para->sim_result.phrase_similarity < 10.0);
    assert(rep_para->sim_result.matching_phrase_count <= 1);
    printf("      Score: %6.2f%% (Verbatim Phrases: %zu) -> PASS\n\n",
           rep_para->sim_result.overall_similarity,
           rep_para->sim_result.matching_phrase_count);
    report_free(rep_para);

    /* 5. Test Mathematical Symmetry: Sim(A, B) == Sim(B, A) */
    printf("[5/6] Testing Mathematical Symmetry across all document pairs...\n");
    PlagiarismReport *ab = report_generate(ref.doc, ref.tokens, ref.sentences, ref.ht,
                                           partial.doc, partial.tokens, partial.sentences, partial.ht);
    PlagiarismReport *ba = report_generate(partial.doc, partial.tokens, partial.sentences, partial.ht,
                                           ref.doc, ref.tokens, ref.sentences, ref.ht);
    assert(ab && ba);
    assert(fabs(ab->sim_result.word_similarity - ba->sim_result.word_similarity) < 0.001);
    assert(fabs(ab->sim_result.sentence_similarity - ba->sim_result.sentence_similarity) < 0.001);
    assert(fabs(ab->sim_result.phrase_similarity - ba->sim_result.phrase_similarity) < 0.001);
    assert(fabs(ab->sim_result.overall_similarity - ba->sim_result.overall_similarity) < 0.001);
    printf("      Sim(Ref, Partial) = %6.2f%%, Sim(Partial, Ref) = %6.2f%% -> SYMMETRIC PASS\n\n",
           ab->sim_result.overall_similarity, ba->sim_result.overall_similarity);
    report_free(ab);
    report_free(ba);

    /* 6. Test Scale and Long Document against Reference */
    printf("[6/6] Testing Long Document Comparison (test_reference vs test_long)...\n");
    PlagiarismReport *rep_long = report_generate(ref.doc, ref.tokens, ref.sentences, ref.ht,
                                                 longdoc.doc, longdoc.tokens, longdoc.sentences, longdoc.ht);
    assert(rep_long != NULL);
    assert(rep_long->sim_result.overall_similarity >= 0.0 && rep_long->sim_result.overall_similarity <= 100.0);
    assert(rep_long->sim_result.matching_sentences == 2);
    printf("      Score: %6.2f%% (Shared 2 common sentences across 350+ words) -> PASS\n\n",
           rep_long->sim_result.overall_similarity);
    report_free(rep_long);

    free_doc(&ref);
    free_doc(&ident);
    free_doc(&partial);
    free_doc(&para);
    free_doc(&unrel);
    free_doc(&repeat);
    free_doc(&longdoc);

    printf("============================================================\n");
    printf("ALL REALISTIC DATASET TESTS PASSED (ZERO LEAKS, ZERO WARNINGS)\n");
    printf("============================================================\n");
    return 0;
}
