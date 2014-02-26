#include "seeder.h"

void init_quaternion(union quaternion *q)
{
    q->q16b[0] = 1;
    for(int i =1; i<4; ++i)
        q->q16b[i] = 0;
}

void init_vector(union vector *v)
{
    for(int i=1; i<3; ++i)
        v->v16b[1] = 0;
}

static int _comp_seeders(struct seeder *s1, struct seeder *s2)
{
    return s1->n->id == s2->n->id;
}

int comp_seeders(void *s1, void *s2)
{
    return _comp_seeders((struct seeder *) s1, (struct seeder *) s2);
}

static int _comp_int_location(uint8_t *i, struct location *l)
{
    return *i == l->s->n->id;
}

int comp_int_location(void *i, void *l)
{
    return _comp_int_location((uint8_t *) i, (struct location *) l);
}

static int _comp_id_location(uint8_t *id, struct location *l)
{
    return *id == l->s->n->id;
}

int comp_id_location(void *id, void *l)
{
    return _comp_id_location((uint8_t *) id, (struct location *) l);
}

static int _comp_seeder_location(struct seeder *s, struct location *l)
{
    return s->n->id == l->s->n->id;
}

int comp_seeder_location(void *s, void *l)
{
    return _comp_seeder_location((struct seeder *) s, (struct location *) l);
}

static int _comp_int_seeder(uint8_t *i, struct seeder *s)
{
    return *i == s->n->id;
}

int comp_int_seeder(void *i, void *s)
{
    return _comp_int_seeder((uint8_t *) i, (struct seeder *) s);
}

static int _comp_int_neigh(uint8_t *i, struct neighbour *n)
{
    return n->id == *i;
}

static int comp_int_neigh(void *i, void *n)
{
    return _comp_int_neigh((uint8_t *) i, (struct neighbour *) n);
}

static int _comp_locations(struct location *l1, struct location *l2)
{
    return l1->s->n->id == l2->s->n->id;
}

int comp_locations(void *l1, void *l2)
{
    return _comp_locations(l1,l2);
}

struct location *next_located(struct location *l)
{
    struct list_node *ln;
    struct location *ls;
    if(l == 0) {
        ls = (struct location *) (ln = get_locations()->head)->data;
        if(ls->c.x != COORD_UINIT)
            return ls;
    } else {
        ln = exists(get_locations(), l, &comp_locations);
    }
    while((ln = ln->next) != 0) {
        ls = (struct location *) ln->data;
        if(ls->c.x != COORD_UINIT)
            return ls;
    }
    return l;
}


struct seeder *new_seeder(uint8_t *sid)
{
    struct seeder *s = malloc(sizeof (struct seeder));
    struct list_node *nn = exists(get_neighbours(), sid, &comp_int_neigh);
    if(nn == 0)
        error_state(SEEDER_NO_MATCHING_N);
    s->n = (struct neighbour *) nn->data;
    init_vector(&s->v);
    init_quaternion(&s->q);
    /*s->q.c.w = 1;
    s->q.c.x = 0;
    s->q.c.y = 0;
    s->q.c.z = 0;
    s->v.c.x = 0;
    s->v.c.y = 0;
    s->v.c.z = 0;*/
    return s;
}
