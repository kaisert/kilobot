/********************************
 *Handling seeders
 ********************************/

#include "seeder_handler.h"

//compare functions
static int _comp_seeders(uint8_t *s1_id, struct seeder *s2) {
    return *s1_id == s2->id;
}

static int comp_seeders(void *s1, void *s2)
{
    return _comp_seeders(s1,s2);
}

void init_seeders(){
    seeders = malloc(sizeof(struct list));
    init_list(seeders);
}

static uint8_t seeder_known(uint8_t i_id)
{
    if(seeders->size == 0)
        return 0;
    return exists(seeders, &i_id, &comp_seeders) != 0;
}

static void add_new_seeder(uint8_t i_id)
{
    struct seeder *new_seeder = malloc(sizeof(struct seeder));
    new_seeder->id = i_id;
    add_node(seeders, new_seeder);
}

void handle_seeder_m(uint8_t id)
{
    if(seeder_known(id) == 0)
        add_new_seeder(id);
}

uint8_t n_of_seeders()
{
    return seeders->size;
}

uint8_t n_of_seeders2() {
    return seeders->size;
}

struct list *seeders_list()
{
    return seeders;
}
