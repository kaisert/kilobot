/**
 **   construction of a global coordinate system, relying only on local information
 **   Bachelor's thesis
 **   Tobias Kaiser, ETH Zuerich / Aalto University, 2013
 **/

#include "BA2.h"

	
/*
 * counts
 */
#define PROP_DELAY_LIMIT 300
#define MAX_DIST 75
#define REF_ELECT_ROUNDS 400
#define SENDING_BACKOFF 25
#define AWAIT_LOC_COUNT 400
#define LOC_ANC_COUNT_MAX 300
#define N_LOC_ROUNDS 6000
#define AWAIT_M_LIMIT 300
#define RESET_COUNTER counter = 0;

/*
 *   DECLARATIONS
 */
uint8_t id;
static FSM2 state = RESTORE_DATA;

static ROLE role = ND;
static struct list neighbours = {0,0,0};
static struct list locations = {0,0,0};

struct references *refs;

static uint16_t counter = 0;

static uint8_t has_recruited = 0;
static uint8_t sending = 0;
static uint8_t old_n_located = 0;
static struct seeder sself;
static struct location lself = {&sself, {0, 0}};
static struct neighbour nself = {0,0,0};
static struct list mergers = {0,0,0};

static ERROR_CODE error_code = -1;

char newline[10] = "        \r\n";

/*
 *  CODE
 */
void led_off()
{
    LED_OFF;
}

void led_white()
{
    LED_WHITE;
}

void led_red()
{
    LED_RED;
}

void led_green()
{
    LED_GREEN;
}

void led_turquoise()
{
    LED_TURQOISE;
}

void led_blue()
{
    LED_BLUE;
}

void led_violet()
{
    LED_VIOLET;
}

void led_orange()
{
    LED_ORANGE;
}

void blink_led(void (*led) (void))
{
    for(uint8_t i=0;i<5;++i){
        LED_OFF;
        BLINK;
        (*led)();
        BLINK;
    }
}

void error_state(ERROR_CODE c)
{
    error_code = c;
    state = ERROR_STATE;
}

struct neighbour *get_nself()
{
    return &nself;
}

struct location *get_lself()
{
    return &lself;
}

struct seeder *get_sself()
{
    return &sself;
}

struct list *get_mergers()
{
    return &mergers;
}

ROLE get_role()
{
    return role;
}

FSM2 get_master_state()
{
    return state;
}

struct list *get_locations()
{
    return &locations;
}

struct list *get_neighbours()
{
    return &neighbours;
}

int receive_message()
{
    get_message();
    if(message_rx[5] == 1 && message_rx[3] <= MAX_DIST) {
        switch(message_rx[2]) {
        case M_ERROR:
            break;
        case M_R_R_REC_B_X:
        case M_R_R_REC_C_ATT:
            if(message_rx[0] == id)
                rr_handler();
            return 2;
        case M_R_R_REC_C_X:
        case M_R_R_REC_C_Y:
            rr_handler();
            return 2;
        case M_R_R_REC_B_ACK:
        case M_R_R_REC_C_A_ACK:
        case M_R_R_REC_C_X_ACK:
        case M_R_R_REC_C_Y_ACK:
            if(state == ELECT_REFERENCES && message_rx[0] == id)
                return 1;
            break;
        case M_A_S:
        case M_A_L_X:
        case M_A_L_X_ONLY:
        case M_A_L_Y:
            if(state >= AWAIT_LOCALIZATION){
                handle_acns();
                return 3;
            }
            break;
        case M_REGISTER_M_G:
            if(message_rx[0] == id)
                fm_register_handler();
            return 4;
        case M_MERGE:
            if(message_rx[0] == id && get_bm_state() == LISTEN && state != FINISHED)
                b_merge_req_handler();
            return 4;
        case M_SEND_V1:
        case M_SEND_V2:
        case M_SEND_V3:
        case M_SEND_V4:
        case M_SEND_V5:
        case M_SEND_V6:
            if(message_rx[1] == id && state != FINISHED)
                s_merge_rec_v_handler();
            return 4;
        case M_SEND_Q1:
        case M_SEND_Q2:
        case M_SEND_Q3:
        case M_SEND_Q4:
        case M_SEND_Q5:
        case M_SEND_Q6:
        case M_SEND_Q7:
        case M_SEND_Q8:
            if(message_rx[1] == id && state != FINISHED)
                s_merge_rec_q_handler();
            return 4;
        case M_PROP_V1:
        case M_PROP_V2:
        case M_PROP_V3:
        case M_PROP_V4:
        case M_PROP_V5:
        case M_PROP_V6:
            p_new_v_handler();
            return 4;
        case M_PROP_Q1:
        case M_PROP_Q2:
        case M_PROP_Q3:
        case M_PROP_Q4:
        case M_PROP_Q5:
        case M_PROP_Q6:
        case M_PROP_Q7:
        case M_PROP_Q8:
            p_new_q_handler();
            return 4;
        default:
            break;
        }
    }
    return 0;
}

void proceed_to_prop()
{
    state = WAIT_FOR_PROP;
}

/*
 *STATE FUNCTIONS
 */
static void state_restore_data()
{
    //initialize all data from BA.c
    refs = malloc(sizeof(struct references));
    uint8_t trole;
    restore_data(&id, &trole, get_neighbours(), get_locations(), refs);
    refs->alpha = angled(refs->dist_b, refs->dist_c, refs->dist_bc);
    if(trole == 1) {
        role = SEEDER;
    } else {
        role = BOT;
        free(refs);
    }
    //initialize yourself
    nself.dist = 0;
    nself.id = id;
    nself.locations = get_locations();
    if(role == SEEDER) {
        sself.n = &nself;
        init_quaternion(&sself.q);
        init_vector(&sself.v);
    }

    sending = id % (SENDING_BACKOFF * 2);
    state = ELECT_REFERENCES;
}

static void state_elect_references()
{
    //to reduce noise, have some backoff
    if(role == SEEDER && has_recruited == 0 && (sending < SENDING_BACKOFF)) {
        receive_message();
        //assign coordinates to B and C
        has_recruited = recruit_reference();
        sending = (sending+1) % (SENDING_BACKOFF * 2);
    } else {
        //if there are no more msgs from this round heard, transit to next state eventually
        if(receive_message() != 2) {
            _delay_ms(5);
            if(counter++ > REF_ELECT_ROUNDS) {
                RESET_COUNTER;
                old_n_located = 0;
                init_localization();
                enable_tx = 1;
                state = AWAIT_LOCALIZATION;
            }
        } else {
            RESET_COUNTER;
        }
        sending = (sending+1) % (SENDING_BACKOFF * 2);
    }
}

static void state_await_localization()
{
    //synchronisation state
    if(receive_message() == 3 || counter++ > AWAIT_LOC_COUNT) {
        RESET_COUNTER;
        state = LOCALIZE;
    }
    _delay_ms(5);
}

static void state_localize()
{
    receive_message();
    //announce new location after certain interval
    if(counter % LOC_ANC_COUNT_MAX == 0)
        announce_location();
    if(old_n_located != fully_announced()) {
        //if there are new localizations possible, do so
#ifdef DEBUG_LED
        blink_led(&led_off);
#endif
        old_n_located = fully_announced();
        traverse_for_loc();
    }
    if(++counter > N_LOC_ROUNDS) {
        finalize_localization();
        RESET_COUNTER;
        state = AWAIT_FORMING_M_G;
        /***DEMO*********/
        //state = FORMING_M_GROUP;
        // \DEMO
    }
    _delay_ms(10);
}


static void state_await_f_m_g()
{
    //wait for neighbours, then start with forming the merging groups
    enable_tx = 0;
    switch(receive_message()) {
    case 0:
        break;
    case 4:
        RESET_COUNTER;
        enable_tx = 1;
        state = FORMING_M_GROUP;
        break;
    default:
        RESET_COUNTER;
        break;
    }
    if(counter++ > AWAIT_M_LIMIT) {
        RESET_COUNTER;
        enable_tx = 1;
        state = FORMING_M_GROUP;
    }
    _delay_ms(5);
}

static void state_forming_m_group()
{
    if(form_m_gs() == 1) {
        state = MERGING;
        /**************************************
         * DEMO
         **************************************/
        //state = FINISHED;
        //enable_tx = 0;
        // \DEMO
    }
    receive_message();
}

static void state_merging()
{
    receive_message();
    if(!(get_role() == SEEDER && s_merge() == 0)) {
        b_merge();
    }
}

static void state_wait_for_prop()
{
    if(get_bm_state() != LISTEN) {
        RESET_COUNTER;
        receive_message();
    }
    get_message();
    if(message_rx[5] == 1) {
         if(message_rx[2] >= M_SEND_V1 && message_rx[2] < M_PROP_V1) {
             RESET_COUNTER;
         } else if(message_rx[2] >= M_PROP_V1) {
             RESET_COUNTER;
             state = PROPAGATE;
         }
    }
    if(++counter >= PROP_DELAY_LIMIT) {
        RESET_COUNTER;
        state = PROPAGATE;
    }
    _delay_ms(5);
}

static void state_propagate()
{
    //prop_delay_counter++;
    counter++;
    receive_message();
    if(propagate() == 1) {
        enable_tx = 0;
        get_message();
        if(message_rx[5] == 1) {
            if(message_rx[2] >= M_PROP_V1) {
                RESET_COUNTER;
                //prop_delay_counter = 0;
            }
            else if(message_rx[2] < M_PROP_V1) {
                RESET_COUNTER;
                state = FINISHED;
            }
        }
        //if(prop_delay_counter++ >= PROP_DELAY_LIMIT) {
        if(counter++ >= PROP_DELAY_LIMIT) {
            /***************
             *TEST
             **************/
            RESET_COUNTER;
            state = FINISHED;

            /*state = MERGING;
            init_new_round();*/
        }
    }
}

/******************************
 * DEMO
 *****************************/

void state_demo()
{
    enable_tx = 0;
    switch(get_m_groups()->size) {
    case 0: LED_OFF; break;
    case 1: LED_BLUE; break;
    case 2: LED_GREEN; break;
    case 3: led_orange(); break;
    case 4: led_violet(); break;
    case 5: LED_RED; break;
    case 6: LED_WHITE; break;
    default: LED_TURQOISE; break;
    }
    _delay_ms(500);
    LED_OFF;
    _delay_ms(10);
}

void user_program(void)
{    
    switch(state){
    case RESTORE_DATA:
        state_restore_data();
        break;
    case ELECT_REFERENCES:
        state_elect_references();
        break;
    case AWAIT_LOCALIZATION:
        state_await_localization();
        break;
    case LOCALIZE:
        state_localize();
        /*NEWLINE;
        struct list_node *nn;
        struct neighbour *n;
        struct list_node *ln;
        struct location *l;
        for(nn = get_neighbours()->head; nn != 0; nn = nn->next) {
            n = (struct neighbour *) nn->data;
            kprints("neigh     ");
            NEWLINE;
            kprinti(n->id);
            NEWLINE;
            for(ln = n->locations->head; ln != 0; ln = ln->next) {
                l = (struct location *) ln->data;
                kprints("s id:     ");
                kprinti(l->s->n->id);
                kprinti(l->c.x);
                kprinti(l->c.y);
                NEWLINE;
            }
        }
        NEWLINE;*/
        break;
    case AWAIT_FORMING_M_G:
        state_await_f_m_g();
        break;
    case FORMING_M_GROUP:
        state_forming_m_group();
        break;
    case MERGING:
        state_merging();
        break;
        /*receive_message();
        struct list_node *nmg;
        struct m_group *mg;
        for(nmg = get_m_groups()->head; nmg != 0; nmg = nmg->next) {
            mg = (struct m_group *) nmg->data;
            NEWLINE;
            kprints("n1:    n2:");
            kprinti(mg->n1);
            kprinti(mg->n2);
            NEWLINE;
            kprints("s1:    s2:");
            kprinti(mg->s1);
            kprinti(mg->s2);
            NEWLINE;
        }
        if(get_role() == SEEDER) {
            kprints("mergers   ");
            NEWLINE;
            struct list_node *nn;
            for(nn = mergers.head; nn != 0; nn = nn->next) {
                kprinti(((struct neighbour *) nn->data)->id);
            }
            NEWLINE;
        }
        break;
    case ERROR_STATE:
        NEWLINE;
        kprints("e_code    ");
        NEWLINE;
        kprinti(error_code);
        NEWLINE;
        break;
    default:
        break;*/
    case WAIT_FOR_PROP:
        state_wait_for_prop();
        break;
    case PROPAGATE:
        state_propagate();
        break;
    case FINISHED:
        receive_message();
        /*if(role == SEEDER) {
            kprints("v:        ");
            kprinti(sself.v.v16b[0]);
            kprinti(sself.v.v16b[1]);
            kprinti(sself.v.v16b[2]);
            NEWLINE;
            kprints("q:        ");
            kprinti(sself.q.q16b[0]);
            kprinti(sself.q.q16b[1]);
            kprinti(sself.q.q16b[2]);
            kprinti(sself.q.q16b[3]);
            NEWLINE;
        }*/
        //state_demo();
        break;
    }
#ifdef DEBUG_MSG
    NEWLINE;
    kprints("id:  state");
    NEWLINE;
    kprinti(id);
    kprinti(state);
    NEWLINE;
    if(role == SEEDER) {
        kprints("seeder    ");
        NEWLINE;
    }
    kprints("neighbours");
    kprinti(get_neighbours()->size);
    NEWLINE;
    struct neighbour *nd;
    struct list_node *cur;
    for(cur = get_neighbours()->head; cur != 0;  cur = cur->next) {
        nd = (struct neighbour *) cur->data;
        kprinti(nd->id);
    }
    NEWLINE;
    kprints("seeders   ");
    kprinti(locations.size);
    NEWLINE;
    struct seeder *s;
    struct location *l;
    for(cur = locations.head ;cur != 0; cur = cur->next) {
        l = (struct location *) cur->data;
        s = l->s;
        kprinti(s->n->id);
        /*NEWLINE;
        kprints("x:        ");
        kprinti(l->c.x);
        kprints("y:        ");
        kprinti(l->c.y);
        NEWLINE;*/
        NEWLINE;
        kprints("v:      q:");
        kprinti(s->v.v16b[0]);
        kprinti(s->v.v16b[1]);
        kprinti(s->v.v16b[2]);
        NEWLINE;
        kprinti(s->q.q16b[0]);
        kprinti(s->q.q16b[1]);
        kprinti(s->q.q16b[2]);
        kprinti(s->q.q16b[3]);
        NEWLINE;
    }
    NEWLINE;
#endif
#ifdef DEBUG_LED
    if(state != FINISHED) {
        switch(role) {
        case ND:
            LED_WHITE;
            break;
        case SEEDER:
            LED_RED;
            break;
        case BOTTOM_SEEDER:
            LED_ORANGE;
            break;
        case BOT:
            LED_GREEN;
            break;
        default:
            LED_BLUE;
            break;
        }
    }

    switch (state) {
    case AWAIT_LOCALIZATION:
    case FORMING_M_GROUP:
        LED_VIOLET;
        break;
    case LOCALIZE:
        LED_BLUE;
        break;
    case AWAIT_FORMING_M_G:
        LED_TURQOISE;
        break;
    case MERGING:
        //LED_OFF;
        break;
    case WAIT_FOR_PROP:
        LED_WHITE;
        break;
    case PROPAGATE:
        LED_ORANGE;
        break;
    case FINISHED:
        LED_GREEN;
        _delay_ms(300);
        LED_BLUE;
        _delay_ms(300);
        LED_TURQOISE;
        _delay_ms(300);
        break;
    case ERROR_STATE:
        LED_RED;
        _delay_ms(300);
        LED_ORANGE;
        _delay_ms(300);
    default:
        break;
    }
#endif
}

int main(void)
{
  init_robot();
  main_program_loop(user_program);
  return 0;
}
