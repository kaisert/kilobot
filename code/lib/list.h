#ifndef LIST_H
#define LIST_H

#include "libkb.h"

struct references {
    uint8_t id_b;
    uint8_t id_c;
    uint8_t dist_b;
    uint8_t dist_c;
    uint8_t dist_bc;
    float alpha;
};

struct list_node {
    void *data;
    struct list_node *next;
    struct list_node *prev;
};

struct list {
    struct list_node *head;
    struct list_node *tail;
    int size;
};

struct neighbour {
    uint8_t id;
    uint8_t dist;
    struct list *locations;
};





extern int comp_refs(struct references *, struct references *);
extern int comp_int(uint8_t *, uint8_t *);
extern int comp_id_neigh(void *, void *);

extern struct list_node *exists(struct list *, void *, int (*)(void *, void*));
//extern void clear_list(struct list * );
extern struct list_node *remove_node(struct list * , struct list_node *);
extern void add_node(struct list *, void * );
extern void init_list(struct list * );
#endif
