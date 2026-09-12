#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include "document.h"
#include "preprocessing.h"
#include "hashtable.h"
#include "kmp.h"
#include "similarity.h"

/**
 * Complete plagiarism detection report containing metrics, deduplicated phrases,
 * and highlighted matching content.
 */
typedef struct PlagiarismReport {
    char *ref_path;                /* Reference document file path */
    char *comp_path;               /* Compared document file path */
    SimilarityResult sim_result;   /* Multi-metric similarity results */
    PhraseMatchArray *phrases;     /* Deduplicated matching phrases */
    int is_identical;              /* Flag indicating 100% identical documents */
    char *highlighted_ref_text;    /* Reference text with >>> MATCH <<< markers */
    char *highlighted_comp_text;   /* Compared text with >>> MATCH <<< markers */
} PlagiarismReport;

/**
 * Finds matching phrases between two documents using both KMP and Rabin-Karp,
 * merging duplicates detected by both algorithms into a single "KMP + Rabin-Karp" entry.
 */
PhraseMatchArray *find_and_deduplicate_phrases(const TokenArray *t1, const TokenArray *t2, size_t min_words);

/**
 * Generates formatted text with >>> MATCH <<< and >>> END MATCH <<< markers
 * around matching word sequences.
 */
char *report_highlight_tokens(const TokenArray *tokens, const PhraseMatchArray *phrases, int is_doc1);

/**
 * Generates a full PlagiarismReport comparing a reference document against a candidate document.
 */
PlagiarismReport *report_generate(const Document *ref_doc,
                                  const TokenArray *ref_tokens,
                                  const SentenceArray *ref_sentences,
                                  const HashTable *ref_ht,
                                  const Document *comp_doc,
                                  const TokenArray *comp_tokens,
                                  const SentenceArray *comp_sentences,
                                  const HashTable *comp_ht);

/**
 * Prints the structured plagiarism detection report to the console.
 */
void report_display(const PlagiarismReport *report);

/**
 * Saves the structured plagiarism report to a file on disk.
 * Returns 1 on success, 0 on failure, or -1 if file exists and allow_overwrite is 0.
 */
int report_save_to_file(const PlagiarismReport *report, const char *output_path, int allow_overwrite);

/**
 * Releases all memory allocated for a PlagiarismReport.
 */
void report_free(PlagiarismReport *report);

#endif /* REPORT_H */
