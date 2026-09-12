#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stddef.h>

/**
 * Represents a document loaded into memory.
 */
typedef struct Document {
    char *filepath;        /* Path or name of the document file */
    char *text;            /* Raw text content of the document */
    size_t length;         /* Length of text in characters */
    size_t word_count;     /* Number of extracted words/tokens */
    size_t sentence_count; /* Number of extracted sentences */
} Document;

/**
 * Reads the complete contents of a document file.
 * Handles file-not-found, empty file, and memory allocation errors gracefully.
 *
 * @param filepath Path to the document.
 * @return Pointer to dynamically allocated Document, or NULL on failure.
 */
Document *document_read(const char *filepath);

/**
 * Releases all dynamically allocated memory for the Document.
 *
 * @param doc Pointer to Document structure to be freed.
 */
void document_free(Document *doc);

#endif /* DOCUMENT_H */
