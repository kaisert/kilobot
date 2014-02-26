#ifndef SEEDER_H
#define SEEDER_H

#include "../part2/BA2.h"

union vector {
    uint16_t v16b[3];
    uint8_t v8b[6];
};

union quaternion {
    int16_t q16b[4];
    uint8_t q8b[8];
};

struct coordinates {
    int8_t x;
    int8_t y;
};

struct seeder {
    struct neighbour *n;
    union vector v;
    union quaternion q;
};

struct location {
    struct seeder *s;
    struct coordinates c;
};

extern int comp_seeders(void *, void *);
extern int comp_int_seeder(void *, void *);
extern int comp_int_location(void *, void *);
extern int comp_seeder_location(void *, void *);
extern int comp_id_location(void *, void *);
extern struct seeder *new_seeder(uint8_t *sid);
extern struct location *next_located(struct location *);
extern void init_quaternion(union quaternion *);
extern void init_vector(union vector *);

#endif
