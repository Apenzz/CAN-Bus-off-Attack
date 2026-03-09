#ifndef CAN_BUS_H
#define CAN_BUS_H

#include "ecu.h"

/* Tx sync constants */
/* at 500 Kbps 1 bit = 2 us
 * Two transmissions happen at the same time if their start time differ by less than one bit time.
*/
#define BIT_TIME_US 2ULL 
#define MAX_BG_MSGS 8 /* maximum background message entries */

/* Bus timing constants at 500 Kbps */
#define FRAME_DURATION_US 200ULL /* One frame transmission window */
#define ERROR_FRAME_US 34ULL /* error flag(6) + delimiter(8) + IFS(3) = 17 bits -> 34 us at 500 Kbps */
#define IFS_US 6ULL /* inter-frame space (3 bits at 500 Kbps)*/
#define MAX_NODES 16

/* CAN bus model and bus-off attack simulation 
 *
 * The adversary (A) targets a victim (V) by injecting an attack message with
 * the same ID as V's periodic message but with DLC=0 and all dominant bits data
 * transmitted at exactly the same time as V's message.
*/

/* CAN frame */
typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
    ECU *sender;
} can_frame_t;

/**
 * Background message
 * 
 * Represents a periodic CAN message sent by a non-victim, non-adversary ECU.
 * The adversary monitors these to locate a preceded ID.
 * The attack time can be derived from:
 * t_attack = t_preceded_id_completion + IFS_US
 */
typedef struct {
    uint16_t id; /* ID of background msg */
    uint64_t period_us; /* Tx period in us */
    uint64_t start_us; /* absolute time of the first tx start */
} bg_msg_t;

/* Simulation record (one row of output data)*/
typedef struct {
    uint64_t time_us; /* simulation clock at this event */
    uint32_t victim_tec;
    uint32_t adv_tec;
    ecu_state_t victim_state;
    ecu_state_t adv_state;
} sim_record_t;

/* CAN bus */
typedef struct {
    ECU *nodes[MAX_NODES];
    int num_nodes;
    uint32_t speed_bps;

    /* Bg traffic registry */
    bg_msg_t bg_msgs[MAX_BG_MSGS];
    int num_bg_msgs;

    /* jitter
     *
     * The victim actual Tx deviates from the nominal value by a uniform random time in
     * [-jitter_max_us, jitter_max_us].
     * If set to 0 reproduce a deterministic attack.
    */
    int64_t jitter_max_us;
} CAN_Bus;


/* === API === */

void bus_init(CAN_Bus *bus, uint32_t speed_bps);

void bus_add_node(CAN_Bus *bus, ECU *ecu);

/**
 * Register a bg periodic message for Tx sync
 * @param start_us Absolute sim time of the message's first Tx
 */
void bus_add_bg_msg(CAN_Bus *bus, uint16_t id, uint64_t period_us, uint64_t start_us);

/**
 * Set uinform jitter on the victim's transmission timing.
 * Each period the victim's actual Tx time = nominal +/- uniform(0, max_us).
 * max_us set to 0 for deterministic mode.
 */
void bus_set_jitter(CAN_Bus *bus, int64_t max_us);

/* Simulate the full iterative bus-off attack. 
 *
 * Records (time, TEC values, states) at every event into the caller supplied array.
 * Returns the number of records written.
 * 
 * @param simulation_duration_us upper bound. Simulation stop if this time is exceeded even before bus-off happens. 
 * @param records Output buffer.
 * @param max_records Capacity of the output buffer.
 * 
*/
int bus_simulate_attack(CAN_Bus *bus, ECU *victim, ECU *adversary, uint64_t simulation_duration_us, sim_record_t *records, int max_records);

void bus_print_summary(const CAN_Bus *bus, FILE *f);
#endif