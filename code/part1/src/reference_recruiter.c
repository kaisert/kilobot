/*******************************
 * definition of the state machine responsible for the recruitment of the seeders' references
 **************************************/

#include "reference_recruiter.h"

#define INQ_COUNTER 20

typedef enum {
    RR_INIT,
    GET_ATTENTION,
    GET_DIST,
    REF_CHOSEN
} RRFSM;

static RRFSM rr_state = RR_INIT;

static struct neighbour *b;
static struct neighbour *c;
static struct list_node *cur_b;
static struct list_node *cur_c;

static uint16_t reset = 0;

static uint8_t inq_count = 0;

void rr_debug_msg_out(int i1, int i2, int i3)
{
    kprints("rr msg out");
    kprints("          ");
    kprinti(i1);
    kprinti(i2);
    kprinti(i3);
    NEWLINE;
}

void rr_debug_msg_in()
{
    kprints("rr msg in ");
    kprints("          ");
    kprinti(message_rx[0]);
    kprinti(message_rx[1]);
    kprinti(message_rx[2]);
    NEWLINE;
}

static int skip_neigh() {
    //get new neighbour from list
    if(cur_b->next != neighbours->tail) {
    #ifdef DEBUG_MSG
        kprints("next att ");
        struct neighbour *n0 = (struct neighbour *) cur_b->data;
        struct neighbour *n1 = (struct neighbour *) cur_c->data;
        struct neighbour *n2 = (struct neighbour *) neighbours->tail->data;
        kprinti(n0->id);
        kprinti(n1->id);
        kprinti(n2->id);
        NEWLINE;
    #endif
        cur_b = cur_b->next;
        b = (struct neighbour *) cur_b->data;
        cur_c = cur_b->next;
        c = (struct neighbour *) cur_c->data;
        return 1;
    } else {
#ifdef DEBUG_MSG
        kprints("next b    ");
        NEWLINE;
#endif
        return 0;
    }
}

static int next_inq() {
    //get next neighbour to inquire distance
    if(cur_c->next != 0) {
        cur_c = cur_c->next;
        c = (struct neighbour *) cur_c->data;
        return 2;
    } else {
        return skip_neigh();
    }
}

void rr_init()
{
    //initialize reference recruiter
    refs->alpha = 0;
    refs->id_b = 0;
    refs->id_c = 0;
    rr_state = GET_ATTENTION;
    cur_b = (struct list_node *) neighbours->head;
    b = (struct neighbour *) cur_b->data;
    cur_c = (struct list_node *) neighbours->head->next;
    c = (struct neighbour *) cur_c->data;
}

static void rr_get_att()
{
    //request attention from B
    enable_tx = 1;
    message_out(b->id, id, M_R_R_REQ_ATT);
#ifdef DEBUG_MSG
    rr_debug_msg_out(b->id, id, M_R_R_REQ_ATT);
#endif
    //_delay_ms(200);
    uint8_t rec_msg = 0;
    rec_msg = receive_message();
    for(int i = 0; i < 10 && rec_msg != 1; ++i) {
        get_message();
        //busy waiting for response
        if(message_rx[5] == 1){
            if(message_rx[2] == M_R_R_LISTENING && message_rx[1] == b->id && message_rx[0] == id)
                rec_msg = 1;
        }
        _delay_ms(20);
    }
    if(rec_msg == 1) {
        //if attention was received, change to distance request state
#ifdef DEBUG_MSG
        rr_debug_msg_in();
#endif
        if(message_rx[2] == M_R_R_LISTENING &&
           message_rx[1] == b->id) {
            rr_state = GET_DIST;
            inq_count = 0;
        }
    }
    if(inq_count++ >= INQ_COUNTER && (cur_b != neighbours->head || inq_count >= 3*INQ_COUNTER)) {
        //if neighbour is unresponsive, change to next neighbour
#ifdef DEBUG_MSG
        NEWLINE;
        kprints("skipping  ");
        NEWLINE;
#endif
        if(skip_neigh() == 0) {
            rr_state = REF_CHOSEN;
        }
        inq_count = 0;
    }
}

static void rr_get_dist()
{
    //request distance between b and c
    enable_tx = 1;
    message_out(b->id, c->id, M_R_R_REQ_DIST);
#ifdef DEBUG_MSG
    rr_debug_msg_out(b->id, c->id, M_R_R_REQ_DIST);
#endif
    //_delay_ms(200);
    uint8_t rec_msg = 0;
    rec_msg = receive_message();
    //busy waiting for response
    for(int i = 0; i < 10 && rec_msg != 1; ++i) {
        get_message();
        if(message_rx[5] == 1){
            if(message_rx[2] == M_R_R_DIST && message_rx[1] == id)
                rec_msg = 1;
        }
        _delay_ms(20);
    }
    if(rec_msg == 1) {
#ifdef DEBUG_MSG
        rr_debug_msg_in();
#endif
        if(message_rx[2] == M_R_R_DIST &&
           message_rx[1] == id) {
            //when distance received check whether visible
            if(message_rx[0] != 0) {
                //calculate alpha, if greater -> remeber as potential references
                uint8_t dist_b = b->dist;
                uint8_t dist_c = c->dist;
                uint8_t dist_bc = message_rx[0];
                float alpha = angled(dist_b, dist_c, dist_bc);
                if(alpha > refs->alpha) {
                    refs->dist_b = dist_b;
                    refs->dist_c = dist_c;
                    refs->dist_bc = dist_bc;
                    refs->alpha = alpha;
                    refs->id_b = b->id;
                    refs->id_c = c->id;
                }
            }
            enable_tx = 1;
            message_out(b->id, id, M_R_R_ACK);
            _delay_ms(400);
    #ifdef DEBUG_MSG
            rr_debug_msg_out(b->id, id, M_R_R_ACK);
    #endif
            reset = 0;
            //get new neighbour
            if(next_inq() != 0) {
                rr_state = GET_ATTENTION;
            } else {
                if(refs->id_b != refs->id_c) {
                    rr_state = REF_CHOSEN;
                    enable_tx = 0;
#ifdef DEBUG_MSG
                    kprints("chosen    ");
                    NEWLINE;
#endif
                } else {
                    rr_state = RR_INIT;
#ifdef DEBUG_MSG
                    kprints("no ref yet");
                    NEWLINE;
#endif
                }
            }
        } else if ((message_rx[2] == M_R_R_ABORT1)
                   && message_rx[1] == b->id) {
            rr_state = GET_ATTENTION;
            reset = 0;
        }
    }
    if(reset++ > 200 && rr_state != REF_CHOSEN) {
        rr_state = GET_ATTENTION;
        reset = 0;
    }
}

static int rr_ref_chosen()
{
    //after iterating through all neighbours
    message_out(EMPTY, EMPTY, EMPTY);
    enable_tx = 0;
    if(refs->id_b != refs->id_c) {
        return 1;
    } else {
        rr_state = RR_INIT;
        return 0;
    }
}

//reference recruiter FSM
uint8_t recruit_reference()
{
    switch(rr_state) {
    case RR_INIT:
        rr_init();
        return 0;
    case GET_ATTENTION:
        LED_BLUE;
        rr_get_att();
        return 0;
    case GET_DIST:
        LED_WHITE;
        rr_get_dist();
        return 0;
    case REF_CHOSEN:
        return rr_ref_chosen();
    }
    return 0;
}
