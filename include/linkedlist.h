#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>

/**
 * Singly linked list node storing string data.
 */
typedef struct ListNode {
    char *data;
    struct ListNode *next;
} ListNode;

/**
 * Linked list container with size tracking.
 */
typedef struct LinkedList {
    ListNode *head;
    size_t size;
} LinkedList;

/* Linked list function prototypes */
LinkedList *linkedlist_create(void);
void linkedlist_append(LinkedList *list, const char *data);
int linkedlist_contains(const LinkedList *list, const char *data);
void linkedlist_free(LinkedList *list);

#endif /* LINKEDLIST_H */
