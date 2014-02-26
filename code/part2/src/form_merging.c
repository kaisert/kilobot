#include "form_merging.h"

#define REG_COUNT_MAX 200

typedef enum {
    INIT,
    //CHOOSE_NEXT,
    //REQ_SEEING,
    REGISTER_M_GROUP,
    DONE
} FMG_STATE;

struct tuple {
    uint8_t n;
    uint8_t s;
};

struct triplet {
    uint8_t n1;
    uint8_t n2;
    uint8_t s;
};

static uint16_t reg_counter = 0;
static struct list *ns_tuples;
static struct list *nns_triplets;
static struct list m_groups = {0,0,0};
static struct list_node *active_nmg = 0;
static struct m_group *active_mg = 0;
static FMG_STATE fmg_state = INIT;

struct list *get_m_groups()
{
    return &m_groups;
}

void init_merging()
{
    ns_tuples = malloc(sizeof(struct list));
    nns_triplets = malloc(sizeof(struct list));
    init_list(ns_tuples);
    init_list(nns_triplets);
}

void create_tuples()
{
    struct list_node *nn;
    struct list_node *ln;
    struct neighbour *n;
    struct location *l;
    //if you're a seeder, add yourself with every neighbour
    if(get_role() == SEEDER) {
        for(nn = get_neighbours()->head; nn != 0; nn = nn->next) {
            n = (struct neighbour *) nn->data;
            if(exists(n->locations, get_sself(), &comp_seeder_location) != 0) {
                struct tuple *t = malloc(sizeof(struct tuple));
                t->n = n->id;
                t->s = get_lself()->s->n->id;
                add_node(ns_tuples, t);
            }
        }
    }
    //first add all seeders, as they always see themselves
    for(ln = get_locations()->head; ln != 0; ln = ln->next) {
        l = (struct location *) ln->data;
        if(l->c.x != COORD_UINIT) {
            struct tuple *t = malloc(sizeof(struct tuple));
            t->n = l->s->n->id;
            t->s = l->s->n->id;
            add_node(ns_tuples, t);
        }
    }
    //then add all neighbour/seeder pairs, where the neighbours are located
    for(nn = get_neighbours()->head; nn != 0; nn = nn->next) {
        n = (struct neighbour *) nn->data;
        for(ln = n->locations->head; ln != 0; ln = ln->next) {
            l = (struct location *) ln->data;
            struct list_node *nown_l = exists(get_locations(), l->s, &comp_seeder_location);
            struct location *own_l = (struct location *) nown_l->data;
            if(l->c.x != COORD_UINIT && nown_l != 0 && own_l->c.x != COORD_UINIT) {
                struct tuple *t = malloc(sizeof(struct tuple));
                t->n = n->id;
                t->s = ((struct location *) ln->data)->s->n->id;
                add_node(ns_tuples, t);
            }
        }
    }
}

void create_triplets()
{
    //create triplets, when two neighbours are located in the same seeder's c-system
    struct list_node *nt1;
    struct list_node *nt2;
    struct tuple *t1;
    struct tuple *t2;
    for(nt1 = ns_tuples->head; nt1 != ns_tuples->tail; nt1 = nt1->next) {
        t1 = (struct tuple *) nt1->data;
        for(nt2 = nt1->next; nt2 != 0; nt2 = nt2->next) {
            t2 = (struct tuple *) nt2->data;
            if(t2->s == t1->s && t2->n != t1->n) {
                struct triplet *trp = malloc(sizeof(struct triplet));
                trp->n1 = t1->n;
                trp->n2 = t2->n;
                trp->s = t1->s;
                add_node(nns_triplets, trp);
            }
        }
    }
}

void create_mgs()
{
    //create merging groups when two bots are located in the same two c-systems
    struct list_node *ntrp1;
    struct list_node *ntrp2;
    struct triplet *trp1;
    struct triplet *trp2;
    for(ntrp1 = nns_triplets->head; ntrp1 != 0; ntrp1 = ntrp1->next) {
        trp1 = (struct triplet *) ntrp1->data;
        for(ntrp2 = ntrp1->next; ntrp2 != 0; ntrp2 = ntrp2->next) {
            trp2 = (struct triplet *) ntrp2->data;
            if(((trp1->n1 == trp2->n1 && trp1->n2 == trp2->n2) ||
                (trp1->n1 == trp2->n2 && trp1->n2 == trp2->n1)) &&
                    trp1->s != trp2->s)
            {
                struct m_group *mg = malloc(sizeof(struct m_group));
                mg->n1 = trp1->n1;
                mg->n2 = trp1->n2;
                mg->s1 = trp1->s;
                mg->s2 = trp2->s;
                add_node(&m_groups, mg);
            }
        }
    }
}

void state_init()
{
    enable_tx = 1;
    init_merging();
    create_tuples();
    create_triplets();
    //free tuples list
    struct list_node *n;
    if(ns_tuples->size > 0){
        if(ns_tuples->size == 1) {
            free(ns_tuples->head->data);
            free(ns_tuples->head);
        } else if(ns_tuples->size > 1) {
            for(n = ns_tuples->head->next; n != 0; n = n->next) {
                free(n->prev->data);
                free(n->prev);
            }
            free(ns_tuples->tail->data);
            free(ns_tuples->tail);
        }
    }
    free(ns_tuples);
    create_mgs();
    //free triplets list
    if(nns_triplets > 0) {
        if(nns_triplets->size == 1) {
            free(nns_triplets->head->data);
            free(nns_triplets->head);
        } else if(nns_triplets->size > 1) {
            for(n = nns_triplets->head->next; n != 0; n = n->next) {
                free(n->prev->data);
                free(n->prev);
            }
            free(nns_triplets->tail->data);
            free(nns_triplets->tail);
        }
    }
    free(nns_triplets);

    active_nmg = get_m_groups()->head;
    active_mg = active_nmg->data;
    fmg_state = REGISTER_M_GROUP;
}

void fm_register_handler()
{
    //save merging group registration
    uint8_t bid = message_rx[1];
    struct list_node *n = exists(get_neighbours(), &bid, &comp_id_neigh);
    if(n == 0) {
        error_state(FM_UNDEFINED_N);
    }
    if(exists(get_mergers(), &bid, &comp_id_neigh) == 0) {
        add_node(get_mergers(), n->data);
        blink_led(&led_green);
    } else {
        //blink_led(&led_red);
    }
}

uint8_t state_register_m_group()
{
    //register all the merging groups a bot is part of
    if(get_role() == SEEDER)
        receive_message();
    if(active_nmg == 0) {
        //make merging group list a ring
        m_groups.head->prev = m_groups.tail;
        m_groups.tail->next = m_groups.head;
        fmg_state = DONE;
        return 1;
    }
    active_mg = (struct m_group *) active_nmg->data;
    reg_counter++;
    if(reg_counter > (2 * REG_COUNT_MAX)) {
        blink_led(&led_red);
        active_nmg = active_nmg->next;
        active_mg = active_nmg->data;
        reg_counter = 0;
    } else if(reg_counter < REG_COUNT_MAX) {
        message_out(active_mg->s1, id, M_REGISTER_M_G);
    } else {
        message_out(active_mg->s2, id, M_REGISTER_M_G);
    }
    kprinti(reg_counter);
    _delay_ms(5);
    return 0;
}

uint8_t form_m_gs()
{
    switch (fmg_state) {
    case INIT:
        state_init();
        //fallthrough
    case REGISTER_M_GROUP:
        return state_register_m_group();
    case DONE:
        return 1;
    }
    return -1;
}
