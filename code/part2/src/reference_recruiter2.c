#include "reference_recruiter2.h"

#define INQ_COUNTER 3

typedef enum {
    RECRUIT_B_X,
    RECRUIT_C_ATT,
    RECRUIT_C_X,
    RECRUIT_C_Y,
    REF_CHOSEN
} RRFSM;

static RRFSM rr_state = RECRUIT_B_X;

static uint8_t c_x;
static uint8_t c_y;

static uint8_t msg_rcvd = 0;

static uint8_t inq_count = 0;

static void rr_recruit_b_x()
{
    //send x coordinate to b (which is the distance)
    message_out(refs->id_b, id, M_R_R_REC_B_X);
    _delay_ms(50);
#ifdef DEBUG_LED
    blink_led(&led_orange);
#endif
    msg_rcvd = receive_message();
    for(int i = 0; i < 20 && msg_rcvd == 0; ++i) {
        get_message();
        msg_rcvd = message_rx[5] == 1 &&
                   message_rx[0] == id &&
                   message_rx[2] == M_R_R_REC_B_ACK &&
                   message_rx[1] == refs->id_b;
        _delay_ms(10);
    }
    if(msg_rcvd == 1 &&
       message_rx[2] == M_R_R_REC_B_ACK && message_rx[1] == refs->id_b) {
        //prepare coordinates for c
        c_x = (uint8_t) (refs->dist_c * cos(refs->alpha));
        c_y = (uint8_t) (((float)refs->dist_c) * sin(refs->alpha));
        rr_state = RECRUIT_C_ATT;
        msg_rcvd = 0;
        inq_count = 0;
    }
}

static void rr_recruit_c_att()
{
    //request c's attention
#ifdef DEBUG_LED
    blink_led(&led_red);
#endif
    message_out(refs->id_c, id, M_R_R_REC_C_ATT);
    _delay_ms(50);
    msg_rcvd = receive_message();
    for(int i = 0; i < 20 && msg_rcvd == 0; ++i) {
        get_message();
        msg_rcvd = message_rx[5] == 1 &&
                   message_rx[0] == id &&
                   message_rx[2] == M_R_R_REC_C_A_ACK;
        _delay_ms(10);
    }
    if(msg_rcvd == 1 && message_rx[2] == M_R_R_REC_C_A_ACK) {
        inq_count = 0;
        rr_state = RECRUIT_C_X;
        msg_rcvd = 0;
    }
}

static void rr_recruit_c_x()
{
    //send x coordinate to c
    message_out(c_x, id, M_R_R_REC_C_X);
    _delay_ms(50);
    msg_rcvd = receive_message();
    for(int i = 0; i < 20 && msg_rcvd == 0; ++i) {
        get_message();
        msg_rcvd = message_rx[5] == 1 &&
                   message_rx[0] == id &&
                   message_rx[2] == M_R_R_REC_C_X_ACK;
        _delay_ms(10);
    }
    if(msg_rcvd == 1 && message_rx[2] == M_R_R_REC_C_X_ACK) {
        inq_count = 0;
        rr_state = RECRUIT_C_Y;
        msg_rcvd = 0;
    }
    if(inq_count++ > INQ_COUNTER) {
        inq_count = 0;
        rr_state = RECRUIT_C_ATT;
        msg_rcvd = 0;
    }
}

static void rr_recruit_c_y()
{
    //send y coordinate to c
    message_out(c_y, id, M_R_R_REC_C_Y);
    _delay_ms(50);
    msg_rcvd = receive_message();
    for(int i = 0; i < 20 && msg_rcvd == 0; ++i) {
        get_message();
        msg_rcvd = message_rx[5] == 1 &&
                   message_rx[0] == id &&
                   message_rx[2] == M_R_R_REC_C_Y_ACK;
        _delay_ms(10);
    }
    if(msg_rcvd == 1 && message_rx[2] == M_R_R_REC_C_Y_ACK) {
        inq_count = 0;
        rr_state = REF_CHOSEN;
        msg_rcvd = 0;
    }
    if(inq_count++ > INQ_COUNTER) {
        inq_count = 0;
        rr_state = RECRUIT_C_X;
        msg_rcvd = 0;
    }
}

static void rr_ref_chosen()
{
    message_out(EMPTY, EMPTY, EMPTY);
    enable_tx = 0;
}

//FSM for recruiting references
uint8_t recruit_reference()
{
    enable_tx = 1;
    switch(rr_state) {
    case RECRUIT_B_X:
        rr_recruit_b_x();
        return 0;
    case RECRUIT_C_ATT:
        rr_recruit_c_att();
        return 0;
    case RECRUIT_C_X:
        rr_recruit_c_x();
        return 0;
    case RECRUIT_C_Y:
        rr_recruit_c_y();
        return 0;
    case REF_CHOSEN:
        rr_ref_chosen();
        return 1;
    }
    return 0;
}
