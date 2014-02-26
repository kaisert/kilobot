#include "list.h"

static int comp_pointers(void *p1, void *p2) {
    return p1 == p2;
}

int comp_refs(struct references *r1, struct references *r2)
{
    if((r1->id_b == r2->id_b && r1->id_c == r2->id_c) ||
       (r1->id_c == r2->id_b && r1->id_b == r2->id_c)) {
        return 1;
    } else {
        return 0;
    }
}

int comp_int(uint8_t *i1, uint8_t *i2)
{
    return (*i1 == *i2);
}

static int _neigh_id(uint8_t *pid, struct neighbour *n)
{
    return (*pid == n->id);
}

int comp_id_neigh(void *pid, void *n)
{
    return _neigh_id((uint8_t *) pid, (struct neighbour *) n);
}

struct list_node *exists(struct list *l, void * data, int (*comp) (void *, void *))
{
    if(l->size <= 0)
        return 0;
    struct list_node *cur;
    for(cur = l->head; cur != 0; cur = cur->next) {
        if((*comp)(data, cur->data) == 1)
            return cur;
    }
    return 0;
}

static int node_exists(struct list *l, struct list_node *n) {
    return exists(l, n, &comp_pointers) != 0;
}

/*void clear_list(struct list *l)
{
    if(l->size <= 0)
        return;
    while(remove_node(l, l->head) != 0) { }
}*/

struct list_node *remove_node(struct list *l, struct list_node *n)
{
    if(l->size <= 0) {
        return 0;
    } else if(node_exists(l, n) != 0) {
        struct list_node *old = n;
        if(l->size == 1) {
            l->head = 0;
            l->tail = 0;
        } else if(n == l->head) {
            l->head = l->head->next;
            l->head->prev = 0;
        } else if(n == l->tail) {
            l->tail = l->tail->prev;
            l->tail->next = 0;
        } else {
            n->prev->next = n->next;
            n->next->prev = n->prev;
        }
        --(l->size);
        return old;
    } else {
        return 0;
    }
}

void add_node(struct list *l, void *d)
{
    struct list_node *n = malloc(sizeof(struct list_node));
    n->data = d;
    if(l->size <= 0) {
        l->head = n;
        l->tail = n;
        n->prev = 0;
        n->next = 0;
        l->size = 0;
    } else {
        l->tail->next = n;
        n->prev = l->tail;
        n->next = 0;
        l->tail = n;
    }
    ++(l->size);
}

void init_list(struct list *l)
{
    l->head = 0;
    l->size = 0;
    l->tail = 0;
}
