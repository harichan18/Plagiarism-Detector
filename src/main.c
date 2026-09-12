#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "document.h"
#include "preprocessing.h"
#include "hashtable.h"
#include "kmp.h"
#include "rabinkarp.h"
#include "similarity.h"
#include "mergesort.h"
#include "search.h"
#include "report.h"

#define MAX_PATH_LENGTH 1024
#define MAX_PATTERN_LENGTH 512
#define TOP_WORDS_LIMIT 5

typedef struct LoadedDoc {
    Document *doc;
    char *clean_text;
    TokenArray *tokens;
    SentenceArray *sentences;
    HashTable *ht;
} LoadedDoc;

static PlagiarismReport *g_last_report = NULL;

static void trim_newline(char *str) {
    if (str == NULL) {
        return;
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static void str_to_lower(char *str) {
    if (str == NULL) {
        return;
    }
    for (size_t i = 0; str[i] != '\0'; i++) {
        str[i] = (char)tolower((unsigned char)str[i]);
    }
}

static void free_loaded_doc(LoadedDoc *ld) {
    if (ld == NULL) {
        return;
    }
    if (ld->ht != NULL) {
        hash_table_free(ld->ht);
        ld->ht = NULL;
    }
    if (ld->sentences != NULL) {
        sentence_array_free(ld->sentences);
        ld->sentences = NULL;
    }
    if (ld->tokens != NULL) {
        token_array_free(ld->tokens);
        ld->tokens = NULL;
    }
    if (ld->clean_text != NULL) {
        free(ld->clean_text);
        ld->clean_text = NULL;
    }
    if (ld->doc != NULL) {
        document_free(ld->doc);
        ld->doc = NULL;
    }
}

static int load_and_preprocess(const char *filepath, LoadedDoc *ld) {
    if (filepath == NULL || ld == NULL) {
        return 0;
    }

    Document *doc = document_read(filepath);
    if (doc == NULL) {
        return 0;
    }

    char *clean_text = preprocess_text(doc->text);
    if (clean_text == NULL) {
        document_free(doc);
        return 0;
    }

    TokenArray *tokens = preprocess_tokenize(clean_text);
    if (tokens == NULL) {
        free(clean_text);
        document_free(doc);
        return 0;
    }

    SentenceArray *sentences = preprocess_extract_sentences(clean_text);
    if (sentences == NULL) {
        token_array_free(tokens);
        free(clean_text);
        document_free(doc);
        return 0;
    }

    doc->word_count = tokens->count;
    doc->sentence_count = sentences->count;

    HashTable *ht = hash_table_build_from_tokens(tokens, 0);
    if (ht == NULL) {
        sentence_array_free(sentences);
        token_array_free(tokens);
        free(clean_text);
        document_free(doc);
        return 0;
    }

    ld->doc = doc;
    ld->clean_text = clean_text;
    ld->tokens = tokens;
    ld->sentences = sentences;
    ld->ht = ht;

    return 1;
}

static void show_frequency_analysis(const LoadedDoc *ld) {
    printf("\n--- Document Statistics ---\n\n");
    printf("Total Words: %zu\n", ld->doc->word_count);
    printf("Unique Words: %zu\n\n", ld->ht->unique_count);

    const HashEntry *top_entries[TOP_WORDS_LIMIT];
    size_t top_count = 0;
    hash_table_get_top_frequent(ld->ht, top_entries, TOP_WORDS_LIMIT, &top_count);

    if (top_count > 0) {
        printf("Most Frequent Words:\n");
        for (size_t i = 0; i < top_count; i++) {
            printf("%zu. %s -> %d\n", i + 1, top_entries[i]->word, top_entries[i]->frequency);
        }
        printf("\n");
    }

    printf("Word Frequencies:\n");
    hash_table_display(ld->ht);
    printf("\n");
}

static void run_kmp_search(const LoadedDoc *ld) {
    char pattern[MAX_PATTERN_LENGTH];
    printf("\nEnter phrase to search using KMP:\n");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        return;
    }
    trim_newline(pattern);
    if (pattern[0] == '\0') {
        printf("Empty pattern entered.\n");
        return;
    }

    char norm_pattern[MAX_PATTERN_LENGTH];
    strncpy(norm_pattern, pattern, sizeof(norm_pattern) - 1);
    norm_pattern[sizeof(norm_pattern) - 1] = '\0';
    str_to_lower(norm_pattern);

    printf("\n## KMP SEARCH\n\n");
    printf("Pattern:\n%s\n\n", pattern);

    MatchArray *matches = kmp_search(ld->clean_text, norm_pattern);
    if (matches != NULL && matches->count > 0) {
        printf("Pattern found.\n");
        printf("Occurrences: %zu\n", matches->count);
        for (size_t i = 0; i < matches->count; i++) {
            printf("Position: %zu\n", matches->positions[i]);
        }
        printf("\n");
    } else {
        printf("Pattern not found.\n\n");
    }

    if (matches != NULL) {
        match_array_free(matches);
    }
}

static void run_rabinkarp_search(const LoadedDoc *ld) {
    char pattern[MAX_PATTERN_LENGTH];
    printf("\nEnter phrase to search using Rabin-Karp:\n");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        return;
    }
    trim_newline(pattern);
    if (pattern[0] == '\0') {
        printf("Empty pattern entered.\n");
        return;
    }

    char norm_pattern[MAX_PATTERN_LENGTH];
    strncpy(norm_pattern, pattern, sizeof(norm_pattern) - 1);
    norm_pattern[sizeof(norm_pattern) - 1] = '\0';
    str_to_lower(norm_pattern);

    printf("\n## RABIN-KARP SEARCH\n\n");
    printf("Pattern:\n%s\n\n", pattern);

    MatchArray *matches = rabin_karp_search(ld->clean_text, norm_pattern);
    if (matches != NULL && matches->count > 0) {
        printf("Pattern found.\n");
        printf("Occurrences: %zu\n", matches->count);
        for (size_t i = 0; i < matches->count; i++) {
            printf("Position: %zu\n", matches->positions[i]);
        }
        printf("\n");
    } else {
        printf("Pattern not found.\n\n");
    }

    if (matches != NULL) {
        match_array_free(matches);
    }
}

static void prompt_save_report(const PlagiarismReport *rep) {
    if (rep == NULL) return;

    char ans[64];
    printf("Do you want to save this report to a file? (y/n): ");
    if (fgets(ans, sizeof(ans), stdin) == NULL) return;
    trim_newline(ans);

    if (ans[0] == 'y' || ans[0] == 'Y') {
        char default_path[MAX_PATH_LENGTH];
        const char *base = search_get_basename(rep->comp_path ? rep->comp_path : "document");

        /* Strip extension if present */
        char clean_name[256];
        strncpy(clean_name, base, sizeof(clean_name) - 1);
        clean_name[sizeof(clean_name) - 1] = '\0';
        char *dot = strrchr(clean_name, '.');
        if (dot) *dot = '\0';

        snprintf(default_path, sizeof(default_path), "reports/%s_report.txt", clean_name);

        int res = report_save_to_file(rep, default_path, 0);
        if (res == -1) {
            printf("Report '%s' already exists. Overwrite? (y/n): ", default_path);
            if (fgets(ans, sizeof(ans), stdin) != NULL) {
                trim_newline(ans);
                if (ans[0] == 'y' || ans[0] == 'Y') {
                    if (report_save_to_file(rep, default_path, 1) == 1) {
                        printf("Report saved successfully to '%s'.\n", default_path);
                    }
                } else {
                    printf("Report export cancelled.\n");
                }
            }
        } else if (res == 1) {
            printf("Report saved successfully to '%s'.\n", default_path);
        }
    }
}

static void compare_two_documents(void) {
    char path1[MAX_PATH_LENGTH];
    char path2[MAX_PATH_LENGTH];

    printf("\nEnter Document 1 path:\n");
    if (fgets(path1, sizeof(path1), stdin) == NULL) return;
    trim_newline(path1);

    printf("Enter Document 2 path:\n");
    if (fgets(path2, sizeof(path2), stdin) == NULL) return;
    trim_newline(path2);

    LoadedDoc ld1 = {NULL, NULL, NULL, NULL, NULL};
    LoadedDoc ld2 = {NULL, NULL, NULL, NULL, NULL};

    if (!load_and_preprocess(path1, &ld1)) {
        return;
    }
    if (!load_and_preprocess(path2, &ld2)) {
        free_loaded_doc(&ld1);
        return;
    }

    /* Generate Phase 6 PlagiarismReport */
    if (g_last_report != NULL) {
        report_free(g_last_report);
        g_last_report = NULL;
    }

    g_last_report = report_generate(ld1.doc, ld1.tokens, ld1.sentences, ld1.ht,
                                    ld2.doc, ld2.tokens, ld2.sentences, ld2.ht);

    if (g_last_report != NULL) {
        report_display(g_last_report);
        prompt_save_report(g_last_report);
    }

    free_loaded_doc(&ld1);
    free_loaded_doc(&ld2);
}

static void run_ranked_results_search(const RankedDocumentArray *ranked_arr, const LoadedDoc *ld_ref) {
    char input_buf[128];
    while (1) {
        printf("\nRanked Results Search Options:\n");
        printf("1. Search by document name\n");
        printf("2. Search by similarity threshold\n");
        printf("3. Generate detailed report for a ranked document\n");
        printf("4. Return to Main Menu\n\n");
        printf("Select an option (1-4): ");

        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            break;
        }
        trim_newline(input_buf);
        int sub_choice = atoi(input_buf);

        if (sub_choice == 1) {
            char search_name[MAX_PATH_LENGTH];
            printf("\nEnter document name to search:\n");
            if (fgets(search_name, sizeof(search_name), stdin) == NULL) continue;
            trim_newline(search_name);

            size_t found_rank = 0;
            const RankedDocument *found = search_ranked_by_filename(ranked_arr, search_name, &found_rank);

            if (found != NULL) {
                printf("\nDocument found.\n\n");
                printf("Rank: %zu\n", found_rank);
                printf("Similarity: %.2f%%\n", found->similarity_percentage);
            } else {
                printf("\nDocument not found in comparison results.\n");
            }
        } else if (sub_choice == 2) {
            char thresh_buf[64];
            printf("\nEnter minimum similarity percentage:\n");
            if (fgets(thresh_buf, sizeof(thresh_buf), stdin) == NULL) continue;
            trim_newline(thresh_buf);

            double thresh = atof(thresh_buf);
            search_ranked_by_threshold(ranked_arr, thresh);
        } else if (sub_choice == 3) {
            char rank_buf[64];
            printf("\nEnter rank for detailed report: ");
            if (fgets(rank_buf, sizeof(rank_buf), stdin) == NULL) continue;
            trim_newline(rank_buf);
            size_t chosen_rank = (size_t)atoi(rank_buf);

            if (chosen_rank == 0 || chosen_rank > ranked_arr->count) {
                printf("Invalid rank specified.\n");
                continue;
            }

            const char *target_path = ranked_arr->items[chosen_rank - 1].filepath;
            LoadedDoc ld_target = {NULL, NULL, NULL, NULL, NULL};
            if (load_and_preprocess(target_path, &ld_target)) {
                if (g_last_report != NULL) {
                    report_free(g_last_report);
                    g_last_report = NULL;
                }
                g_last_report = report_generate(ld_ref->doc, ld_ref->tokens, ld_ref->sentences, ld_ref->ht,
                                                ld_target.doc, ld_target.tokens, ld_target.sentences, ld_target.ht);
                if (g_last_report != NULL) {
                    report_display(g_last_report);
                    prompt_save_report(g_last_report);
                }
                free_loaded_doc(&ld_target);
            }
        } else if (sub_choice == 4) {
            break;
        } else {
            printf("Invalid selection.\n");
        }
    }
}

static void compare_against_multiple_documents(void) {
    char ref_path[MAX_PATH_LENGTH];
    printf("\nEnter reference document:\n");
    if (fgets(ref_path, sizeof(ref_path), stdin) == NULL) return;
    trim_newline(ref_path);

    if (ref_path[0] == '\0') {
        printf("Error: Empty reference document path.\n");
        return;
    }

    LoadedDoc ld_ref = {NULL, NULL, NULL, NULL, NULL};
    if (!load_and_preprocess(ref_path, &ld_ref)) {
        return;
    }

    char num_buf[64];
    printf("\nHow many documents do you want to compare?\n");
    if (fgets(num_buf, sizeof(num_buf), stdin) == NULL) {
        free_loaded_doc(&ld_ref);
        return;
    }
    trim_newline(num_buf);
    int num_docs = atoi(num_buf);

    if (num_docs <= 0) {
        printf("No documents specified for comparison.\n");
        free_loaded_doc(&ld_ref);
        return;
    }

    RankedDocumentArray *ranked_arr = ranked_document_array_create((size_t)num_docs);
    if (ranked_arr == NULL) {
        free_loaded_doc(&ld_ref);
        return;
    }

    printf("\n");
    for (int i = 0; i < num_docs; i++) {
        char doc_path[MAX_PATH_LENGTH];
        printf("Document %d:\n", i + 1);
        if (fgets(doc_path, sizeof(doc_path), stdin) == NULL) break;
        trim_newline(doc_path);

        if (doc_path[0] == '\0') {
            printf("Warning: Skipping empty document path.\n");
            continue;
        }

        LoadedDoc ld_comp = {NULL, NULL, NULL, NULL, NULL};
        if (!load_and_preprocess(doc_path, &ld_comp)) {
            printf("Warning: Skipping inaccessible document '%s'.\n", doc_path);
            continue;
        }

        PhraseMatchArray *matches = find_matching_phrases_kmp(ld_ref.tokens, ld_comp.tokens, DEFAULT_MIN_PHRASE_WORDS);
        SimilarityResult sim = similarity_analyze(ld_ref.doc, ld_ref.tokens, ld_ref.sentences, ld_ref.ht,
                                                 ld_comp.doc, ld_comp.tokens, ld_comp.sentences, ld_comp.ht,
                                                 matches);

        ranked_document_array_add(ranked_arr, doc_path, sim.overall_similarity);

        if (matches != NULL) {
            phrase_match_array_free(matches);
        }
        free_loaded_doc(&ld_comp);
    }

    if (ranked_arr->count == 0) {
        printf("\nNo valid documents were compared.\n");
        ranked_document_array_free(ranked_arr);
        free_loaded_doc(&ld_ref);
        return;
    }

    ranked_document_sort_and_assign_ranks(ranked_arr);

    printf("\n========================================\n");
    printf("SIMILARITY RANKING\n");
    printf("========================================\n\n");
    printf("Reference Document:\n%s\n\n", search_get_basename(ref_path));
    printf("---\n\n");
    printf("## Rank    Document                  Score\n\n");

    for (size_t i = 0; i < ranked_arr->count; i++) {
        printf("%-7zu %-25s %6.2f%%\n",
               ranked_arr->items[i].rank,
               search_get_basename(ranked_arr->items[i].filepath),
               ranked_arr->items[i].similarity_percentage);
    }

    printf("\n---\n\n");
    printf("Highest Similarity:\n%s\n\n", search_get_basename(ranked_arr->items[0].filepath));
    printf("Similarity:\n%.2f%%\n\n", ranked_arr->items[0].similarity_percentage);

    size_t last_idx = ranked_arr->count - 1;
    printf("Lowest Similarity:\n%s\n\n", search_get_basename(ranked_arr->items[last_idx].filepath));
    printf("Similarity:\n%.2f%%\n\n", ranked_arr->items[last_idx].similarity_percentage);

    size_t cat_very_high = 0;
    size_t cat_high = 0;
    size_t cat_moderate = 0;
    size_t cat_low = 0;
    size_t cat_very_low = 0;
    double sum_sim = 0.0;

    for (size_t i = 0; i < ranked_arr->count; i++) {
        double score = ranked_arr->items[i].similarity_percentage;
        sum_sim += score;

        if (score >= 80.0) cat_very_high++;
        else if (score >= 60.0) cat_high++;
        else if (score >= 40.0) cat_moderate++;
        else if (score >= 20.0) cat_low++;
        else cat_very_low++;
    }

    double avg_sim = sum_sim / (double)ranked_arr->count;

    printf("========================================\n");
    printf("PLAGIARISM STATISTICS\n");
    printf("========================================\n\n");
    printf("Total Documents Compared: %zu\n\n", ranked_arr->count);
    printf("Very High Similarity (80-100%%): %zu\n", cat_very_high);
    printf("High Similarity (60-79%%): %zu\n", cat_high);
    printf("Moderate Similarity (40-59%%): %zu\n", cat_moderate);
    printf("Low Similarity (20-39%%): %zu\n", cat_low);
    printf("Very Low Similarity (0-19%%): %zu\n\n", cat_very_low);
    printf("Average Similarity: %.2f%%\n\n", avg_sim);
    printf("Highest Similarity: %.2f%%\n", ranked_arr->items[0].similarity_percentage);
    printf("Lowest Similarity: %.2f%%\n\n", ranked_arr->items[last_idx].similarity_percentage);

    run_ranked_results_search(ranked_arr, &ld_ref);

    ranked_document_array_free(ranked_arr);
    free_loaded_doc(&ld_ref);
}

static void show_last_detailed_report(void) {
    if (g_last_report == NULL) {
        printf("\nNo comparison available.\nPlease compare two documents first.\n");
        return;
    }

    printf("\nRe-displaying Latest Plagiarism Report:\n\n");
    report_display(g_last_report);
    prompt_save_report(g_last_report);
}

static void print_json_escaped(FILE *f, const char *str) {
    if (str == NULL) {
        fputs("\"\"", f);
        return;
    }
    fputc('"', f);
    for (const char *p = str; *p != '\0'; p++) {
        switch (*p) {
            case '\\': fputs("\\\\", f); break;
            case '"':  fputs("\\\"", f); break;
            case '\b': fputs("\\b", f); break;
            case '\f': fputs("\\f", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if ((unsigned char)*p < 0x20) {
                    fprintf(f, "\\u%04x", (unsigned char)*p);
                } else {
                    fputc(*p, f);
                }
                break;
        }
    }
    fputc('"', f);
}

static int run_cli_doc_info(const char *path) {
    LoadedDoc ld = {NULL, NULL, NULL, NULL, NULL};
    if (!load_and_preprocess(path, &ld)) {
        printf("{\"success\":false,\"error\":\"Failed to load or preprocess document\"}\n");
        return 1;
    }

    const HashEntry *top_entries[10];
    size_t top_count = 0;
    hash_table_get_top_frequent(ld.ht, top_entries, 10, &top_count);

    printf("{\"success\":true,\"filepath\":");
    print_json_escaped(stdout, path);
    printf(",\"filename\":");
    print_json_escaped(stdout, search_get_basename(path));
    printf(",\"file_size\":%zu,\"word_count\":%zu,\"sentence_count\":%zu,\"unique_words\":%zu,\"top_words\":[",
           ld.doc->length, ld.doc->word_count, ld.doc->sentence_count, ld.ht->unique_count);

    for (size_t i = 0; i < top_count; i++) {
        if (i > 0) printf(",");
        printf("{\"word\":");
        print_json_escaped(stdout, top_entries[i]->word);
        printf(",\"frequency\":%d}", top_entries[i]->frequency);
    }
    printf("]}\n");

    free_loaded_doc(&ld);
    return 0;
}

static int run_cli_compare_json(const char *path1, const char *path2) {
    LoadedDoc ld1 = {NULL, NULL, NULL, NULL, NULL};
    LoadedDoc ld2 = {NULL, NULL, NULL, NULL, NULL};

    if (!load_and_preprocess(path1, &ld1)) {
        printf("{\"success\":false,\"error\":\"Failed to load reference document\"}\n");
        return 1;
    }
    if (!load_and_preprocess(path2, &ld2)) {
        free_loaded_doc(&ld1);
        printf("{\"success\":false,\"error\":\"Failed to load comparison document\"}\n");
        return 1;
    }

    PlagiarismReport *rep = report_generate(ld1.doc, ld1.tokens, ld1.sentences, ld1.ht,
                                            ld2.doc, ld2.tokens, ld2.sentences, ld2.ht);
    if (rep == NULL) {
        free_loaded_doc(&ld1);
        free_loaded_doc(&ld2);
        printf("{\"success\":false,\"error\":\"Failed to generate similarity report\"}\n");
        return 1;
    }

    printf("{\"success\":true,\"reference_file\":");
    print_json_escaped(stdout, path1);
    printf(",\"compared_file\":");
    print_json_escaped(stdout, path2);
    printf(",\"reference_name\":");
    print_json_escaped(stdout, search_get_basename(path1));
    printf(",\"compared_name\":");
    print_json_escaped(stdout, search_get_basename(path2));
    printf(",\"is_identical\":%d", rep->is_identical);
    printf(",\"similarity_score\":%.2f", rep->sim_result.overall_similarity);
    printf(",\"similarity_level\":");
    print_json_escaped(stdout, similarity_get_level(rep->sim_result.overall_similarity));
    printf(",\"word_similarity\":%.2f", rep->sim_result.word_similarity);
    printf(",\"sentence_similarity\":%.2f", rep->sim_result.sentence_similarity);
    printf(",\"phrase_similarity\":%.2f", rep->sim_result.phrase_similarity);
    printf(",\"reference_words\":%zu", rep->sim_result.total_words_doc1);
    printf(",\"compared_words\":%zu", rep->sim_result.total_words_doc2);
    printf(",\"reference_sentences\":%zu", rep->sim_result.total_sentences_doc1);
    printf(",\"compared_sentences\":%zu", rep->sim_result.total_sentences_doc2);
    printf(",\"common_words\":%zu", rep->sim_result.common_words);
    printf(",\"matching_sentences\":%zu", rep->sim_result.matching_sentences);
    printf(",\"matching_phrase_count\":%zu", rep->sim_result.matching_phrase_count);
    printf(",\"matching_phrase_words\":%zu", rep->sim_result.matching_phrase_words);

    printf(",\"phrases\":[");
    if (rep->phrases != NULL) {
        for (size_t i = 0; i < rep->phrases->count; i++) {
            if (i > 0) printf(",");
            printf("{\"phrase\":");
            print_json_escaped(stdout, rep->phrases->items[i].phrase);
            printf(",\"doc1_pos\":%zu,\"doc2_pos\":%zu,\"word_count\":%zu,\"algorithm\":",
                   rep->phrases->items[i].doc1_pos, rep->phrases->items[i].doc2_pos, rep->phrases->items[i].word_count);
            print_json_escaped(stdout, rep->phrases->items[i].algorithm);
            printf("}");
        }
    }
    printf("],\"highlighted_ref_text\":");
    print_json_escaped(stdout, rep->highlighted_ref_text);
    printf(",\"highlighted_comp_text\":");
    print_json_escaped(stdout, rep->highlighted_comp_text);
    printf(",\"raw_ref_text\":");
    print_json_escaped(stdout, ld1.doc->text);
    printf(",\"raw_comp_text\":");
    print_json_escaped(stdout, ld2.doc->text);
    printf("}\n");

    report_free(rep);
    free_loaded_doc(&ld1);
    free_loaded_doc(&ld2);
    return 0;
}

static int run_cli_batch_json(const char *ref_path, int count, char *cand_paths[]) {
    if (count <= 0) {
        printf("{\"success\":false,\"error\":\"No candidate documents provided\"}\n");
        return 1;
    }

    LoadedDoc ld_ref = {NULL, NULL, NULL, NULL, NULL};
    if (!load_and_preprocess(ref_path, &ld_ref)) {
        printf("{\"success\":false,\"error\":\"Failed to load reference document\"}\n");
        return 1;
    }

    RankedDocumentArray *ranked_arr = ranked_document_array_create((size_t)count);
    if (ranked_arr == NULL) {
        free_loaded_doc(&ld_ref);
        printf("{\"success\":false,\"error\":\"Memory allocation failed for ranking array\"}\n");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        LoadedDoc ld_cand = {NULL, NULL, NULL, NULL, NULL};
        if (!load_and_preprocess(cand_paths[i], &ld_cand)) {
            continue;
        }

        PhraseMatchArray *phrases = find_and_deduplicate_phrases(ld_ref.tokens, ld_cand.tokens, DEFAULT_MIN_PHRASE_WORDS);
        SimilarityResult res = similarity_analyze(ld_ref.doc, ld_ref.tokens, ld_ref.sentences, ld_ref.ht,
                                                  ld_cand.doc, ld_cand.tokens, ld_cand.sentences, ld_cand.ht,
                                                  phrases);
        phrase_match_array_free(phrases);
        ranked_document_array_add(ranked_arr, cand_paths[i], res.overall_similarity);
        free_loaded_doc(&ld_cand);
    }

    if (ranked_arr->count == 0) {
        ranked_document_array_free(ranked_arr);
        free_loaded_doc(&ld_ref);
        printf("{\"success\":false,\"error\":\"No valid candidate documents could be processed\"}\n");
        return 1;
    }

    ranked_document_sort_and_assign_ranks(ranked_arr);

    size_t cat_very_high = 0, cat_high = 0, cat_moderate = 0, cat_low = 0, cat_very_low = 0;
    double sum_sim = 0.0;

    for (size_t i = 0; i < ranked_arr->count; i++) {
        double s = ranked_arr->items[i].similarity_percentage;
        sum_sim += s;
        if (s >= 80.0) cat_very_high++;
        else if (s >= 60.0) cat_high++;
        else if (s >= 40.0) cat_moderate++;
        else if (s >= 20.0) cat_low++;
        else cat_very_low++;
    }

    double avg_sim = sum_sim / (double)ranked_arr->count;
    double max_sim = ranked_arr->items[0].similarity_percentage;
    double min_sim = ranked_arr->items[ranked_arr->count - 1].similarity_percentage;

    printf("{\"success\":true,\"reference_file\":");
    print_json_escaped(stdout, ref_path);
    printf(",\"reference_name\":");
    print_json_escaped(stdout, search_get_basename(ref_path));
    printf(",\"total_compared\":%zu,\"rankings\":[", ranked_arr->count);

    for (size_t i = 0; i < ranked_arr->count; i++) {
        if (i > 0) printf(",");
        printf("{\"rank\":%zu,\"document\":", ranked_arr->items[i].rank);
        print_json_escaped(stdout, search_get_basename(ranked_arr->items[i].filepath));
        printf(",\"filepath\":");
        print_json_escaped(stdout, ranked_arr->items[i].filepath);
        printf(",\"similarity\":%.2f,\"level\":", ranked_arr->items[i].similarity_percentage);
        print_json_escaped(stdout, similarity_get_level(ranked_arr->items[i].similarity_percentage));
        printf("}");
    }

    printf("],\"statistics\":{\"total\":%zu,\"very_high_count\":%zu,\"high_count\":%zu,\"moderate_count\":%zu,\"low_count\":%zu,\"very_low_count\":%zu,\"average_similarity\":%.2f,\"max_similarity\":%.2f,\"min_similarity\":%.2f}}\n",
           ranked_arr->count, cat_very_high, cat_high, cat_moderate, cat_low, cat_very_low, avg_sim, max_sim, min_sim);

    ranked_document_array_free(ranked_arr);
    free_loaded_doc(&ld_ref);
    return 0;
}

static int run_cli_export_report(const char *path1, const char *path2, const char *out_path) {
    LoadedDoc ld1 = {NULL, NULL, NULL, NULL, NULL};
    LoadedDoc ld2 = {NULL, NULL, NULL, NULL, NULL};

    if (!load_and_preprocess(path1, &ld1) || !load_and_preprocess(path2, &ld2)) {
        free_loaded_doc(&ld1);
        free_loaded_doc(&ld2);
        printf("{\"success\":false,\"error\":\"Failed to load documents\"}\n");
        return 1;
    }

    PlagiarismReport *rep = report_generate(ld1.doc, ld1.tokens, ld1.sentences, ld1.ht,
                                            ld2.doc, ld2.tokens, ld2.sentences, ld2.ht);
    if (rep == NULL) {
        free_loaded_doc(&ld1);
        free_loaded_doc(&ld2);
        printf("{\"success\":false,\"error\":\"Failed to generate report\"}\n");
        return 1;
    }

    int res = report_save_to_file(rep, out_path, 1);
    if (res == 1) {
        printf("{\"success\":true,\"exported_path\":");
        print_json_escaped(stdout, out_path);
        printf("}\n");
    } else {
        printf("{\"success\":false,\"error\":\"Failed to write report file\"}\n");
    }

    report_free(rep);
    free_loaded_doc(&ld1);
    free_loaded_doc(&ld2);
    return (res == 1) ? 0 : 1;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--compare-json") == 0 && argc >= 4) {
            return run_cli_compare_json(argv[2], argv[3]);
        } else if (strcmp(argv[1], "--batch-json") == 0 && argc >= 4) {
            return run_cli_batch_json(argv[2], argc - 3, &argv[3]);
        } else if (strcmp(argv[1], "--doc-info") == 0 && argc >= 3) {
            return run_cli_doc_info(argv[2]);
        } else if (strcmp(argv[1], "--export-report") == 0 && argc >= 5) {
            return run_cli_export_report(argv[2], argv[3], argv[4]);
        } else {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "  %s (interactive CLI mode)\n", argv[0]);
            fprintf(stderr, "  %s --compare-json <doc1> <doc2>\n", argv[0]);
            fprintf(stderr, "  %s --batch-json <ref_doc> <cand1> [cand2 ...]\n", argv[0]);
            fprintf(stderr, "  %s --doc-info <doc>\n", argv[0]);
            fprintf(stderr, "  %s --export-report <doc1> <doc2> <output_file>\n", argv[0]);
            return 1;
        }
    }

    LoadedDoc current_doc = {NULL, NULL, NULL, NULL, NULL};
    char input_buf[128];

    printf("============================================================\n");
    printf("PLAGIARISM DETECTION SYSTEM\n");
    printf("============================================================\n");

    while (1) {
        printf("\nMenu:\n");
        printf("1. Load Document\n");
        printf("2. Word Frequency Analysis\n");
        printf("3. KMP Phrase Search\n");
        printf("4. Rabin-Karp Phrase Search\n");
        printf("5. Compare Two Documents\n");
        printf("6. Compare Against Multiple Documents\n");
        printf("7. Generate Detailed Report\n");
        printf("8. Exit\n\n");
        printf("Select an option (1-8): ");

        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            break;
        }

        trim_newline(input_buf);
        if (input_buf[0] == '\0') {
            continue;
        }

        int choice = atoi(input_buf);

        if (choice == 1) {
            char path[MAX_PATH_LENGTH];
            printf("\nEnter document path:\n");
            if (fgets(path, sizeof(path), stdin) == NULL) continue;
            trim_newline(path);

            if (path[0] == '\0') {
                printf("Error: Empty file path.\n");
                continue;
            }

            free_loaded_doc(&current_doc);
            if (load_and_preprocess(path, &current_doc)) {
                printf("\nDocument loaded successfully.\n");
            }
        } else if (choice == 2) {
            if (current_doc.doc == NULL) {
                printf("\nNo document loaded. Please select Option 1 first.\n");
            } else {
                show_frequency_analysis(&current_doc);
            }
        } else if (choice == 3) {
            if (current_doc.doc == NULL) {
                printf("\nNo document loaded. Please select Option 1 first.\n");
            } else {
                run_kmp_search(&current_doc);
            }
        } else if (choice == 4) {
            if (current_doc.doc == NULL) {
                printf("\nNo document loaded. Please select Option 1 first.\n");
            } else {
                run_rabinkarp_search(&current_doc);
            }
        } else if (choice == 5) {
            compare_two_documents();
        } else if (choice == 6) {
            compare_against_multiple_documents();
        } else if (choice == 7) {
            show_last_detailed_report();
        } else if (choice == 8) {
            printf("\nExiting Plagiarism Detection System. Goodbye!\n");
            break;
        } else {
            printf("\nInvalid selection. Please choose an option between 1 and 8.\n");
        }
    }

    if (g_last_report != NULL) {
        report_free(g_last_report);
        g_last_report = NULL;
    }
    free_loaded_doc(&current_doc);
    return 0;
}
