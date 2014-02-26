#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "../BA2.h"

struct bot {
    struct neighbour *n;
    struct coordinates *c;
    uint8_t ancd;
};

extern void init_localization(void);
extern void handle_acns(void);
extern void announce_location(void);
extern uint8_t fully_announced(void);
extern void traverse_for_loc(void);
extern void finalize_localization(void);

#endif
