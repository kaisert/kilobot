#include "reference_handler2.h"

typedef enum {
    WAITING,
    REC_B_X,
    REC_C_X,
    REC_C_Y
} RHFSM;

static RHFSM rh_state = WAITING;
static uint8_t sid;
static uint8_t c_x;

static void att_b_x_handler()
{
    if(rh_state == WAITING) {
#ifdef DEBUG_LED
        blink_led(&led_turquoise);
#endif
        enable_tx = 1;
        sid = message_rx[1];
        message_out(sid, id, M_R_R_REC_B_ACK);
        //find seeder, where coordinate gets associated with
        struct list_node *ln = exists(get_locations(), &sid, &comp_int_location);
        if(ln == 0) {
            error_state(ATT_B_X_H_SEEDER_UK);
            return;
        }
        struct location *l = (struct location *) ln->data;
        struct seeder *s = l->s;
        l->c.x = s->n->dist;
        l->c.y = 0;
        _delay_ms(200);
    }
}

static void att_c_handler()
{
    if(rh_state == WAITING || message_rx[1] == sid) {
#ifdef DEBUG_LED
        blink_led(&led_blue);
#endif
        sid = message_rx[1];
        enable_tx = 1;
        message_out(sid, id, M_R_R_REC_C_A_ACK);
        rh_state = REC_C_X;
        _delay_ms(200);
    }
}

static void att_c_x_handler()
{
    if((rh_state == REC_C_X || rh_state == REC_C_Y) && message_rx[1] == sid) {
        //buffer x coordinate
        c_x = message_rx[0];
        enable_tx = 1;
        message_out(sid, id, M_R_R_REC_C_X_ACK);
        _delay_ms(200);
        rh_state = REC_C_Y;
    }
}

static void att_c_y_handler()
{
    if(rh_state == REC_C_Y && message_rx[1] == sid) {
        enable_tx = 1;
        message_out(sid, id, M_R_R_REC_C_Y_ACK);
        _delay_ms(200);
        //find seeder, where coordinate gets associated with
        struct list_node *ln = exists(get_locations(), &sid, &comp_int_location);
        if(ln == 0) {
            error_state(ATT_C_Y_H_SEEDER_UK);
            return;
        }
        struct location *l = (struct location *) ln->data;
        l->c.x = c_x;
        l->c.y = message_rx[0];
        rh_state = WAITING;
#ifdef DEBUG_LED
        blink_led(&led_orange);
#endif
    }
}

//reference recruitment handler FSM
void rr_handler()
{
    switch(message_rx[2])
    {
    case M_R_R_REC_B_X:
        att_b_x_handler();
        break;
    case M_R_R_REC_C_ATT:
        att_c_handler();
        break;
    case M_R_R_REC_C_X:
        att_c_x_handler();
        break;
    case M_R_R_REC_C_Y:
        att_c_y_handler();
        break;
    }

    if(rh_state == WAITING)
        enable_tx = 0;
    else
        enable_tx = 1;
}


