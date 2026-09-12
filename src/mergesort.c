#include "mergesort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_RANKED_CAPACITY 8

RankedDocumentArray *ranked_document_array_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = INITIAL_RANKED_CAPACITY;
    }

    RankedDocumentArray *arr = (RankedDocumentArray *)malloc(sizeof(RankedDocumentArray));
    if (arr == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for RankedDocumentArray.\n");
        return NULL;
    }

    arr->count = 0;
    arr->capacity = initial_capacity;
    arr->items = (RankedDocument *)malloc(arr->capacity * sizeof(RankedDocument));
    if (arr->items == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for RankedDocument items.\n");
        free(arr);
        return NULL;
    }

    return arr;
}

int ranked_document_array_add(RankedDocumentArray *arr, const char *filepath, double similarity_percentage) {
    if (arr == NULL || filepath == NULL) {
        return 0;
    }

    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity * 2;
        RankedDocument *new_items = (RankedDocument *)realloc(arr->items, new_cap * sizeof(RankedDocument));
        if (new_items == NULL) {
            fprintf(stderr, "Error: Memory reallocation failed for RankedDocumentArray.\n");
            return 0;
        }
        arr->items = new_items;
        arr->capacity = new_cap;
    }

    size_t path_len = strlen(filepath);
    char *path_copy = (char *)malloc(path_len + 1);
    if (path_copy == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for ranked document filepath.\n");
        return 0;
    }
    memcpy(path_copy, filepath, path_len + 1);

    arr->items[arr->count].filepath = path_copy;
    arr->items[arr->count].similarity_percentage = similarity_percentage;
    arr->items[arr->count].rank = 0;
    arr->count++;

    return 1;
}

void ranked_document_array_free(RankedDocumentArray *arr) {
    if (arr == NULL) {
        return;
    }

    if (arr->items != NULL) {
        for (size_t i = 0; i < arr->count; i++) {
            if (arr->items[i].filepath != NULL) {
                free(arr->items[i].filepath);
                arr->items[i].filepath = NULL;
            }
        }
        free(arr->items);
        arr->items = NULL;
    }

    arr->count = 0;
    arr->capacity = 0;
    free(arr);
}

void merge(RankedDocument *arr, size_t left, size_t mid, size_t right) {
    size_t n1 = mid - left + 1;
    size_t n2 = right - mid;

    RankedDocument *L = (RankedDocument *)malloc(n1 * sizeof(RankedDocument));
    RankedDocument *R = (RankedDocument *)malloc(n2 * sizeof(RankedDocument));

    if (L == NULL || R == NULL) {
        fprintf(stderr, "Error: Memory allocation failed during merge operation.\n");
        if (L) free(L);
        if (R) free(R);
        return;
    }

    for (size_t i = 0; i < n1; i++) {
        L[i] = arr[left + i];
    }
    for (size_t j = 0; j < n2; j++) {
        R[j] = arr[mid + 1 + j];
    }

    size_t i = 0;
    size_t j = 0;
    size_t k = left;

    /* Merge into arr[left..right] in descending order (highest score first) */
    while (i < n1 && j < n2) {
        if (L[i].similarity_percentage >= R[j].similarity_percentage) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) {
        arr[k++] = L[i++];
    }

    while (j < n2) {
        arr[k++] = R[j++];
    }

    free(L);
    free(R);
}

void merge_sort(RankedDocument *arr, size_t left, size_t right) {
    if (left < right) {
        size_t mid = left + (right - left) / 2;

        merge_sort(arr, left, mid);
        merge_sort(arr, mid + 1, right);

        merge(arr, left, mid, right);
    }
}

void ranked_document_sort_and_assign_ranks(RankedDocumentArray *arr) {
    if (arr == NULL || arr->count == 0) {
        return;
    }

    if (arr->count > 1) {
        merge_sort(arr->items, 0, arr->count - 1);
    }

    for (size_t i = 0; i < arr->count; i++) {
        arr->items[i].rank = i + 1;
    }
}
