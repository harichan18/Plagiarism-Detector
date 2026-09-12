#include "report.h"
#include "search.h"
#include "rabinkarp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MAKE_DIR(path) mkdir(path, 0755)
#endif

PhraseMatchArray *find_and_deduplicate_phrases(const TokenArray *t1, const TokenArray *t2, size_t min_words) {
    PhraseMatchArray *kmp_matches = find_matching_phrases_kmp(t1, t2, min_words);
    PhraseMatchArray *rk_matches = find_matching_phrases_rabinkarp(t1, t2, min_words);

    PhraseMatchArray *merged = phrase_match_array_create(
        (kmp_matches ? kmp_matches->count : 0) + (rk_matches ? rk_matches->count : 0) + 4
    );

    if (merged == NULL) {
        if (kmp_matches) phrase_match_array_free(kmp_matches);
        if (rk_matches) phrase_match_array_free(rk_matches);
        return NULL;
    }

    /* Add KMP matches into merged */
    if (kmp_matches != NULL) {
        for (size_t i = 0; i < kmp_matches->count; i++) {
            phrase_match_array_add(merged,
                                   kmp_matches->items[i].phrase,
                                   kmp_matches->items[i].doc1_pos,
                                   kmp_matches->items[i].doc2_pos,
                                   kmp_matches->items[i].word_count,
                                   "KMP");
        }
    }

    /* Merge Rabin-Karp matches: deduplicate if match at same positions already recorded */
    if (rk_matches != NULL) {
        for (size_t i = 0; i < rk_matches->count; i++) {
            int found_duplicate = 0;
            for (size_t j = 0; j < merged->count; j++) {
                if (merged->items[j].doc1_pos == rk_matches->items[i].doc1_pos &&
                    merged->items[j].doc2_pos == rk_matches->items[i].doc2_pos) {
                    /* Match detected by both algorithms */
                    merged->items[j].algorithm = "KMP + Rabin-Karp";
                    found_duplicate = 1;
                    break;
                }
            }

            if (!found_duplicate) {
                phrase_match_array_add(merged,
                                       rk_matches->items[i].phrase,
                                       rk_matches->items[i].doc1_pos,
                                       rk_matches->items[i].doc2_pos,
                                       rk_matches->items[i].word_count,
                                       "Rabin-Karp");
            }
        }
    }

    if (kmp_matches) phrase_match_array_free(kmp_matches);
    if (rk_matches) phrase_match_array_free(rk_matches);

    return merged;
}

char *report_highlight_tokens(const TokenArray *tokens, const PhraseMatchArray *phrases, int is_doc1) {
    if (tokens == NULL || tokens->count == 0) {
        char *empty = (char *)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    char *in_match = (char *)calloc(tokens->count, sizeof(char));
    if (in_match == NULL) {
        return NULL;
    }

    if (phrases != NULL) {
        for (size_t i = 0; i < phrases->count; i++) {
            size_t pos = is_doc1 ? phrases->items[i].doc1_pos : phrases->items[i].doc2_pos;
            size_t wc = phrases->items[i].word_count;
            for (size_t k = 0; k < wc && (pos + k) < tokens->count; k++) {
                in_match[pos + k] = 1;
            }
        }
    }

    /* Estimate buffer size */
    size_t est_size = 0;
    for (size_t i = 0; i < tokens->count; i++) {
        est_size += strlen(tokens->items[i].word) + 32;
    }

    char *out = (char *)malloc(est_size + 64);
    if (out == NULL) {
        free(in_match);
        return NULL;
    }
    out[0] = '\0';

    int inside_match = 0;

    for (size_t i = 0; i < tokens->count; i++) {
        if (in_match[i] && !inside_match) {
            strcat(out, ">>> MATCH <<< ");
            inside_match = 1;
        } else if (!in_match[i] && inside_match) {
            strcat(out, ">>> END MATCH <<< ");
            inside_match = 0;
        }

        strcat(out, tokens->items[i].word);
        strcat(out, " ");
    }

    if (inside_match) {
        strcat(out, ">>> END MATCH <<<");
    }

    free(in_match);
    return out;
}

PlagiarismReport *report_generate(const Document *ref_doc,
                                  const TokenArray *ref_tokens,
                                  const SentenceArray *ref_sentences,
                                  const HashTable *ref_ht,
                                  const Document *comp_doc,
                                  const TokenArray *comp_tokens,
                                  const SentenceArray *comp_sentences,
                                  const HashTable *comp_ht) {
    PlagiarismReport *rep = (PlagiarismReport *)malloc(sizeof(PlagiarismReport));
    if (rep == NULL) {
        return NULL;
    }
    memset(rep, 0, sizeof(PlagiarismReport));

    if (ref_doc && ref_doc->filepath) {
        size_t l = strlen(ref_doc->filepath);
        rep->ref_path = (char *)malloc(l + 1);
        if (rep->ref_path) memcpy(rep->ref_path, ref_doc->filepath, l + 1);
    }
    if (comp_doc && comp_doc->filepath) {
        size_t l = strlen(comp_doc->filepath);
        rep->comp_path = (char *)malloc(l + 1);
        if (rep->comp_path) memcpy(rep->comp_path, comp_doc->filepath, l + 1);
    }

    /* Check identical content */
    if (ref_doc && comp_doc && ref_doc->text && comp_doc->text &&
        ref_doc->length == comp_doc->length &&
        strcmp(ref_doc->text, comp_doc->text) == 0) {
        rep->is_identical = 1;
    } else {
        rep->is_identical = 0;
    }

    /* Deduplicated phrase extraction */
    rep->phrases = find_and_deduplicate_phrases(ref_tokens, comp_tokens, DEFAULT_MIN_PHRASE_WORDS);

    /* Multi-metric similarity calculation */
    rep->sim_result = similarity_analyze(ref_doc, ref_tokens, ref_sentences, ref_ht,
                                         comp_doc, comp_tokens, comp_sentences, comp_ht,
                                         rep->phrases);

    /* Generate highlighted text */
    rep->highlighted_ref_text = report_highlight_tokens(ref_tokens, rep->phrases, 1);
    rep->highlighted_comp_text = report_highlight_tokens(comp_tokens, rep->phrases, 0);

    return rep;
}

static void print_report_content(FILE *stream, const PlagiarismReport *rep) {
    if (stream == NULL || rep == NULL) {
        return;
    }

    fprintf(stream, "============================================================\n");
    fprintf(stream, "PLAGIARISM DETECTION REPORT\n");
    fprintf(stream, "============================================================\n\n");

    fprintf(stream, "Reference Document:\n%s\n\n", search_get_basename(rep->ref_path ? rep->ref_path : "Unknown"));
    fprintf(stream, "Compared Document:\n%s\n\n", search_get_basename(rep->comp_path ? rep->comp_path : "Unknown"));

    fprintf(stream, "---\n\n");
    fprintf(stream, "## OVERALL RESULT\n\n");
    fprintf(stream, "Similarity Score: %6.2f%%\n\n", rep->sim_result.overall_similarity);
    fprintf(stream, "Similarity Level: %s\n\n", similarity_get_level(rep->sim_result.overall_similarity));
    if (rep->is_identical) {
        fprintf(stream, "Identical Content: YES\n\n");
    }

    fprintf(stream, "---\n\n");
    fprintf(stream, "## ANALYSIS BREAKDOWN\n\n");
    fprintf(stream, "Word Similarity:       %6.2f%%\n", rep->sim_result.word_similarity);
    fprintf(stream, "Sentence Similarity:   %6.2f%%\n", rep->sim_result.sentence_similarity);
    fprintf(stream, "Phrase Similarity:     %6.2f%%\n\n", rep->sim_result.phrase_similarity);

    fprintf(stream, "---\n\n");
    fprintf(stream, "## DOCUMENT STATISTICS\n\n");
    fprintf(stream, "Reference Words:       %zu\n", rep->sim_result.total_words_doc1);
    fprintf(stream, "Compared Words:        %zu\n\n", rep->sim_result.total_words_doc2);
    fprintf(stream, "Reference Sentences:   %zu\n", rep->sim_result.total_sentences_doc1);
    fprintf(stream, "Compared Sentences:    %zu\n\n", rep->sim_result.total_sentences_doc2);
    fprintf(stream, "Common Unique Words:   %zu\n", rep->sim_result.common_words);
    fprintf(stream, "Matching Sentences:    %zu\n", rep->sim_result.matching_sentences);
    fprintf(stream, "Matching Phrases:      %zu\n", rep->sim_result.matching_phrase_count);
    fprintf(stream, "Matching Phrase Words: %zu\n\n", rep->sim_result.matching_phrase_words);

    fprintf(stream, "---\n\n");
    fprintf(stream, "## MATCHING PHRASES\n\n");

    if (rep->phrases != NULL && rep->phrases->count > 0) {
        for (size_t i = 0; i < rep->phrases->count; i++) {
            fprintf(stream, "Match #%zu\n\n", i + 1);
            fprintf(stream, "Phrase:\n%s\n\n", rep->phrases->items[i].phrase);
            fprintf(stream, "Reference Position: %zu\n", rep->phrases->items[i].doc1_pos);
            fprintf(stream, "Compared Position:  %zu\n", rep->phrases->items[i].doc2_pos);
            fprintf(stream, "Words Matched:      %zu\n", rep->phrases->items[i].word_count);
            fprintf(stream, "Algorithm:          %s\n\n", rep->phrases->items[i].algorithm);
            if (i + 1 < rep->phrases->count) {
                fprintf(stream, "---\n\n");
            }
        }
    } else {
        fprintf(stream, "No matching phrases found (minimum threshold: %d words).\n\n", DEFAULT_MIN_PHRASE_WORDS);
    }

    fprintf(stream, "---\n\n");
    fprintf(stream, "## MATCHING CONTENT HIGHLIGHTING\n\n");
    fprintf(stream, "Reference Document:\n%s\n\n", rep->highlighted_ref_text ? rep->highlighted_ref_text : "");
    fprintf(stream, "Compared Document:\n%s\n\n", rep->highlighted_comp_text ? rep->highlighted_comp_text : "");

    fprintf(stream, "---\n\n");
    fprintf(stream, "## CONCLUSION\n\n");
    if (rep->sim_result.overall_similarity >= 60.0) {
        fprintf(stream, "The compared document shows a high degree of textual\n");
        fprintf(stream, "similarity with the reference document.\n\n");
    } else if (rep->sim_result.overall_similarity >= 40.0) {
        fprintf(stream, "The compared document shows a moderate degree of textual\n");
        fprintf(stream, "overlap with the reference document.\n\n");
    } else {
        fprintf(stream, "The compared document shows little to no significant\n");
        fprintf(stream, "textual similarity with the reference document.\n\n");
    }

    fprintf(stream, "Note:\n");
    fprintf(stream, "Similarity percentage is an analytical indicator and does\n");
    fprintf(stream, "not independently prove plagiarism. Human review is recommended.\n\n");
    fprintf(stream, "============================================================\n");
}

void report_display(const PlagiarismReport *report) {
    print_report_content(stdout, report);
}

int report_save_to_file(const PlagiarismReport *report, const char *output_path, int allow_overwrite) {
    if (report == NULL || output_path == NULL || output_path[0] == '\0') {
        return 0;
    }

    /* Ensure parent folder exists */
    MAKE_DIR("reports");

    /* Check if file already exists */
    FILE *check = fopen(output_path, "r");
    if (check != NULL) {
        fclose(check);
        if (!allow_overwrite) {
            return -1; /* File exists, overwrite disallowed */
        }
    }

    FILE *f = fopen(output_path, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: Could not create report file '%s'.\n", output_path);
        return 0;
    }

    print_report_content(f, report);
    fclose(f);
    return 1;
}

void report_free(PlagiarismReport *report) {
    if (report == NULL) {
        return;
    }

    if (report->ref_path != NULL) {
        free(report->ref_path);
        report->ref_path = NULL;
    }
    if (report->comp_path != NULL) {
        free(report->comp_path);
        report->comp_path = NULL;
    }
    if (report->phrases != NULL) {
        phrase_match_array_free(report->phrases);
        report->phrases = NULL;
    }
    if (report->highlighted_ref_text != NULL) {
        free(report->highlighted_ref_text);
        report->highlighted_ref_text = NULL;
    }
    if (report->highlighted_comp_text != NULL) {
        free(report->highlighted_comp_text);
        report->highlighted_comp_text = NULL;
    }

    free(report);
}
