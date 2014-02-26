#ifndef BA_H
#define BA_H

#include "../lib/list.h"
#include "../lib/triliteration_small.h"
#include "../lib/libkb.h"
#include "src/seeder_handler.h"
#include "src/reference_recruiter.h"
#include "src/neighbour_handler.h"
#include "src/ref_request_handler.h"
#include "src/save_data.h"

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

#define NEWLINE kprints("        \r\n")

struct seeder {
    uint8_t id;
};

/*
 *   MESSAGES
 *  if not otherwise stated, a message consists of {data, receiver id, message-type}
 */
typedef enum {
    M_ERROR             = (0 << 1),
    M_CHECK_ID          = (1 << 1),
    M_CHANGE_ID         = (2 << 1),
    M_ELECT_SEED_TOP    = (3 << 1),
    M_ELECT_SEED_BOTTOM = (4 << 1),
    M_SEEDER            = (5 << 1),
    M_NOT_ENOUGH_SEEDS  = (6 << 1),
    M_R_R_REQ_ATT       = (7 << 1),
    M_R_R_LISTENING     = (8 << 1),
    M_R_R_REQ_DIST      = (9 << 1),
    M_R_R_DIST          = (10 << 1),
    //M_R_R_BACKOFF       = (11 << 1),
    M_R_R_ACK           = (12 << 1),
    M_R_R_ABORT1        = (13 << 1),
    M_R_R_REC_B         = (14 << 1),
    M_R_R_REC_C_X       = (15 << 1),
    M_R_R_REC_C_Y       = (16 << 1),
    M_R_R_REC_B_ACK     = (17 << 1),
    M_R_R_REC_C_X_ACK   = (18 << 1),
    M_R_R_REC_C_Y_ACK   = (19 << 1),
    M_R_R_REC_B_ATT     = (20 << 1),
    M_R_R_REC_B_A_ACK   = (21 << 1),
    M_R_R_REC_C_ATT     = (22 << 1),
    M_R_R_REC_C_A_ACK   = (16 << 1)
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

/*
 *FSM
 */
typedef enum {
    INIT,
    CONSTRUCT_ID,
    TEST_ID,
    ELECT_SEED_TOP,
    ELECT_SEED_BOTTOM,
    CHECK_SEEDER,
    AWAIT_R_R,
    RECRUIT_REFERENCE,
    SAVE_DATA,
    AWAIT_OTHERS,
    ERROR_STATE
} FSM ;

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
extern FSM get_master_state(void);
extern void error_state(void);

extern struct list *neighbours;
extern uint8_t id;
extern struct list *seeders;
extern struct references *refs;
extern void reset_rh_c(void);

extern int receive_message(void);

#endif
