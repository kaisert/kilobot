#ifndef BA2_H
#define BA2_H

#include "../lib/list.h"
#include "../lib/libkb.h"
#include "../lib/seeder.h"
#include "src/restore_data.h"
#include "src/reference_handler2.h"
#include "src/reference_recruiter2.h"
#include "src/trilateration.h"
#include "src/localization.h"
#include "src/form_merging.h"
#include "src/merge.h"



/*
 *   UTILS
 */
#define BLINK _delay_ms(25);
#define BREAK _delay_ms(30);
#define PAUSE _delay_ms(250);
#define LED_OFF      set_color(0,0,0)
#define LED_ON       set_color(2,2,2)
#define LED_WHITE    set_color(3,3,3)
#define LED_RED      set_color(3,0,0)
#define LED_ORANGE   set_color(3,3,0)
#define LED_GREEN    set_color(0,3,0)
#define LED_TURQOISE set_color(0,3,3)
#define LED_BLUE     set_color(0,0,3)
#define LED_VIOLET   set_color(3,0,3)

#define COORD_UINIT 0x7f

/*
 * DEBUG FLAG
 */
#define DEBUG
#define DEBUG_LED
#define DEBUG_MSG 3000
#define DEBUG_MSG_INC_MSG

/*
 *ERROR CODE
 */
#define E_NO_NEW_MESSAGE 0
#define DISTANCE_E 4

#define EMPTY 0x00

#define NEWLINE kprints(newline);

/*
 *   MESSAGES
 *  if not otherwise stated, a message consists of {data, receiver id, message-type}
 */
typedef enum {
    M_ERROR             = 0,
    M_R_R_REC_B_X       = (1 << 1),
    M_R_R_REC_C_X       = (2 << 1),
    M_R_R_REC_C_Y       = (3 << 1),
    M_R_R_REC_B_ACK     = (4 << 1),
    M_R_R_REC_C_X_ACK   = (5 << 1),
    M_R_R_REC_C_Y_ACK   = (6 << 1),
    M_R_R_REC_C_ATT     = (7 << 1),
    M_R_R_REC_C_A_ACK   = (8 << 1),
    M_A_S               = (9 << 1),
    M_A_L_X_ONLY        = (10 << 1),
    M_A_L_X             = (11 << 1),
    M_A_L_Y             = (12 << 1),
    M_MERGE             = (13 << 1),
    M_REGISTER_M_G      = (14 << 1),
    M_SEND_V1           = (15 << 1),
    M_SEND_V2           = (16 << 1),
    M_SEND_V3           = (17 << 1),
    M_SEND_V4           = (18 << 1),
    M_SEND_V5           = (19 << 1),
    M_SEND_V6           = (20 << 1),
    M_SEND_Q1           = (21 << 1),
    M_SEND_Q2           = (22 << 1),
    M_SEND_Q3           = (23 << 1),
    M_SEND_Q4           = (24 << 1),
    M_SEND_Q5           = (25 << 1),
    M_SEND_Q6           = (26 << 1),
    M_SEND_Q7           = (27 << 1),
    M_SEND_Q8           = (28 << 1),
    M_PROP_V1           = (29 << 1),
    M_PROP_V2           = (30 << 1),
    M_PROP_V3           = (31 << 1),
    M_PROP_V4           = (32 << 1),
    M_PROP_V5           = (33 << 1),
    M_PROP_V6           = (34 << 1),
    M_PROP_Q1           = (35 << 1),
    M_PROP_Q2           = (36 << 1),
    M_PROP_Q3           = (37 << 1),
    M_PROP_Q4           = (38 << 1),
    M_PROP_Q5           = (39 << 1),
    M_PROP_Q6           = (40 << 1),
    M_PROP_Q7           = (41 << 1),
    M_PROP_Q8           = (42 << 1)
} MESSAGE;

/*
 *ROLES
 */
typedef enum {
    ND,
    SEEDER,
    REF,
    BOTTOM_SEEDER,
    BOT
} ROLE;

typedef enum {
    MALLOC_0,
    MALLOC_0_FMG,
    ATT_B_X_H_SEEDER_UK,
    ATT_C_Y_H_SEEDER_UK,
    LOC_H_A_S_NEIGH_UK,
    SEEDER_NO_MATCHING_N,
    TRI_LBS_NOT_LOC,
    TRI_SEEDER_NO_MATCH,
    FM_UNDEFINED_N
} ERROR_CODE;

/*
 *FSM
 */
typedef enum {
    ERROR_STATE,
    RESTORE_DATA,
    ELECT_REFERENCES,
    AWAIT_LOCALIZATION,
    LOCALIZE,
    AWAIT_FORMING_M_G,
    FORMING_M_GROUP,
    MERGING,
    WAIT_FOR_PROP,
    PROPAGATE,
    FINISHED
} FSM2;

extern char newline[10];
extern void led_off(void);
extern void led_white(void);
extern void led_red(void);
extern void led_green(void);
extern void led_turquoise(void);
extern void led_blue(void);
extern void led_violet(void);
extern void led_orange(void);
extern void blink_led(void (*)(void));
extern ROLE get_role(void);
extern FSM2 get_master_state(void);
extern struct neighbour *get_nself(void);
extern struct location *get_lself(void);
extern struct seeder *get_sself(void);
extern struct list *get_locations(void);
extern struct list *get_mergers(void);
extern void error_state(ERROR_CODE);
extern void proceed_to_prop(void);

extern struct list *get_neighbours(void);
extern uint8_t id;
extern struct references *refs;

extern void add_coords(int , int , int , uint8_t);
extern int receive_message(void);

#endif
