#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "mergesort.h"

/**
 * Extracts filename or basename from a file path.
 *
 * @param path Full file path.
 * @return Pointer within path string representing the base filename.
 */
const char *search_get_basename(const char *path);

/**
 * Searches for a document in the ranked results by exact filepath or base filename.
 * Case-insensitive search.
 *
 * @param arr Pointer to RankedDocumentArray.
 * @param query Filename or full path to search for.
 * @param out_rank Pointer to store 1-based rank if found.
 * @return Pointer to matching RankedDocument, or NULL if not found.
 */
const RankedDocument *search_ranked_by_filename(const RankedDocumentArray *arr, const char *query, size_t *out_rank);

/**
 * Displays all documents in the ranked results with similarity >= min_similarity.
 *
 * @param arr Pointer to RankedDocumentArray.
 * @param min_similarity Minimum threshold percentage.
 * @return Number of documents meeting or exceeding the threshold.
 */
size_t search_ranked_by_threshold(const RankedDocumentArray *arr, double min_similarity);

#endif /* SEARCH_H */
