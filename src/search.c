#include "search.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const char *search_get_basename(const char *path) {
    if (path == NULL) {
        return "";
    }

    const char *last_slash = path;
    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            last_slash = p + 1;
        }
    }
    return last_slash;
}

static int str_case_equal(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) {
        return 0;
    }
    while (*s1 != '\0' && *s2 != '\0') {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return 0;
        }
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

const RankedDocument *search_ranked_by_filename(const RankedDocumentArray *arr, const char *query, size_t *out_rank) {
    if (out_rank != NULL) {
        *out_rank = 0;
    }

    if (arr == NULL || query == NULL || query[0] == '\0') {
        return NULL;
    }

    const char *query_base = search_get_basename(query);

    for (size_t i = 0; i < arr->count; i++) {
        const char *doc_base = search_get_basename(arr->items[i].filepath);

        if (str_case_equal(arr->items[i].filepath, query) ||
            str_case_equal(doc_base, query) ||
            str_case_equal(doc_base, query_base)) {
            if (out_rank != NULL) {
                *out_rank = arr->items[i].rank;
            }
            return &arr->items[i];
        }
    }

    return NULL;
}

size_t search_ranked_by_threshold(const RankedDocumentArray *arr, double min_similarity) {
    if (arr == NULL || arr->count == 0) {
        printf("No comparison results available to search.\n");
        return 0;
    }

    size_t matches_count = 0;
    printf("\nDocuments with similarity >= %.2f%%:\n\n", min_similarity);

    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].similarity_percentage >= min_similarity) {
            printf("%s -> %.2f%%\n",
                   search_get_basename(arr->items[i].filepath),
                   arr->items[i].similarity_percentage);
            matches_count++;
        }
    }

    if (matches_count == 0) {
        printf("None found.\n");
    }
    printf("\n");

    return matches_count;
}
