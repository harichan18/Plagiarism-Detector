#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "mergesort.h"
#include "search.h"

int main(void) {
    printf("Running Search Unit Tests...\n");

    RankedDocumentArray *arr = ranked_document_array_create(5);
    ranked_document_array_add(arr, "documents/student3.txt", 87.43);
    ranked_document_array_add(arr, "documents/student1.txt", 74.26);
    ranked_document_array_add(arr, "documents/student5.txt", 51.82);
    ranked_document_array_add(arr, "documents/student2.txt", 26.17);
    ranked_document_array_add(arr, "documents/student4.txt", 8.94);

    ranked_document_sort_and_assign_ranks(arr);

    /* 1. Search existing document by basename */
    size_t rank = 0;
    const RankedDocument *d1 = search_ranked_by_filename(arr, "student3.txt", &rank);
    assert(d1 != NULL);
    assert(rank == 1);
    assert(fabs(d1->similarity_percentage - 87.43) < 1e-5);
    printf("  [PASS] Existing document by basename (first result)\n");

    /* 2. Search existing document by full path */
    const RankedDocument *d2 = search_ranked_by_filename(arr, "documents/student4.txt", &rank);
    assert(d2 != NULL);
    assert(rank == 5);
    assert(fabs(d2->similarity_percentage - 8.94) < 1e-5);
    printf("  [PASS] Existing document by full path (last result)\n");

    /* 3. Search case-insensitively */
    const RankedDocument *d3 = search_ranked_by_filename(arr, "STUDENT1.TXT", &rank);
    assert(d3 != NULL);
    assert(rank == 2);
    assert(fabs(d3->similarity_percentage - 74.26) < 1e-5);
    printf("  [PASS] Case-insensitive search\n");

    /* 4. Missing document */
    const RankedDocument *d4 = search_ranked_by_filename(arr, "nonexistent.txt", &rank);
    assert(d4 == NULL);
    printf("  [PASS] Non-existent document returns NULL\n");

    /* 5. Threshold search - multiple matches (>= 60%) */
    size_t count60 = search_ranked_by_threshold(arr, 60.0);
    assert(count60 == 2);
    printf("  [PASS] Threshold >= 60%% (2 matches)\n");

    /* 6. Threshold search - all match (>= 0%) */
    size_t count0 = search_ranked_by_threshold(arr, 0.0);
    assert(count0 == 5);
    printf("  [PASS] Threshold >= 0%% (all match)\n");

    /* 7. Threshold search - no matches (>= 95%) */
    size_t count95 = search_ranked_by_threshold(arr, 95.0);
    assert(count95 == 0);
    printf("  [PASS] Threshold >= 95%% (zero match)\n");

    ranked_document_array_free(arr);
    printf("All Search Unit Tests Passed Successfully!\n");
    return 0;
}
