#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "mergesort.h"

int main(void) {
    printf("Running Merge Sort Unit Tests...\n");

    /* 1. Zero elements */
    RankedDocumentArray *arr0 = ranked_document_array_create(4);
    ranked_document_sort_and_assign_ranks(arr0);
    assert(arr0->count == 0);
    ranked_document_array_free(arr0);
    printf("  [PASS] Zero elements test\n");

    /* 2. One element */
    RankedDocumentArray *arr1 = ranked_document_array_create(4);
    ranked_document_array_add(arr1, "single.txt", 42.0);
    ranked_document_sort_and_assign_ranks(arr1);
    assert(arr1->count == 1);
    assert(arr1->items[0].rank == 1);
    assert(fabs(arr1->items[0].similarity_percentage - 42.0) < 1e-5);
    ranked_document_array_free(arr1);
    printf("  [PASS] One element test\n");

    /* 3. Reverse sorted (ascending -> should sort descending) */
    RankedDocumentArray *arr_asc = ranked_document_array_create(4);
    ranked_document_array_add(arr_asc, "doc1.txt", 10.0);
    ranked_document_array_add(arr_asc, "doc2.txt", 30.0);
    ranked_document_array_add(arr_asc, "doc3.txt", 50.0);
    ranked_document_array_add(arr_asc, "doc4.txt", 70.0);
    ranked_document_array_add(arr_asc, "doc5.txt", 90.0);

    ranked_document_sort_and_assign_ranks(arr_asc);
    assert(arr_asc->count == 5);
    for (size_t i = 0; i < arr_asc->count; i++) {
        assert(arr_asc->items[i].rank == i + 1);
        if (i > 0) {
            assert(arr_asc->items[i - 1].similarity_percentage >= arr_asc->items[i].similarity_percentage);
        }
    }
    assert(fabs(arr_asc->items[0].similarity_percentage - 90.0) < 1e-5);
    assert(fabs(arr_asc->items[4].similarity_percentage - 10.0) < 1e-5);
    ranked_document_array_free(arr_asc);
    printf("  [PASS] Reverse sorted array test\n");

    /* 4. Already sorted (descending) */
    RankedDocumentArray *arr_desc = ranked_document_array_create(4);
    ranked_document_array_add(arr_desc, "doc1.txt", 95.0);
    ranked_document_array_add(arr_desc, "doc2.txt", 80.0);
    ranked_document_array_add(arr_desc, "doc3.txt", 60.0);
    ranked_document_array_add(arr_desc, "doc4.txt", 40.0);

    ranked_document_sort_and_assign_ranks(arr_desc);
    assert(arr_desc->count == 4);
    assert(fabs(arr_desc->items[0].similarity_percentage - 95.0) < 1e-5);
    assert(fabs(arr_desc->items[3].similarity_percentage - 40.0) < 1e-5);
    ranked_document_array_free(arr_desc);
    printf("  [PASS] Already sorted array test\n");

    /* 5. Equal similarity values (stability test) */
    RankedDocumentArray *arr_eq = ranked_document_array_create(4);
    ranked_document_array_add(arr_eq, "first.txt", 50.0);
    ranked_document_array_add(arr_eq, "second.txt", 50.0);
    ranked_document_array_add(arr_eq, "third.txt", 50.0);

    ranked_document_sort_and_assign_ranks(arr_eq);
    assert(arr_eq->count == 3);
    assert(arr_eq->items[0].rank == 1);
    assert(arr_eq->items[1].rank == 2);
    assert(arr_eq->items[2].rank == 3);
    assert(strcmp(arr_eq->items[0].filepath, "first.txt") == 0);
    assert(strcmp(arr_eq->items[1].filepath, "second.txt") == 0);
    assert(strcmp(arr_eq->items[2].filepath, "third.txt") == 0);
    ranked_document_array_free(arr_eq);
    printf("  [PASS] Equal similarity values test\n");

    /* 6. Large array test (120 elements) */
    RankedDocumentArray *arr_large = ranked_document_array_create(16);
    for (int i = 0; i < 120; i++) {
        char buf[32];
        sprintf(buf, "doc_%d.txt", i);
        double val = (double)((i * 37) % 100);
        ranked_document_array_add(arr_large, buf, val);
    }
    ranked_document_sort_and_assign_ranks(arr_large);
    assert(arr_large->count == 120);
    for (size_t i = 1; i < arr_large->count; i++) {
        assert(arr_large->items[i - 1].similarity_percentage >= arr_large->items[i].similarity_percentage);
        assert(arr_large->items[i].rank == i + 1);
    }
    ranked_document_array_free(arr_large);
    printf("  [PASS] Large array test (120 elements)\n");

    printf("All Merge Sort Unit Tests Passed Successfully!\n");
    return 0;
}
