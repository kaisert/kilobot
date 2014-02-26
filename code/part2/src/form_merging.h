#ifndef FORM_MERGING_H
#define FORM_MERGING_H

#include "../BA2.h"

struct m_group {
    uint8_t s1;
    uint8_t s2;
    uint8_t n1;
    uint8_t n2;
};

extern uint8_t form_m_gs(void);
extern void seeing_resp_handler(void);
extern void seeing_req_handler(void);
extern struct list *get_m_groups(void);
extern void fm_register_handler(void);

#endif
