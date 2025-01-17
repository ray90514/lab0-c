#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head)
        return NULL;

    INIT_LIST_HEAD(head);

    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    struct list_head *node;
    struct list_head *safe;

    list_for_each_safe (node, safe, head) {
        q_release_element(list_entry(node, element_t, list));
    }

    free(head);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *element = malloc(sizeof(element_t));
    if (!element)
        return false;

    element->value = strdup(s);
    if (!element->value) {
        free(element);
        return false;
    }

    list_add(&element->list, head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *element = malloc(sizeof(element_t));
    if (!element)
        return false;

    element->value = strdup(s);
    if (!element->value) {
        free(element);
        return false;
    }

    list_add_tail(&element->list, head);
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *element = list_first_entry(head, element_t, list);
    list_del(&element->list);
    if (sp) {
        strncpy(sp, element->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return element;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *element = list_last_entry(head, element_t, list);
    list_del(&element->list);
    if (sp) {
        strncpy(sp, element->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return element;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    struct list_head *node;
    int len = 0;

    list_for_each (node, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;

    struct list_head *forward = head;
    struct list_head *backward = head;

    do {
        forward = forward->next;
        if (forward == backward)
            break;
        backward = backward->prev;
    } while (forward != backward);

    list_del(forward);
    q_release_element(list_entry(forward, element_t, list));
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;

    bool is_dup = false;
    element_t *entry;
    element_t *safe;
    struct list_head *prev = head;

    list_for_each_entry_safe (entry, safe, head, list) {
        if (&safe->list != head && strcmp(entry->value, safe->value) == 0) {
            q_release_element(entry);
            is_dup = true;
        } else if (is_dup) {
            is_dup = false;
            q_release_element(entry);
            prev->next = &safe->list;
            safe->list.prev = prev;
        } else {
            prev = prev->next;
        }
    }
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || list_empty(head))
        return;

    for (struct list_head *node = head->next;
         node != head && node->next != head; node = node->next) {
        struct list_head *next = node->next;
        node->prev->next = next;
        next->next->prev = node;
        node->next = next->next;
        next->next = node;
        next->prev = node->prev;
        node->prev = next;
    }
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    struct list_head *node = head;
    struct list_head *next = node->next;
    do {
        node->next = node->prev;
        node->prev = next;
        node = next;
        next = node->next;
    } while (node != head);
}

struct list_head *merge(struct list_head *a, struct list_head *b)
{
    struct list_head head = {.next = NULL};
    struct list_head *tail = &head;

    while (a && b) {
        char *sa = list_entry(a, element_t, list)->value;
        char *sb = list_entry(b, element_t, list)->value;

        /* if equal, take 'a' -- important for sort stability */
        struct list_head **smaller = strcmp(sa, sb) <= 0 ? &a : &b;
        tail->next = *smaller;
        tail = tail->next;
        *smaller = (*smaller)->next;
    }

    tail->next = (struct list_head *) ((uintptr_t) a | (uintptr_t) b);
    return head.next;
}

struct list_head *merge_final(struct list_head *head,
                              struct list_head *a,
                              struct list_head *b)
{
    struct list_head *tail = head;

    while (a && b) {
        char *sa = list_entry(a, element_t, list)->value;
        char *sb = list_entry(b, element_t, list)->value;

        /* if equal, take 'a' -- important for sort stability */
        struct list_head **smaller = strcmp(sa, sb) <= 0 ? &a : &b;
        tail->next = *smaller;
        (*smaller)->prev = tail;
        tail = tail->next;
        *smaller = (*smaller)->next;
    }

    tail->next = (struct list_head *) ((uintptr_t) a | (uintptr_t) b);
    while (tail->next) {
        tail->next->prev = tail;
        tail = tail->next;
    }

    tail->next = head;
    head->prev = tail;
    return head;
}

#define SORT_BUFSIZE 32
/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    /* https://en.wikipedia.org/wiki/Merge_sort#Bottom-up_implementation_using_lists
     */
    if (!head || head->next == head->prev)
        return;

    struct list_head *pending[SORT_BUFSIZE] = {};
    struct list_head *result = head->next;
    struct list_head *next;
    int i;

    head->prev->next = NULL;
    while (result) {
        next = result->next;
        result->next = NULL;
        for (i = 0; i < SORT_BUFSIZE && pending[i]; i++) {
            result = merge(pending[i], result);
            pending[i] = NULL;
        }

        if (i == SORT_BUFSIZE)
            i--;
        pending[i] = result;
        result = next;
    }

    /*merge final*/
    result = NULL;
    for (i = 0; i < SORT_BUFSIZE - 1; i++) {
        result = merge(pending[i], result);
    }
    merge_final(head, result, pending[SORT_BUFSIZE - 1]);
}