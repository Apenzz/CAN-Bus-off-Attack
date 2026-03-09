#include <string.h>
#include "ecu.h"

/* === API === */

void ecu_init(ECU *ecu, uint8_t node_id, const char *name, uint16_t msg_id, uint8_t dlc, uint64_t period_us, uint64_t start_us) {
    memset(ecu, 0, sizeof *ecu);
    ecu->node_id = node_id;
    strncpy(ecu->name, name, sizeof(ecu->name) - 1);
    ecu->msg_id = msg_id;
    ecu->msg_dlc = dlc;
    ecu->tec = 0;
    ecu->rec = 0;
    ecu->state = ECU_STATE_ERROR_ACTIVE;
    ecu->period_us = period_us;
    ecu->next_tx_us = start_us;
    ecu->is_adversary = false;
}

void ecu_make_adversary(ECU *ecu, uint16_t target_id) {
    ecu->is_adversary = true;
    ecu->target_id = target_id;
    ecu->msg_id = target_id; /* math victim's arbitration ID */
    ecu->msg_dlc = 0; /* all dominant bits*/
}

void ecu_set_preceded_id(ECU *ecu, uint16_t id) {
    ecu->use_preceded_id = true;
    ecu->preceded_id = id;
}

void ecu_on_tx_error(ECU *ecu) {
    if (ecu->state == ECU_STATE_BUS_OFF) return;
    ecu->tec += TEC_TX_ERROR_INC;
    ecu_update_state(ecu);
}

void ecu_on_tx_success(ECU *ecu) {
    if (ecu->state == ECU_STATE_BUS_OFF) return;
    if (ecu->tec >= TEC_TX_SUCCESS_DEC)
        ecu->tec -= TEC_TX_SUCCESS_DEC;
    else
        ecu->tec = 0;
    ecu_update_state(ecu);
}

void ecu_on_rx_error(ECU *ecu) {
    if (ecu->state == ECU_STATE_BUS_OFF) return;
    ecu->rec += REC_RX_ERROR_INC;
    ecu_update_state(ecu);
}

void ecu_on_rx_success(ECU *ecu) {
    if (ecu->state == ECU_STATE_BUS_OFF) return;
    if (ecu->rec >= REC_RX_SUCCESS_DEC)
        ecu->rec -= REC_RX_SUCCESS_DEC;
    else
        ecu->rec = 0;
    ecu_update_state(ecu);
}

void ecu_update_state(ECU *ecu) {
    if (ecu->state == ECU_STATE_BUS_OFF) return; /* Terminal state */

    if (ecu->tec > TEC_BUS_OFF_THRESHOLD) {
        ecu->state = ECU_STATE_BUS_OFF;
    } else if (ecu->tec > TEC_ERROR_PASSIVE_THRESHOLD || ecu->rec > REC_ERROR_PASSIVE_THRESHOLD) {
        ecu->state = ECU_STATE_ERROR_PASSIVE;
    } else {
        ecu->state = ECU_STATE_ERROR_ACTIVE;
    }
}

const char *ecu_state_name(ecu_state_t s) {
    switch (s) {
        case ECU_STATE_ERROR_ACTIVE: return "Error Active";
        case ECU_STATE_ERROR_PASSIVE: return "Error Passive";
        case ECU_STATE_BUS_OFF: return "Bus-off";
        default: return "Unknown state";
    }
}

void ecu_print(const ECU *ecu, FILE *f) {
    fprintf(f,
            " ECU[%u] %-12s msg_id=0x%03X dlc=%u "
            "TEC=%-4u REC=%-4u state=%s%s\n",
            ecu->node_id, ecu->name,
            ecu->msg_id, ecu->msg_dlc,
            ecu->tec, ecu->rec,
            ecu_state_name(ecu->state),
            ecu->is_adversary ? " [ADVERSARY]" : "");
}