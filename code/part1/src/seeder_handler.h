#ifndef SEEDER_HANDLER_H
#define SEEDER_HANDLER_H

#include "../BA.h"

extern void handle_seeder_m(uint8_t);
extern struct list *seeders_list();
extern uint8_t n_of_seeders();
extern uint8_t n_of_seeders2();
extern void init_seeders();

#endif
