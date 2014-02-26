#include "localization.h"

typedef enum {
    FIND_NEXT_LOC_S,
    BC_SID,
    A_X_ONLY,
    A_X,
    A_Y
} A_STATE;

static struct location *anc_l = 0;
static A_STATE a_state = FIND_NEXT_LOC_S;
static struct list *locals;

static void *_malloc(size_t s)
{
    void *p = malloc(s);
    if(p == 0) {
        error_state(MALLOC_0);
    }
    return p;
}

void init_localization()
{
    locals = _malloc(sizeof(struct list));
    init_list(locals);
}

void finalize_localization()
{
    NEWLINE;
    //free locals
    if(locals->size > 0) {
        struct list_node *nlb;
        if(locals->size > 1) {
            for(nlb = locals->head->next; nlb != 0 && nlb != locals->tail; nlb = nlb->next) {
                free(nlb->prev->data);
                free(nlb->prev);
            }
        }
        free(locals->tail->data);
        free(locals->tail);
    }
    free(locals);
}

void traverse_for_loc()
{
    //if e and f are located, trilaterate and get location
    struct list_node *lbn;
    struct local_bots *lb;
    for(lbn = locals->head; lbn != 0; lbn = lbn->next) {
        lb = (struct local_bots *) lbn->data;
        struct location *l = (struct location *) exists(get_locations(), lb->s, &comp_seeder_location)->data;
        if(lb->el != 0 && lb->el->c.x != COORD_UINIT && lb->el->c.y != COORD_UINIT &&
           lb->fl != 0 && lb->fl->c.x != COORD_UINIT && lb->fl->c.y != COORD_UINIT &&
           l->c.x == COORD_UINIT)
            trilaterate(lb);
    }
}

uint8_t fully_announced()
{
    uint8_t count = 0;
    struct list_node *lbn;
    struct local_bots *lb;
    for(lbn = locals->head; lbn != 0; lbn = lbn->next) {
        lb = (struct local_bots*) lbn->data;
        count += (lb->el->c.x != COORD_UINIT && lb->el->c.y != COORD_UINIT &&
                  lb->fl->c.x != COORD_UINIT && lb->fl->c.y != COORD_UINIT);
    }
    return count;
}

static int _comp_seeder_local(struct seeder *s, struct local_bots *l)
{
    return (l->s->n->id == s->n->id);
}

static int comp_seeder_local(void *s, void *l)
{
    return _comp_seeder_local((struct seeder *) s, (struct local_bots *) l);
}

static int _comp_uanc_id_localbs(uint8_t *bid, struct local_bots *l)
{
    return ((*bid == l->e->id && (l->el->c.x == COORD_UINIT || l->el->c.y == COORD_UINIT)) ||
            (*bid == l->f->id && (l->fl->c.x == COORD_UINIT || l->fl->c.y == COORD_UINIT)));
}

static int comp_uanc_id_localbs(void *bid, void *l)
{
    return _comp_uanc_id_localbs((uint8_t *) bid, (struct local_bots *) l);
}

static uint8_t n_localized()
{
    uint8_t count = 0;
    struct list_node *ln;
    struct location *l;
    for(ln = get_locations()->head; ln != 0; ln = ln->next) {
        l = (struct location *) ln->data;
        count += (l->c.x != COORD_UINIT);
    }
    return count;
}

static struct location *get_uanc_location(uint8_t bid)
{
    struct list_node *lbn = exists(locals, &bid, &comp_uanc_id_localbs);
    struct local_bots *lbs = (struct local_bots *) lbn->data;
    if(lbn != 0 && lbs->e != 0 && lbs->e->id == bid) {
        return lbs->el;
    } else if(lbn != 0 && lbs->f != 0 && lbs->f->id == bid) {
        return lbs->fl;
    } else {
        return 0;
    }

}

static void handle_a_s()
{
    /*kprints("handle_a_s");
    NEWLINE;*/
    uint8_t new_sid = message_rx[0];
    uint8_t bid = message_rx[1];
    struct list_node *ln;
    struct seeder *s;

    //if you're a seeder and indicates position relative to you, store it
    if(get_role() == SEEDER && new_sid == id) {
        struct list_node *nn = exists(get_neighbours(), &bid, &comp_id_neigh);
        struct neighbour *n = (struct neighbour *) nn->data;
        if(exists(n->locations, &new_sid, &comp_int_location) == 0) {
           struct local_bots *lb = _malloc(sizeof(struct local_bots));
           struct location *new_l = _malloc(sizeof(struct location));
           new_l->c.x = COORD_UINIT;
           new_l->c.y = COORD_UINIT;
           new_l->s = get_sself();
           add_node(n->locations, new_l);
           lb->e = n;
           lb->el = new_l;
           lb->f = 0;
           lb->fl = 0;
           add_node(locals, lb);
        }
    } else if((ln = exists(get_locations(), &new_sid, &comp_int_location)) != 0) {
         //otherwise test whether seeder is actually known and not already located
        s = ((struct location *) ln->data)->s;
        //remember seeders seen by n
        struct list_node *nn = exists(get_neighbours(), &bid, &comp_id_neigh);
        if(nn == 0) {
            struct neighbour *new_n = _malloc(sizeof(struct neighbour));
            new_n->id = bid;
            new_n->dist = message_rx[3];
            new_n->locations = _malloc(sizeof(struct list));
            init_list(new_n->locations);
            add_node(get_neighbours(), new_n);
            nn = exists(get_neighbours(), &bid, &comp_id_neigh);
            if(nn == 0){
                error_state(LOC_H_A_S_NEIGH_UK);
                return;
            }
        }
        struct neighbour *n = (struct neighbour *) nn->data;
        struct location *l;
        if((l = (struct location *) exists(n->locations, s, &comp_seeder_location)) == 0) {
            l = _malloc(sizeof(struct location));
            l->c.x = COORD_UINIT;
            l->c.y = COORD_UINIT;
            l->s = s;
            add_node(n->locations, l);
        }
        if(l->c.x == COORD_UINIT) {
            //if unlocated then store neighbour which sent msg
            //test whether there already was a local initialized
            struct list_node *lbn = exists(locals, s, &comp_seeder_local);
            if(lbn != 0) {
                //if there was, amend the other, test, whether it's the same bot
                struct local_bots *lb = (struct local_bots *) lbn->data;
                if(lb->e == 0) {
                    lb->e = n;
                    lb->el = l;
                    lb->f = 0;
                    lb->fl = 0;
                } else if(lb->e->id != bid && lb->f == 0) {
                    lb->f = n;
                    lb->fl = l;
                } else if(lb->e->id != bid && lb->f != 0 && lb->f->id != bid) {
                    //if seeder happens to be located already, just add, to gain coordinates anyway
                    struct local_bots *lb = _malloc(sizeof(struct local_bots));
                    lb->e = n;
                    lb->el = l;
                    lb->f = 0;
                    lb->fl = 0;
                    lb->s = s;
                    add_node(locals, lb);
                }
            } else{
                //if there was not, create new one
                NEWLINE;
                blink_led(&led_orange);
                struct local_bots *lb = _malloc(sizeof(struct local_bots));
                lb->e = n;
                lb->el = l;
                lb->f = 0;
                lb->fl = 0;
                lb->s = s;
                add_node(locals, lb);
            }
        }
    }
}

static void handle_a_x()
{
    uint8_t bid = message_rx[1];
    struct location *l;
    if((l = get_uanc_location(bid)) != 0) {
        l->c.x = message_rx[0];
        kprints("get x     ");
        kprinti(l->c.x);
        NEWLINE;
        kprints("for seeder");
        kprinti(l->s->n->id);
        NEWLINE;
        if(message_rx[2] == M_A_L_X_ONLY)
            l->c.y = 0;
    }
}

static void handle_a_y()
{
    uint8_t bid = message_rx[1];
    struct location *l;
    if((l = get_uanc_location(bid)) != 0) {
        l->c.y = message_rx[0];
        kprints("get y     ");
        kprinti(l->c.y);
        NEWLINE;
    }
}

//FSM for announcments handler
void handle_acns()
{
    enable_tx = 1;
    switch(message_rx[2]) {
    case M_A_S:
        handle_a_s();
        break;
    case M_A_L_X_ONLY:
    case M_A_L_X:
        handle_a_x();
        break;
    case M_A_L_Y:
        handle_a_y();
        break;
    }
}

static void state_find_next_loc_s()
{
    if(n_localized() == 0)
        return;
    anc_l = next_located(anc_l);
    a_state = BC_SID;
}

static void state_bc_sid()
{
#ifdef DEBUG_LED
    blink_led(&led_green);
#endif
    message_out(anc_l->s->n->id, id, M_A_S);
    a_state = (anc_l->c.y == 0) ? A_X_ONLY : A_X;
}

static void state_a_x_only()
{
#ifdef DEBUG_LED
    blink_led(&led_violet);
#endif
    message_out(anc_l->c.x, id, M_A_L_X_ONLY);
    a_state = FIND_NEXT_LOC_S;
}

static void state_a_x()
{
#ifdef DEBUG_LED
    blink_led(&led_orange);
#endif
    message_out(anc_l->c.x, id, M_A_L_X);
    a_state = A_Y;
}

static void state_a_y()
{
#ifdef DEBUG_LED
    blink_led(&led_red);
#endif
    message_out(anc_l->c.y, id, M_A_L_Y);
    a_state = FIND_NEXT_LOC_S;
}

//localization FSM
void announce_location()
{
    switch(a_state){
    case FIND_NEXT_LOC_S:
        state_find_next_loc_s();
        //fallthrough
    case BC_SID:
        if(anc_l != 0)
            state_bc_sid();
        break;
    case A_X_ONLY:
        state_a_x_only();
        break;
    case A_X:
        state_a_x();
        break;
    case A_Y:
        state_a_y();
        break;
    }
    enable_tx = (anc_l != 0);
    if(anc_l == 0) {
        LED_RED;
    }
}

