#include "document.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Document *document_read(const char *filepath) {
    if (filepath == NULL || filepath[0] == '\0') {
        fprintf(stderr, "Error: Invalid file path provided.\n");
        return NULL;
    }

    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'. File not found or inaccessible.\n", filepath);
        return NULL;
    }

    /* Determine file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: Failed to seek within file '%s'.\n", filepath);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fprintf(stderr, "Error: Failed to determine size of file '%s'.\n", filepath);
        fclose(file);
        return NULL;
    }

    if (file_size == 0) {
        fprintf(stderr, "Error: Document '%s' is empty.\n", filepath);
        fclose(file);
        return NULL;
    }

    rewind(file);

    /* Allocate Document struct */
    Document *doc = (Document *)malloc(sizeof(Document));
    if (doc == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for Document structure.\n");
        fclose(file);
        return NULL;
    }

    /* Allocate memory for file path copy */
    size_t path_len = strlen(filepath);
    doc->filepath = (char *)malloc(path_len + 1);
    if (doc->filepath == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for document file path.\n");
        free(doc);
        fclose(file);
        return NULL;
    }
    memcpy(doc->filepath, filepath, path_len + 1);

    /* Allocate memory for text buffer (+1 for null terminator) */
    doc->text = (char *)malloc((size_t)file_size + 1);
    if (doc->text == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for document text buffer.\n");
        free(doc->filepath);
        free(doc);
        fclose(file);
        return NULL;
    }

    /* Read complete file content into buffer */
    size_t bytes_read = fread(doc->text, 1, (size_t)file_size, file);
    fclose(file);

    doc->text[bytes_read] = '\0';
    doc->length = bytes_read;
    doc->word_count = 0;
    doc->sentence_count = 0;

    return doc;
}

void document_free(Document *doc) {
    if (doc == NULL) {
        return;
    }

    if (doc->filepath != NULL) {
        free(doc->filepath);
        doc->filepath = NULL;
    }

    if (doc->text != NULL) {
        free(doc->text);
        doc->text = NULL;
    }

    doc->length = 0;
    doc->word_count = 0;
    doc->sentence_count = 0;

    free(doc);
}
