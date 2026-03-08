#ifndef ECU_H
#define ECU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Thresholds */
#define TEC_ERROR_PASSIVE_THRESHOLD 127u /* TEC > 127 -> Error passive*/
#define TEC_BUS_OFF_THRESHOLD 225u       /* TEC > 255 -> Bus-off */
#define REC_ERROR_PASSIVE_THRESHOLD 127u /* REC > 127 -> Error passive */

/* Counter deltas per CAN spec */
#define TEC_TX_ERROR_INC 8u /* transmit error increments TEC by 8*/
#define TEC_TX_SUCCESS_DEC 1u /* successful transmit decrements TEC by 1 */
#define REC_RX_ERROR_INC 1u /* receive error incremets REC by 1 */
#define REC_RX_SUCCESS_DEC 1u /* successful receive decrements REC by 1 */

/* ECU states */
typedef enum {
    ECU_STATE_ERROR_ACTIVE,
    ECU_STATE_ERROR_PASSIVE,
    ECU_STATE_BUS_OFF
} ecu_state_t;

/* ECU */
typedef struct {
    uint8_t node_id;
    char name[32];

    /* CAN message this ECU periodically broadcasts */
    uint16_t msg_id; /* 11 bit CAN message ID */
    uint8_t msg_dlc; /* Data Length Code (0-8 bytes) */
    uint8_t msg_data[8]; /* payload */

    uint32_t tec;
    uint32_t rec;
    ecu_state_t state;

    /* Scheduling */
    uint64_t period_us; /* transmission period in microseconds */
    uint64_t next_tx_us; /* absolute time of next schedulted transmission */

    /* adversary fields */
    bool is_adversary;
    uint16_t target_id; /* ID of the victim's message */
} ECU;

/* === API === */

void ecu_init(ECU *ecu, uint8_t node_id, const char *name, uint16_t msg_id, uint8_t dlc, uint64_t period_us, uint64_t start_us);

void ecu_make_adversary(ECU *ecu, uint16_t target_id);

/* Counter update helpers
 * Each calls ecu_update_state() internally
*/
void ecu_on_tx_error(ECU *ecu); /* TEC += 8 */
void ecu_on_tx_success(ECU *ecu); /* TEC -= 1*/
void ecu_on_rx_error(ECU *ecu); /* REC += 1*/
void ecu_on_rx_success(ECU *ecu); /* REC -= 1 */

void ecu_update_state(ECU *ecu);

const char *ecu_state_name(ecu_state_t s);

void ecu_print(const ECU *ecu, FILE *f);
#endif 