#include "preprocessing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_TOKEN_CAPACITY 8
#define INITIAL_SENTENCE_CAPACITY 4

static int is_sentence_delimiter(char c) {
    return (c == '.' || c == '!' || c == '?');
}

char *preprocess_text(const char *raw_text) {
    if (raw_text == NULL) {
        return NULL;
    }

    size_t len = strlen(raw_text);
    /* Output text will not exceed (len * 2 + 2) */
    char *result = (char *)malloc(len * 2 + 2);
    if (result == NULL) {
        fprintf(stderr, "Error: Memory allocation failed during text preprocessing.\n");
        return NULL;
    }

    size_t out_idx = 0;
    int prev_was_space = 0;

    for (size_t i = 0; i < len; i++) {
        char c = raw_text[i];

        if (is_sentence_delimiter(c)) {
            /* Remove space immediately before punctuation if present */
            if (out_idx > 0 && result[out_idx - 1] == ' ') {
                out_idx--;
            }
            result[out_idx++] = c;
            /* If next character is alphanumeric or space, ensure a space follows */
            if (i + 1 < len && !is_sentence_delimiter(raw_text[i + 1])) {
                result[out_idx++] = ' ';
                prev_was_space = 1;
            } else {
                prev_was_space = 0;
            }
        } else if (isalnum((unsigned char)c)) {
            result[out_idx++] = (char)tolower((unsigned char)c);
            prev_was_space = 0;
        } else {
            /* Treat all other punctuation and whitespace as whitespace separator */
            if (!prev_was_space && out_idx > 0) {
                result[out_idx++] = ' ';
                prev_was_space = 1;
            }
        }
    }

    /* Trim trailing space */
    while (out_idx > 0 && result[out_idx - 1] == ' ') {
        out_idx--;
    }

    result[out_idx] = '\0';
    return result;
}

TokenArray *preprocess_tokenize(const char *clean_text) {
    if (clean_text == NULL) {
        return NULL;
    }

    TokenArray *arr = (TokenArray *)malloc(sizeof(TokenArray));
    if (arr == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for TokenArray.\n");
        return NULL;
    }

    arr->count = 0;
    arr->capacity = INITIAL_TOKEN_CAPACITY;
    arr->items = (Token *)malloc(arr->capacity * sizeof(Token));
    if (arr->items == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for token items.\n");
        free(arr);
        return NULL;
    }

    size_t len = strlen(clean_text);
    size_t i = 0;

    while (i < len) {
        /* Skip non-alphanumeric characters */
        while (i < len && !isalnum((unsigned char)clean_text[i])) {
            i++;
        }
        if (i >= len) {
            break;
        }

        size_t start = i;
        while (i < len && isalnum((unsigned char)clean_text[i])) {
            i++;
        }
        size_t word_len = i - start;

        /* Dynamically grow TokenArray capacity if required */
        if (arr->count >= arr->capacity) {
            size_t new_capacity = arr->capacity * 2;
            Token *new_items = (Token *)realloc(arr->items, new_capacity * sizeof(Token));
            if (new_items == NULL) {
                fprintf(stderr, "Error: Memory allocation failed while resizing TokenArray.\n");
                token_array_free(arr);
                return NULL;
            }
            arr->items = new_items;
            arr->capacity = new_capacity;
        }

        char *word = (char *)malloc(word_len + 1);
        if (word == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for word string.\n");
            token_array_free(arr);
            return NULL;
        }

        memcpy(word, clean_text + start, word_len);
        word[word_len] = '\0';

        arr->items[arr->count].word = word;
        arr->items[arr->count].position = arr->count;
        arr->count++;
    }

    return arr;
}

SentenceArray *preprocess_extract_sentences(const char *clean_text) {
    if (clean_text == NULL) {
        return NULL;
    }

    SentenceArray *arr = (SentenceArray *)malloc(sizeof(SentenceArray));
    if (arr == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for SentenceArray.\n");
        return NULL;
    }

    arr->count = 0;
    arr->capacity = INITIAL_SENTENCE_CAPACITY;
    arr->items = (Sentence *)malloc(arr->capacity * sizeof(Sentence));
    if (arr->items == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for sentence items.\n");
        free(arr);
        return NULL;
    }

    size_t len = strlen(clean_text);
    size_t start = 0;

    for (size_t i = 0; i <= len; i++) {
        char c = clean_text[i];
        if (is_sentence_delimiter(c) || c == '\0') {
            size_t end = i;

            /* Trim leading spaces */
            while (start < end && isspace((unsigned char)clean_text[start])) {
                start++;
            }

            /* Trim trailing spaces and trailing delimiters */
            while (end > start && (isspace((unsigned char)clean_text[end - 1]) ||
                                   is_sentence_delimiter(clean_text[end - 1]))) {
                end--;
            }

            size_t sent_len = end - start;
            if (sent_len > 0) {
                /* Dynamically grow SentenceArray capacity if required */
                if (arr->count >= arr->capacity) {
                    size_t new_capacity = arr->capacity * 2;
                    Sentence *new_items = (Sentence *)realloc(arr->items, new_capacity * sizeof(Sentence));
                    if (new_items == NULL) {
                        fprintf(stderr, "Error: Memory allocation failed while resizing SentenceArray.\n");
                        sentence_array_free(arr);
                        return NULL;
                    }
                    arr->items = new_items;
                    arr->capacity = new_capacity;
                }

                char *sent_text = (char *)malloc(sent_len + 1);
                if (sent_text == NULL) {
                    fprintf(stderr, "Error: Memory allocation failed for sentence text.\n");
                    sentence_array_free(arr);
                    return NULL;
                }

                memcpy(sent_text, clean_text + start, sent_len);
                sent_text[sent_len] = '\0';

                arr->items[arr->count].text = sent_text;
                arr->items[arr->count].index = arr->count;
                arr->count++;
            }

            start = i + 1;
        }
    }

    return arr;
}

void token_array_free(TokenArray *array) {
    if (array == NULL) {
        return;
    }

    if (array->items != NULL) {
        for (size_t i = 0; i < array->count; i++) {
            if (array->items[i].word != NULL) {
                free(array->items[i].word);
                array->items[i].word = NULL;
            }
        }
        free(array->items);
        array->items = NULL;
    }

    array->count = 0;
    array->capacity = 0;
    free(array);
}

void sentence_array_free(SentenceArray *array) {
    if (array == NULL) {
        return;
    }

    if (array->items != NULL) {
        for (size_t i = 0; i < array->count; i++) {
            if (array->items[i].text != NULL) {
                free(array->items[i].text);
                array->items[i].text = NULL;
            }
        }
        free(array->items);
        array->items = NULL;
    }

    array->count = 0;
    array->capacity = 0;
    free(array);
}
