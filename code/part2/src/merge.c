#include "merge.h"

#define COUNTER_LIMIT 200

typedef enum {
    PROP_V,
    PROP_Q,
    FINISH_PROP
} P_FSM;

typedef enum {
    S_CHOOSE_M_G,
    S_INIT_MERGER,
    S_MERGING
} SM_FSM;

static SM_FSM sm_state = S_CHOOSE_M_G;
static BM_FSM bm_state = LISTEN;
static P_FSM p_state = PROP_V;
static struct neighbour *chosen_merger;
static struct location *mergee;
static uint16_t counter;

BM_FSM get_bm_state()
{
    return bm_state;
}

void s_merge_rec_v_handler()
{
    //receive new vector, as seeder
    uint8_t i = (message_rx[2] - M_SEND_V1) >> 1;
    get_lself()->s->v.v8b[i] = message_rx[0];
}

void s_merge_rec_q_handler()
{
    //receive new quaternion, as seeder
    uint8_t i = (message_rx[2] - M_SEND_Q1) >> 1;
    get_lself()->s->q.q8b[i] = message_rx[0];
    if(i == 7)
        proceed_to_prop();
}

void b_merge_req_handler()
{
    blink_led(&led_violet);
    uint8_t sid = message_rx[1];
    if(sid == id)
        return;
    //choose random merging group s is in
    int8_t r = rand() % get_m_groups()->size;
    struct list_node *nmg = get_m_groups()->head;
    struct m_group *mg = nmg->data;
    //iterate through merging group circle until r > 0 and a matching merging group is found
    while(r > 0) {
        nmg = nmg->next;
        mg = nmg->data;
        --r;
    }
    while(mg->s1 != sid && mg->s2 != sid) {
        nmg = nmg->next;
        mg = nmg->data;
    }
    uint8_t s2id;
    if(sid == mg->s1)
        s2id = mg->s2;
    else
        s2id = mg->s1;

    mergee = exists(get_locations(), &sid, &comp_id_location)->data;
    /*******************************************************************
     *TEST, actual protocol of merging is not functioning properly     *
     *******************************************************************/
    mergee->s->v.v16b[0] = id;
    mergee->s->v.v16b[1] = id + 1;
    mergee->s->v.v16b[2] = id + 2;
    mergee->s->q.q16b[0] = id + 3;
    mergee->s->q.q16b[1] = id + 4;
    mergee->s->q.q16b[2] = id + 5;
    mergee->s->q.q16b[3] = id + 6;
    /*struct location s2 = exists(get_locations(), &s2id, &comp_id_location)->data;
    //retrieve the corresponding neighbours and calculate the new v and q for the seeder
    struct neighbour *n1 = exists(get_neighbours(), &mg->n1, &comp_id_neigh)->data;
    struct neighbour *n2 = exists(get_neighbours(), &mg->n2, &comp_id_neigh)->data;
    struct coordinates *c1_n1 = ((struct location *)exists(n1->locations, &sid, &comp_id_location)->data)->c;
    struct coordinates *c2_n1 = ((struct location *)exists(n1->locations, &s2id, &comp_id_location)->data)->c;
    struct coordinates *c1_n2 = ((struct location *)exists(n2->locations, &sid, &comp_id_location)->data)->c;
    struct coordinates *c2_n2 = ((struct location *)exists(n2->locations, &s2id, &comp_id_location)->data)->c;
    calculate_new_v_q(mergee, s2, c1_n1, c2_n1, c1_n2, c2_n2);*/

    enable_tx = 1;
    counter = 0;
    bm_state = SEND_V;
}

static void state_b_send_v()
{
    //gradually send new vector to seeder
    uint8_t i = counter++ / COUNTER_LIMIT;
    if(i == 6) {
        counter = 0;
        bm_state = SEND_Q;
        return;
    }
    uint8_t msg = M_SEND_V1 + (i << 1);
    message_out(mergee->s->v.v8b[i], mergee->s->n->id, msg);
    kprints("send v    ");
    kprinti(mergee->s->n->id);
    NEWLINE;
    LED_BLUE;
    _delay_ms(5);
}

static void state_b_send_q()
{
    //gradually send the new quaternion to the seeder
    uint8_t i = counter++ / COUNTER_LIMIT;
    if(i == 8) {
        counter = 0;
        bm_state = LISTEN;
        return;
    }
    uint8_t msg = M_SEND_Q1 + (i << 1);
    message_out(mergee->s->q.q8b[i], mergee->s->n->id, msg);
    kprints("send Q    ");
    kprinti(mergee->s->n->id);
    NEWLINE;
    LED_ORANGE;
    _delay_ms(5);
}

void b_merge()
{
    switch (bm_state) {
    case LISTEN:
        LED_OFF;
        break;
    case SEND_V:
        state_b_send_v();
        break;
    case SEND_Q:
        state_b_send_q();
        break;
    }
    enable_tx = bm_state != LISTEN;
}

void init_new_round()
{
    enable_tx = 1;
    sm_state = S_CHOOSE_M_G;
}

static void state_s_choose_m_g()
{
    //randomly choose a merging group
    blink_led(&led_white);
    uint8_t rand_merger = rand() % get_mergers()->size;
    struct list_node *ln_merger = get_mergers()->head;
    for(;rand_merger > 0; rand_merger--)
        ln_merger = ln_merger->next;
    chosen_merger = ln_merger->data;
    if(chosen_merger->id == id)
        return;
    sm_state = S_INIT_MERGER;
}

static void state_s_init_merger()
{
    //initialize merging
    enable_tx = 1;
    message_out(chosen_merger->id, id, M_MERGE);
    get_message();
    if(message_rx[3] == M_SEND_V1 && message_rx[1] == id) {
        s_merge_rec_v_handler();
        enable_tx = 0;
        sm_state = S_MERGING;
    }
}

uint8_t s_merge()
{
    //FSM of the seeder while merging
    switch(sm_state) {
    case S_CHOOSE_M_G:
        state_s_choose_m_g();
        //fallthrough
    case S_INIT_MERGER:
        state_s_init_merger();
        return 0;
    case S_MERGING:
        return 1;
    }
    return 0;
}

void p_new_v_handler()
{
    //new vector-byte received
    uint8_t sid = message_rx[1];
    struct location *l = exists(get_locations(), &sid, &comp_id_location)->data;
    uint8_t i = (message_rx[2] - M_PROP_V1) >> 1;
    l->s->v.v8b[i] = message_rx[0];
}

void p_new_q_handler()
{
    //new quaternion-byte received
    uint8_t sid = message_rx[1];
    struct location *l = exists(get_locations(), &sid, &comp_id_location)->data;
    uint8_t i = (message_rx[2] - M_PROP_V1) >> 1;
    l->s->q.q8b[i] = message_rx[0];
}

static void state_prop_v()
{
    //prpagate new vector
    uint8_t i = counter++ / COUNTER_LIMIT;
    if(i == 6) {
        counter = 0;
        p_state = PROP_Q;
    }
    uint8_t msg = M_PROP_V1 + (i << 1);
    message_out(mergee->s->v.v8b[i], id, msg);
    LED_VIOLET;
    _delay_ms(5);
}

void state_prop_q()
{
    //propagate the quaternion
    uint8_t i = counter++ / COUNTER_LIMIT;
    if(i == 8) {
        counter = 0;
        p_state = FINISH_PROP;
    }
    uint8_t msg = M_PROP_V1 + (i << 1);
    message_out(mergee->s->v.v8b[i], id, msg);
    LED_RED;
    _delay_ms(5);
}

uint8_t propagate()
{
    //propagation of new vector and quaternion
    enable_tx = !(p_state == FINISH_PROP);
    switch(p_state) {
    case PROP_V:
        state_prop_v();
        return 0;
    case PROP_Q:
        state_prop_q();
        return 0;
    case FINISH_PROP:
        return 1;
    }
    return 0;
}
