#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include <stddef.h>

/**
 * Represents an individual word/token and its position in the document.
 */
typedef struct Token {
    char *word;        /* Word string */
    size_t position;   /* 0-based token index in document */
} Token;

/**
 * Dynamically resizable array of Tokens.
 */
typedef struct TokenArray {
    Token *items;      /* Pointer to dynamically allocated array of tokens */
    size_t count;      /* Current number of tokens */
    size_t capacity;   /* Total allocated capacity */
} TokenArray;

/**
 * Represents an individual sentence and its index in the document.
 */
typedef struct Sentence {
    char *text;        /* Sentence text string */
    size_t index;      /* 0-based sentence index in document */
} Sentence;

/**
 * Dynamically resizable array of Sentences.
 */
typedef struct SentenceArray {
    Sentence *items;   /* Pointer to dynamically allocated array of sentences */
    size_t count;      /* Current number of sentences */
    size_t capacity;   /* Total allocated capacity */
} SentenceArray;

/**
 * Preprocesses raw text by converting to lowercase, preserving sentence-ending
 * punctuation (. ! ?), removing unnecessary symbols, and normalizing whitespace.
 *
 * @param raw_text The original raw document text.
 * @return Dynamically allocated preprocessed string, or NULL on error.
 */
char *preprocess_text(const char *raw_text);

/**
 * Tokenizes text into individual words using a dynamically growing array.
 *
 * @param clean_text The preprocessed or raw text.
 * @return Pointer to dynamically allocated TokenArray, or NULL on error.
 */
TokenArray *preprocess_tokenize(const char *clean_text);

/**
 * Extracts individual sentences using a dynamically growing array.
 *
 * @param clean_text The preprocessed text containing sentence punctuation.
 * @return Pointer to dynamically allocated SentenceArray, or NULL on error.
 */
SentenceArray *preprocess_extract_sentences(const char *clean_text);

/**
 * Releases memory for a TokenArray and all contained words.
 *
 * @param array Pointer to TokenArray to free.
 */
void token_array_free(TokenArray *array);

/**
 * Releases memory for a SentenceArray and all contained sentences.
 *
 * @param array Pointer to SentenceArray to free.
 */
void sentence_array_free(SentenceArray *array);

#endif /* PREPROCESSING_H */
