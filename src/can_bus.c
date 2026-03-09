#include "can_bus.h"
#include <string.h>
#include <stdlib.h>

static int push_record(sim_record_t *records, int *count, int max, uint64_t t, const ECU *v, const ECU *a) {
    if (*count >= max) return 0;
    records[*count].time_us = t;
    records[*count].victim_tec = v->tec;
    records[*count].adv_tec = a->tec;
    records[*count].victim_state = v->state;
    records[*count].adv_state = a->state;
    (*count)++;
    return 1;
}

void bus_init(CAN_Bus *bus, uint32_t speed_bps) {
    memset(bus, 0, sizeof *bus);
    bus->speed_bps = speed_bps;
}

void bus_add_node(CAN_Bus *bus, ECU *ecu) {
    if (bus->num_nodes < MAX_NODES) {
        bus->nodes[bus->num_nodes++] = ecu;
    }
}

void bus_add_bg_msg(CAN_Bus *bus, uint16_t id, uint64_t period_us, uint64_t start_us) {
    if (bus->num_bg_msgs < MAX_BG_MSGS) {
        bg_msg_t *m = &bus->bg_msgs[bus->num_bg_msgs++];
        m->id = id;
        m->period_us = period_us;
        m->start_us = start_us;
    }
}

void bus_set_jitter(CAN_Bus *bus, int64_t max_us) {
    bus->jitter_max_us = max_us;
}

/**
 * Look up the registered background msg whose ID matches preceded_id
 * and return the nominal absolute time at which its most recent transmission completed relative to
 * the victim's nominal Tx time ref_us.
 */
static uint64_t preceded_msg_done(const CAN_Bus *bus, uint16_t preceded_id, uint64_t ref_us) {
    for (int i = 0; i < bus->num_bg_msgs; i++) {
        const bg_msg_t *m = &bus->bg_msgs[i];
        if (m->id != preceded_id) continue;

        uint64_t elapsed = (ref_us >= m->start_us) ? ref_us - m->start_us : 0;
        uint64_t n = elapsed / m->period_us;
        uint64_t done = m->start_us + n * m->period_us + FRAME_DURATION_US;

        /* step back if this completion is too late to preced ref_us */
        while (n > 0 && done + IFS_US > ref_us + BIT_TIME_US) {
            n--;
            done = m->start_us + n * m->period_us + FRAME_DURATION_US;
        }
        return done;
    }
    return 0; /* predeced_id not registered */
}

int bus_simulate_attack(CAN_Bus *bus, ECU *victim, ECU *adversary, uint64_t simulation_duration_us, sim_record_t *records, int max_records) {
    int count = 0;
    uint64_t t = 0;

    /* Record initial state */
    push_record(records, &count, max_records, t, victim, adversary);

    /* Phase 1 
     * 
     * Each loop iteration:
     * 1. Adversary dominant bits overwirte victim's recessive bits in the data field -> victim TEC += 8.
     * 2. Victim issue Active Error Flag (6 dominant bits) which violates stuffing at the adversary -> adversary TEC += 8.
     * 3. CAN Controller triggers automatic retransmission for both.
     * This repeats until one node exits Error Active.
    */
    while (victim->state == ECU_STATE_ERROR_ACTIVE && adversary->state == ECU_STATE_ERROR_ACTIVE) {
        ecu_on_tx_error(victim); /* bit error detected at victim */
        ecu_on_tx_error(adversary); /* victim's active error flag */

        t += FRAME_DURATION_US + ERROR_FRAME_US;
        push_record(records, &count, max_records, t, victim, adversary);
    }

    /* Phase 1->2 transition 
     *
     * At this point both nods are Error Passive (TEC = 128)
     * One more collision occurs:
     * - Victim still gets a bit error (TEC = 136) but now can only send
     *   a Passive Error Flag (6 recessive bits), which A ignores.
     * - A completes its frame successfully (TEC = 127 -> becomes Error Active).
     * - V retransmits and succeeds (TEC = 135) -> still Error Passive.
    */
    if (victim->state == ECU_STATE_ERROR_PASSIVE && adversary->state == ECU_STATE_ERROR_PASSIVE) {
        /* Victim bit error; passive err flag ignored by adversary */
        ecu_on_tx_error(victim);
        t += FRAME_DURATION_US;
        push_record(records, &count, max_records, t, victim, adversary);

        /* Adversary completes frame -> returns to Error Active*/
        ecu_on_tx_success(adversary);
        /* Victim retransmits without further interference */
        ecu_on_tx_success(victim);
        t += FRAME_DURATION_US + IFS_US;
        push_record(records, &count, max_records, t, victim, adversary);
    }

    /* Phase 2
     * 
     * The adversary watches for the registered preceded ID msg to complete, it then 
     * queues its attack msg. Both nodes trasmits after the IFS gap.
     * 
     * Per successful attack cycle:
     * 1. Victim bit error -> victim TEC += 8.
     * 2. Victim sends Passive err flag -> ignored by adversary.
     * 3. Adversary transmission completes successfully -> adversary TEC -= 1.
     * 4. Victim retrasmits successfully -> victim TEC -= 1.
     */

     /* Period based estimated attack */
    uint64_t victim_nominal = victim->next_tx_us + victim->period_us;

    while (victim->state != ECU_STATE_BUS_OFF && victim_nominal < simulation_duration_us) {
        /* jitter */
        int64_t j = 0;
        if (bus->jitter_max_us > 0) {
            j = (int64_t)(rand() % (int64_t)(2 * bus->jitter_max_us + 1)) - bus->jitter_max_us;
        }
        /* Preceded ID sync */
        uint64_t victim_actual, attack_us;
        if (adversary->use_preceded_id) {
            uint64_t prec_nominal = preceded_msg_done(bus, adversary->preceded_id, victim_nominal);
            
            if (prec_nominal > 0) {
                uint64_t prec_actual = (uint64_t)((int64_t)prec_nominal + j);
                victim_actual = prec_actual + IFS_US;
                attack_us = prec_actual + IFS_US; 
            } else {
                /* preceded_id not in registry - fallback */
                victim_actual = (uint64_t)((int64_t)victim_nominal + j);
                attack_us = victim_actual;
            }
        } else {
            /* Period based adversary estimate attack */
            victim_actual = (int64_t)((int64_t)victim_nominal + j);
            attack_us = victim_nominal;
        }

        bool synchronized = llabs((int64_t)attack_us - (int64_t)victim_actual) <= (int64_t)BIT_TIME_US;

        t = victim_actual;

        if (synchronized) {
            ecu_on_tx_error(victim);
            push_record(records, &count, max_records, t, victim, adversary);

            if (victim->state == ECU_STATE_BUS_OFF) break;

            t += FRAME_DURATION_US;

            ecu_on_tx_success(adversary);
            
            ecu_on_tx_success(victim);
            push_record(records, &count, max_records, t, victim, adversary);
        } else {
            ecu_on_tx_success(victim);
            push_record(records, &count, max_records, t, victim, adversary);
        }
        victim_nominal += victim->period_us;
    }

    /* Record final state */
    push_record(records, &count, max_records, t, victim, adversary);
    return count;
}

void bus_print_summary(const CAN_Bus *bus, FILE *f) {
    fprintf(f, "=== CAN Bus (speed: %u bps, nodes: %d) ===\n",
            bus->speed_bps, bus->num_nodes);
    for (int i = 0; i < bus->num_nodes; i++) {
        ecu_print(bus->nodes[i], f);
    }
    fprintf(f, "======================\n");
}