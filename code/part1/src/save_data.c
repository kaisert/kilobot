/*********************************************************
 * save all the states needed for next part of the program
 *********************************************************/

#include "save_data.h"

static uint8_t write_byte(uint8_t *p, uint8_t data)
{
    if(0x00 <= p && p < 0x100) {
        error_state();
        return 0;
    }
    else {
        eeprom_write_byte(p, data);
    }
    return 1;
}

static uint8_t *save_id(uint8_t *_p)
{
    uint8_t *p = _p;
    if(write_byte(p, id) != 1)
        error_state();
    ++p;
    return p;
}

static uint8_t *check_id(uint8_t *_p)
{
    uint8_t *p = _p;
    uint8_t cid = eeprom_read_byte(p);
    ++p;
    if(cid != id)
        error_state();
    return p;
}

static uint8_t *save_role(uint8_t *_p)
{
    uint8_t *p = _p;
    uint8_t data = (get_role() == SEEDER);
    write_byte(p, data);
    ++p;
    return p;
}

static uint8_t *check_role(uint8_t *_p)
{
    uint8_t *p = _p;
    uint8_t crole = eeprom_read_byte(p);
    ++p;
    if(crole != get_role())
        error_state();
    return p;
}

static uint8_t *save_neighs(uint8_t *_p)
{
    uint8_t *p = _p;
    if(write_byte(p, neighbours->size) != 1)
        error_state();
    ++p;
    struct list_node *cur;
    for(cur = neighbours->head; cur != 0; cur = cur->next) {
        struct neighbour *n = (struct neighbour *) cur->data;
        if(write_byte(p, n->id) != 1)
            error_state();
        ++p;
        if(write_byte(p, n->dist) != 1)
            error_state();
        ++p;
    }
    return p;
}

static uint8_t *check_neighs(uint8_t *_p)
{
    uint8_t *p = _p;
    uint8_t csize = eeprom_read_byte(p);
    ++p;
    if(csize != neighbours->size)
        error_state();
    struct list_node *cur;
    for(cur = neighbours->head; cur != 0; cur = cur->next) {
        struct neighbour *n = (struct neighbour *) cur->data;
        uint8_t cid = eeprom_read_byte(p);
        ++p;
        if(cid != n->id)
            error_state();
        uint8_t cdist = eeprom_read_byte(p);
        ++p;
        if(cdist != n->dist)
            error_state();
    }
    return p;
}

static uint8_t *save_seeders(uint8_t *_p)
{
    uint8_t *p = _p;
    if(write_byte(p, seeders_list()->size) != 1)
        error_state();
    ++p;
    struct list_node *cur;
    for(cur = seeders_list()->head; cur != 0; cur = cur->next) {
        struct seeder *s = (struct seeder *) cur->data;
        if(write_byte(p, s->id) != 1)
            error_state();
        ++p;
    }
    return p;
}

static uint8_t *check_seeders(uint8_t *_p)
{
    uint8_t *p = _p;
    uint8_t csize = eeprom_read_byte(p);
    ++p;
    if(csize != seeders->size)
        error_state();
    struct list_node *cur;
    for(cur = seeders->head; cur != 0; cur = cur->next) {
        struct seeder *s = (struct seeder *) cur->data;
        uint8_t csid = eeprom_read_byte(p);
        ++p;
        if(csid != s->id)
            error_state();
    }
    return p;
}

static uint8_t *save_refs(uint8_t *_p)
{
    uint8_t *p = _p;
    write_byte(p, refs->id_b);
    ++p;
    write_byte(p, refs->id_c);
    ++p;
    write_byte(p, refs->dist_b);
    ++p;
    write_byte(p, refs->dist_c);
    ++p;
    write_byte(p, refs->dist_bc);
    ++p;
    return p;
}

static uint8_t *check_refs(uint8_t *_p)
{
    uint8_t *p = _p;
    uint8_t cidb = eeprom_read_byte(p);
    ++p;
    uint8_t cidc = eeprom_read_byte(p);
    ++p;
    uint8_t cdistb = eeprom_read_byte(p);
    ++p;
    uint8_t cdistc = eeprom_read_byte(p);
    ++p;
    uint8_t cdistbc = eeprom_read_byte(p);
    if(cidb != refs->id_b ||
       cidc != refs->id_c ||
       cdistb != refs->dist_b ||
       cdistc != refs->dist_c ||
       cdistbc != refs->dist_bc)
            error_state();
    return p;
}

void save_data()
{
    uint8_t *p = START_EEPROM;
    p = save_id(p);
    p = save_role(p);
    p = save_neighs(p);
    p = save_seeders(p);
    if(get_role() == SEEDER)
        p = save_refs(p);

    uint8_t *cp = START_EEPROM;
    cp = check_id(cp);
    cp = check_role(cp);
    cp = check_neighs(cp);
    cp = check_seeders(cp);
    if(get_role() == SEEDER)
        cp = check_refs(cp);
}
