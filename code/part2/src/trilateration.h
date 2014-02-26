#ifndef TRILATERATION_H
#define TRILATERATION_H

#include "../BA2.h"
//#include <stdint.h>

struct local_bots {
    struct neighbour *e;
    struct neighbour *f;
    struct location *el;
    struct location *fl;
    struct seeder *s;
};

/*struct seeder {
     uint8_t id;
     uint8_t dist;
     uint8_t located;
     struct coordinates *coords;
};
 
struct bot {
     struct neighbour *n;
     struct coordinates *c;
     uint8_t ancd;
};
 
struct neighbour {
     uint8_t id;
     uint8_t dist;
};
*/

extern int anglei(int , int , int );
extern float angled(int, int, int);
extern void trilaterate(struct local_bots *);

#endif
