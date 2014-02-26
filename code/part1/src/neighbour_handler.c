#include "neighbour_handler.h"

void add_neighbour(uint8_t id, uint8_t dist)
{
    struct neighbour *n = malloc(sizeof(struct neighbour));
    n->dist = dist;
    n->id = id;
    add_node(neighbours, n);
}

uint8_t known_id(uint8_t newId, int newDist)
{
    if(newId == id)
        return 3;
    struct neighbour *cur;
    struct list_node *it;
    for(it = neighbours->head; it != 0; it = it->next) {
        cur = (struct neighbour *) it->data;
        if(newId == cur->id &&
                ((newDist <= (cur->dist + DISTANCE_E)) || newDist >= (cur->dist - DISTANCE_E))) {
            return 1;
        } else if(newId == cur->id) {
            return 2;
        }
    }
    return 0;
}

void remove_neighbour(uint8_t i_id){
    struct list_node *n;
    if((n = exists(neighbours, &i_id, &comp_id_neigh)) != 0)
        remove_node(neighbours, n);
}

int check_id_handler(uint8_t i_id, uint8_t i_dist)
{
    switch(known_id(i_id, i_dist)) {
    case 0:
        add_neighbour(i_id,i_dist);
        break;
    case 1:
        break;
    case 2:
        remove_neighbour(i_id);
        message_out(EMPTY, i_id, M_CHANGE_ID);
        _delay_ms(250);
        break;
    case 3:
        message_out(EMPTY, i_id, M_CHANGE_ID);
        _delay_ms(250);
        return 2;
    }
    return 1;
}
