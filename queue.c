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
    /*list_del_init*/
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

    struct list_head *forward = head->next;
    struct list_head *backward = head;

    while (forward != backward) {
        backward = backward->prev;
        if (forward == backward)
            break;
        forward = forward->next;
    }

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

    struct list_head *node = head->next->next;
    struct list_head *prev = head;

    while (node != head) {
        element_t *element = list_entry(node, element_t, list);
        element_t *prev_element = list_entry(node->prev, element_t, list);

        if (strcmp(element->value, prev_element->value) == 0) {
            while (node != head &&
                   strcmp(element->value, prev_element->value) == 0) {
                node = node->next;
                q_release_element(prev_element);
                prev_element = element;
                element = list_entry(node, element_t, list);
            }
            q_release_element(prev_element);
            prev->next = node;
            node->prev = prev;
            if (node != head)
                node = node->next;
        } else {
            node = node->next;
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

    struct list_head *node = head->next;
    int toggle = 0;

    while (node != head) {
        if (toggle) {
            struct list_head *prev = node->prev;
            prev->next = node->next;
            node->prev = prev->prev;
            node->next->prev = prev;
            prev->prev->next = node;
            prev->prev = node;
            node->next = prev;
            node = prev;
        }
        toggle = !toggle;
        node = node->next;
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

    struct list_head *node;
    struct list_head *safe;

    list_for_each_safe (node, safe, head) {
        struct list_head *next = node->next;
        struct list_head *prev = node->prev;

        node->next = prev;
        node->prev = next;
    }

    struct list_head *next = head->next;
    struct list_head *prev = head->prev;

    head->next = prev;
    head->prev = next;
}

struct list_head *merge(struct list_head *a, struct list_head *b)
{
    if (!a || !b)
        return a ? a : b;

    struct list_head *head = NULL;
    struct list_head **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        char *sa = list_entry(a, element_t, list)->value;
        char *sb = list_entry(b, element_t, list)->value;

        if (strcmp(sa, sb) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

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

    struct list_head *array[32] = {};
    struct list_head *result = head->next;
    struct list_head *next;
    int i;

    head->prev->next = NULL;
    while (result) {
        next = result->next;
        result->next = NULL;
        for (i = 0; i < 32 && array[i]; i++) {
            result = merge(array[i], result);
            array[i] = NULL;
        }

        if (i == 32)
            i--;
        array[i] = result;
        result = next;
    }

    /*merge final*/
    result = NULL;
    for (i = 0; i < 32; i++) {
        result = merge(array[i], result);
    }
    head->next = result;
    struct list_head *node = head;
    while (node->next) {
        node->next->prev = node;
        node = node->next;
    }
    node->next = head;
    head->prev = node;
}
