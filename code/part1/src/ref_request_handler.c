#include "ref_request_handler.h"

#define UK_NEIGH 0x00
#define RESET_MAX 7

typedef enum {
    NOT_LISTENING,
    LISTENING
} RHFSM;

static uint8_t id_s;
static uint8_t id_c;
static RHFSM rh_state;
static uint16_t reset;



static void rh_debug_msg_out(int i1, int i2, int i3)
{
    kprints("rh msg out");
    kprints("          ");
    kprinti(i1);
    kprinti(i2);
    kprinti(i3);
    NEWLINE;
}

static void rh_debug_msg_in()
{
    kprints("rh msg in ");
    kprints("          ");
    kprinti(message_rx[0]);
    kprinti(message_rx[1]);
    kprinti(message_rx[2]);
    NEWLINE;
}

void rh_reset()
{
    rh_state = NOT_LISTENING;
    reset = 0;
}

void init_rh()
{
    //initialize the reference handler
    id_s = 0;
    id_c = 0;
    reset = 0;
    rh_state = NOT_LISTENING;
    if(get_role() != SEEDER)
        enable_tx = 0;
    else
        enable_tx = 1;
}

static void handle_att_req()
{
    if(rh_state == NOT_LISTENING) {
        //if bot is not listening, change responsiveness only to requesting seeder
        reset = 0;
        id_s = message_rx[1];
        message_out(id_s, id, M_R_R_LISTENING);
        rh_state = LISTENING;
        enable_tx = 1;
        _delay_ms(200);

#ifdef DEBUG_MSG
        NEWLINE;
        kprints("start lis ");
        rh_debug_msg_out(id_s, id, M_R_R_LISTENING);
        NEWLINE;
#endif
#ifdef DEBUG_LED
        blink_led(&led_orange);
#endif
    } else if (id_s == message_rx[1] && rh_state == LISTENING) {
        //if same seeder asks again, respond the same, otherwise, ignore msg
        reset = 0;
        enable_tx = 1;
        message_out(id_s, id, M_R_R_LISTENING);
        _delay_ms(200);

#ifdef DEBUG_MSG
        NEWLINE;
        kprints("already li");
        rh_debug_msg_out(id_s, id, M_R_R_LISTENING);
        NEWLINE;
#endif
#ifdef DEBUG_LED
        blink_led(&led_orange);
#endif
    } else if (id_s != message_rx[1] && rh_state == LISTENING) {
        if(reset++ >= RESET_MAX) {
            //reset state, if seeder does not react anymore
            rh_state = NOT_LISTENING;
            reset = 0;
        }
#ifdef DEBUG_LED
        blink_led(&led_red);
#endif
    }
#ifdef DEBUG_MSG
    NEWLINE;
    kprints("end att h ");
    NEWLINE;
#endif
}

static void handle_dist_req()
{
    if(rh_state == NOT_LISTENING) {
        //make sure kilobot is in appropriate state otherwise send abort msg
        enable_tx = 1;
        message_out(0x00, id, M_R_R_ABORT1);
#ifdef DEBUG_MSG
        rh_debug_msg_out(0x00, id, M_R_R_ABORT1);
#endif
        _delay_ms(200);
    } else {
        reset = 0;
        struct list_node *n;
        id_c = message_rx[1];
        enable_tx = 1;
        if((n = exists(neighbours, &id_c, &comp_id_neigh)) != 0) {
            //if neighbour is visible, send distance
            struct neighbour *c = (struct neighbour *) n->data;
            enable_tx = 1;
            message_out(c->dist, id_s, M_R_R_DIST);
            _delay_ms(200);
#ifdef DEBUG_MSG
            kprints("send dist ");
            NEWLINE;
            rh_debug_msg_out(c->dist, id_s, M_R_R_DIST);
#endif
#ifdef DEBUG_LED
            blink_led(&led_off);
#endif
        } else {
            //otherwise send 0 as indicator, neighbour is not visible
            enable_tx = 1;
            message_out(UK_NEIGH, id_s, M_R_R_DIST);
            _delay_ms(200);
#ifdef DEBUG_MSG
            kprints("neigh uk  ");
            NEWLINE;
            rh_debug_msg_out(UK_NEIGH, id_s, M_R_R_DIST);
#endif
#ifdef DEBUG_LED
            blink_led(&led_white);
#endif
        }
    }
}

static void handle_ack()
{
    //reset state, when seeder acks
    if(message_rx[1] == id_s && rh_state == LISTENING)
        rh_state = NOT_LISTENING;
}

void request_handler()
{
#ifdef DEBUG_MSG
    kprints("handler st");
    kprinti(rh_state);
    NEWLINE;
    rh_debug_msg_in();
    NEWLINE;
    kprints("seeder att");
    kprinti(id_s);
    NEWLINE;
#endif
    //reference request handle FSM
    switch(message_rx[2]) {
    case M_R_R_REQ_ATT:
        handle_att_req();
        reset_rh_c();
        break;
    case M_R_R_REQ_DIST:
        handle_dist_req();
        reset_rh_c();
        break;
    case M_R_R_ACK:
        handle_ack();
        break;
    }
    if(get_role() == SEEDER && get_master_state() == RECRUIT_REFERENCE) {
        enable_tx = 1;
    } else {
        //if not listening, don't send
        enable_tx = (rh_state == LISTENING) ? 1 : 0;
    }
    _delay_ms(100);
}
