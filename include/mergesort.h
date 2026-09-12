#ifndef MERGESORT_H
#define MERGESORT_H

#include <stddef.h>

/**
 * Represents a document ranked by its similarity to a reference document.
 */
typedef struct RankedDocument {
    char *filepath;                  /* Path or name of the document */
    double similarity_percentage;    /* Overall similarity score (0.0 to 100.0%) */
    size_t rank;                     /* 1-based rank position after sorting */
} RankedDocument;

/**
 * Dynamically resizable array of RankedDocument entries.
 */
typedef struct RankedDocumentArray {
    RankedDocument *items;           /* Array of ranked documents */
    size_t count;                    /* Number of documents compared */
    size_t capacity;                 /* Allocated capacity */
} RankedDocumentArray;

/* Memory management for RankedDocumentArray */
RankedDocumentArray *ranked_document_array_create(size_t initial_capacity);
int ranked_document_array_add(RankedDocumentArray *arr, const char *filepath, double similarity_percentage);
void ranked_document_array_free(RankedDocumentArray *arr);

/**
 * Merges two sorted subarrays arr[left..mid] and arr[mid+1..right]
 * in descending order of similarity percentage.
 */
void merge(RankedDocument *arr, size_t left, size_t mid, size_t right);

/**
 * Recursively sorts an array of RankedDocument structures using Merge Sort
 * in descending order (highest similarity first).
 */
void merge_sort(RankedDocument *arr, size_t left, size_t right);

/**
 * Sorts all documents in the array and assigns 1-based ranks (1, 2, 3, ...).
 */
void ranked_document_sort_and_assign_ranks(RankedDocumentArray *arr);

#endif /* MERGESORT_H */
