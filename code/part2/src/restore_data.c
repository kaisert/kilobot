#include "restore_data.h"

static uint8_t *restore_id(uint8_t *_p, uint8_t *tid)
{
    uint8_t *p = _p;
    *tid = eeprom_read_byte(p);
    ++p;
    return p;
}

static uint8_t *restore_role(uint8_t *_p, uint8_t *trole)
{
    uint8_t *p = _p;
    *trole = eeprom_read_byte(p);
    ++p;
    return p;
}

static uint8_t *restore_neighs(uint8_t *_p, struct list *ns)
{
    uint8_t *p = _p;
    uint8_t size = eeprom_read_byte(p);
    ++p;
    for(int i = 0; i < size; ++i) {
        struct neighbour *n = malloc(sizeof(struct neighbour));
        n->locations = malloc(sizeof(struct list));
        init_list(n->locations);
        n->id = eeprom_read_byte(p);
        ++p;
        n->dist = eeprom_read_byte(p);
        ++p;
        add_node(ns, n);
    }
    return p;
}

static uint8_t *restore_locations(uint8_t *_p, struct list *ls)
{
    uint8_t *p = _p;
    uint8_t size = eeprom_read_byte(p);
    ++p;
    for(int i = 0; i < size; ++i) {
        uint8_t sid = eeprom_read_byte(p);
        ++p;
        struct seeder *s = new_seeder(&sid);
        struct location *l = malloc(sizeof (struct location));
        l->s = s;
        l->c.x = COORD_UINIT;
        l->c.y = COORD_UINIT;
        add_node(ls, l);
    }
    return p;
}

static uint8_t *restore_refs(uint8_t *_p, struct references *r)
{
    uint8_t *p = _p;
    r->id_b = eeprom_read_byte(p);
    ++p;
    r->id_c = eeprom_read_byte(p);
    ++p;
    r->dist_b = eeprom_read_byte(p);
    ++p;
    r->dist_c = eeprom_read_byte(p);
    ++p;
    r->dist_bc = eeprom_read_byte(p);
    return p;
}

void restore_data(uint8_t *tid, uint8_t *trole, struct list *ns, struct list *ss, struct references *r) {
    uint8_t *p = START_EEPROM;
    p = restore_id(p, tid);
    p = restore_role(p, trole);
    p = restore_neighs(p, ns);
    p = restore_locations(p, ss);
    if(*trole == SEEDER)
        restore_refs(p, r);
}
