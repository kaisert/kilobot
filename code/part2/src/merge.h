#ifndef MERGE_H
#define MERGE_H

#include "../BA2.h"

typedef enum {
    LISTEN,
    SEND_V,
    SEND_Q
} BM_FSM;

extern BM_FSM get_bm_state(void);
extern uint8_t s_merge(void);
extern uint8_t propagate(void);
extern void init_new_round(void);
extern void b_merge(void);
extern void b_merge_req_handler(void);
extern void s_merge_rec_q_handler(void);
extern void s_merge_rec_v_handler(void);
extern void p_new_q_handler(void);
extern void p_new_v_handler(void);

#endif
