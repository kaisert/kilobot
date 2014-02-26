/**
 **   construction of a global coordinate system, relying only on local information
 **   Bachelor's thesis
 **   Tobias Kaiser, ETH Zuerich / Aalto University, 2013
 **/

#include "BA.h"

	
/*
 * counts
 */
#define CHECK_ID_COUNT 1000
#define ELECT_SEEDER_COUNT_T 50
#define ELECT_SEEDER_COUNT_B 200
#define CHECK_SEEDER_COUNT 300
#define AWAIT_R_R_COUNT 800
#define MIN_SEEDERS 2
#define MAX_DIST 75
#define RECRUIT_TIMEOUT 50
#define RECRUIT_CONT 50
#define RH_RESET_MAX 100
#define RESET_COUNTER counter = 0


/*
 *   DECLARATIONS
 */
static uint16_t rec_counter = 0;
static uint16_t counter = 0;
static uint16_t rh_reset_c = 0;
uint8_t id;
static FSM state = INIT;

static ROLE role;
struct list *neighbours;
struct list *seeders;
struct references *refs;

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

void error_state()
{
    state = ERROR_STATE;
}

ROLE get_role()
{
    return role;
}

FSM get_master_state()
{
    return state;
}

void reset_rh_c()
{
    rh_reset_c = 0;
}

static void compute_rseed()
{
    int16_t seed = 0;
    seed = (get_ambient_light() + measure_voltage()) * get_ambient_light();
    srand(seed);
    message_out(rand(), rand(), rand());
    get_message();
    seed += message_rx[0] + message_rx[1] * message_rx[2] + (message_rx[3] << 2);
    srand(seed);
    message_out(rand(), rand(), rand());
    get_message();
    seed += message_rx[0] - message_rx[1] * message_rx[2] + (message_rx[3] << 2);
    srand(seed);
}

int receive_message()
{
    get_message();
    if(message_rx[5] == 1 && message_rx[3] <= MAX_DIST) {
        switch(message_rx[2]) {
        case M_ERROR:
            break;
        case M_CHECK_ID:
            if(state < RECRUIT_REFERENCE) {
                if(check_id_handler ((uint8_t) message_rx[1], message_rx[3]) == 2)
                    state = CONSTRUCT_ID;
            }
            break;
        case M_CHANGE_ID:
            if(message_rx[1] == id){
                //reset, if I have to change ID
                RESET_COUNTER;
                message_out(EMPTY, id, M_CHANGE_ID);
                _delay_ms(250);
                state = CONSTRUCT_ID;
            } else {
                //drop the double id
                uint8_t faulty_id = message_rx[1];
                struct list_node *n;
                if((n = exists(neighbours, &faulty_id, &comp_id_neigh)) != 0)
                    remove_node(neighbours, n);
            }
            break;
        case M_ELECT_SEED_TOP:
            if(state == ELECT_SEED_TOP)
                return 1;
            break;
        case M_ELECT_SEED_BOTTOM:
            if(state == ELECT_SEED_BOTTOM)
                return 1;
            break;
        case M_NOT_ENOUGH_SEEDS:
            //if neighbour needs more seeders, new bottom election
            if((role != SEEDER) &&
                    (state == AWAIT_R_R || state == CHECK_SEEDER)) {
                role = BOTTOM_SEEDER;
                state = ELECT_SEED_BOTTOM;
                RESET_COUNTER;
            }
            break;
        case M_SEEDER:
            handle_seeder_m(message_rx[1]);
            break;
        case M_R_R_REQ_ATT:
        case M_R_R_ACK:
        case M_R_R_REQ_DIST:
            if(state >= RECRUIT_REFERENCE && message_rx[0] == id)
                request_handler();
            break;
        case M_R_R_LISTENING:
            if(message_rx[0] == id && state == RECRUIT_REFERENCE)
                return 1;
            break;
        case M_R_R_DIST:
            if(message_rx[1] == id && state == RECRUIT_REFERENCE)
                return 1;
            break;
        case M_R_R_ABORT1:
            if(message_rx[1] == id && state == RECRUIT_REFERENCE)
                return 1;
            break;
        }

    } else {
        return E_NO_NEW_MESSAGE;
    }
	return 0;
}


/*
 *STATE FUNCTIONS
 */

static void state_init()
{
    //initialize lists
    neighbours = malloc(sizeof(struct list));
    init_seeders();
    init_list(neighbours);
    refs = malloc(sizeof(struct references));

    role = ND;
    //set seed
    compute_rseed();
    state = CONSTRUCT_ID;
}

static void state_construct_id()
{
    id = (uint8_t) (rand() & (0xFF));
    RESET_COUNTER;
    state = TEST_ID;
}

static void state_test_id()
{
    //broadcast id, check all received ids for doubles
    message_out(EMPTY, id, M_CHECK_ID);
    receive_message();
    if(counter >= CHECK_ID_COUNT){
        state = ELECT_SEED_TOP;
        RESET_COUNTER;
        if(neighbours->size < 2)
            error_state();
    }
    ++counter;
}

static void state_elect_seeder_top()
{
    //broadcast id for seeder election
    message_out(EMPTY, id, M_ELECT_SEED_TOP);
    BREAK;
    if(receive_message() == 1) {
        //if greater id is received -> do not become top seeder
        if(message_rx[1] > id)
            role = BOTTOM_SEEDER;
    }
    if(counter >= ELECT_SEEDER_COUNT_T) {
        //after ELECT_SEEDER_COUNT_T rounds, if role is still ND, become top seeder
        if(role == ND)
            role = SEEDER;
        state = ELECT_SEED_BOTTOM;
        RESET_COUNTER;
#ifdef DEBUG_LED
        blink_led(&led_turquoise);
#endif
    }
    ++counter;
}

static void state_elect_seeder_bottom()
{
    enable_tx = 1;
    if(role == SEEDER) {
        //top seeders do not participate
        RESET_COUNTER;
        state = CHECK_SEEDER;
        return;
    } else if(role == BOTTOM_SEEDER) {
        //broadcast id, if greater id is encountered, become a usual bot
        message_out(EMPTY, id, M_ELECT_SEED_BOTTOM);
        if(receive_message() == 1) {
            if(message_rx[2] == M_ELECT_SEED_BOTTOM && message_rx[1] > id) {
                role = BOT;
            }
        } else {
            _delay_ms(10);
        }
    }
    if(counter >= ELECT_SEEDER_COUNT_B) {
        //after ELECT_SEEDER_COUNT_B rounds, if role is not BOT, become seeder
        RESET_COUNTER;
        if(role == BOTTOM_SEEDER)
            role = SEEDER;
        state = CHECK_SEEDER;
    }
    _delay_ms(5);
    ++counter;
}

static void state_check_seeder()
{
    receive_message();
    if(role == SEEDER) {
        //propagate seederness
        message_out(EMPTY, id, M_SEEDER);
    } else {
        enable_tx = 0;
        message_out(EMPTY, EMPTY, EMPTY);
    }
    _delay_ms(10);
    ++counter;
    if(counter >= CHECK_SEEDER_COUNT) {
        //after CHECK_SEEDER_COUNT rounds, check, whether enough seeders are visible
        RESET_COUNTER;
        if(n_of_seeders() < (MIN_SEEDERS - (role == SEEDER))) {
            //if not, provoke new bottom election
            enable_tx = 1;
            message_out(EMPTY, EMPTY, M_NOT_ENOUGH_SEEDS);
            _delay_ms(100);
            state = ELECT_SEED_BOTTOM;
            if(role != SEEDER)
                role = BOTTOM_SEEDER;
        } else {
            enable_tx = 1;
            state = AWAIT_R_R;
            RESET_COUNTER;
        }
    }
}

static void state_await_r_r()
{
    //synchronization state
    receive_message();
    if(role == SEEDER) {
        message_out(EMPTY, id, M_SEEDER);
    } else {
        //enable_tx = 0;
    }
    ++counter;
    _delay_ms(10);
    if(counter >= AWAIT_R_R_COUNT) {
        //statetransition after AWAIT_R_R_COUNT rounds
        RESET_COUNTER;
        init_rh();
        rec_counter = 0;
        state = (role == SEEDER) ? RECRUIT_REFERENCE : SAVE_DATA;
#ifdef DEBUG_LED
        blink_led(&led_turquoise);
#endif
    }
}

static void state_recruit_reference()
{
    receive_message();
    //for non-seeders, there is nothing to do, except listening
    if(role == SEEDER) {
        if(rec_counter < (RECRUIT_TIMEOUT)) {
            enable_tx = 1;
            if(recruit_reference() == 1) {
                //when recruitment terminated -> state transition
                state = SAVE_DATA;
                message_out(EMPTY, EMPTY, EMPTY);
                enable_tx = 0;
            }
        } else if(rec_counter >= RECRUIT_TIMEOUT && rec_counter <= (RECRUIT_TIMEOUT + RECRUIT_CONT)) {
            enable_tx = 0;
        } else if(rec_counter >= (RECRUIT_TIMEOUT + RECRUIT_CONT + (id % RECRUIT_TIMEOUT))) {
            rec_counter = 0;
            enable_tx = 1;
        }
        ++rec_counter;
    }
    if(rh_reset_c++ >= RH_RESET_MAX) {
        rh_reset();
        rh_reset_c = 0;
    }
}

static void state_save_data()
{
    LED_ORANGE;
    save_data();
    state = (role == SEEDER) ? AWAIT_OTHERS : RECRUIT_REFERENCE;
}

//main program loop
void user_program(void)
{    
    switch(state){
    case INIT:
        state_init();
        break;
    case CONSTRUCT_ID:
        state_construct_id();
        break;
    case TEST_ID:
        state_test_id();
        break;
    case ELECT_SEED_TOP:
        state_elect_seeder_top();
        break;
    case ELECT_SEED_BOTTOM:
        state_elect_seeder_bottom();
        break;
    case CHECK_SEEDER:
        state_check_seeder();
        break;
    case AWAIT_R_R:
        state_await_r_r();
        break;
    case RECRUIT_REFERENCE:
        state_recruit_reference();
        break;
    case SAVE_DATA:
        state_save_data();
        break;
    case AWAIT_OTHERS:
        receive_message();
        if(rh_reset_c++ >= RH_RESET_MAX) {
            //if seeder is not responsive, reset rh state
            rh_reset();
            rh_reset_c = 0;
        }
        break;
    case ERROR_STATE:
        break;
    }
#ifdef DEBUG_LED
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
    switch(state){
    case INIT:
    case CONSTRUCT_ID:
    case TEST_ID:
        LED_TURQOISE;
        break;
    case ELECT_SEED_TOP:
    case ELECT_SEED_BOTTOM:
        break;
    case CHECK_SEEDER:
        if(counter % 50 >= 25)
            LED_BLUE;
        break;
    case SAVE_DATA:
        LED_OFF;
        break;
    case AWAIT_OTHERS:
        if(role != BOT)
            LED_OFF;
        break;
    case ERROR_STATE:
        LED_RED;
        _delay_ms(300);
        LED_ORANGE;
        _delay_ms(300);
        break;
    default:
        break;
    }
#endif
#ifdef DEBUG_MSG
    NEWLINE;
    kprints("id:       ");
    kprinti(id);
    NEWLINE;
    kprints("state     ");
    kprinti(state);
    NEWLINE;
    kprints("neighbours");
    kprinti(neighbours->size);
    NEWLINE;
    struct neighbour *nd;
    struct list_node *cur;
    for(cur = neighbours->head; cur != 0;  cur = cur->next) {
        nd = (struct neighbour *) cur->data;
        kprinti(nd->id);
    }
    NEWLINE;
    kprints("seeders   ");
    kprinti(n_of_seeders());
    NEWLINE;
    struct seeder *s;
    for(cur = seeders_list()->head ;cur != 0; cur = cur->next) {
        s = (struct seeder *) cur->data;
        kprinti(s->id);
    }
    NEWLINE;
    kprints("enable_tx ");
    kprinti(enable_tx);
    NEWLINE;
#endif
#ifndef DEBUG_MSG
    _delay_ms(5);
#endif
}

int main(void)
{
  init_robot();
  main_program_loop(user_program);
  return 0;
}
