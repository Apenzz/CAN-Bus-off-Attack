#include "can_bus.h"

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

void bus_simulate_attack(CAN_Bus *bus, ECU *victim, ECU *adversary, uint64_t simulation_duration_us, sim_record_t *records, int max_records) {
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
     * Per successful attack cycle:
     * 1. Victim bit error -> victim TEC += 8.
     * 2. Victim sends Passive err flag -> ignored by adversary.
     * 3. Adversary transmission completes successfully -> adversary TEC -= 1.
     * 4. Victim retrasmits successfully -> victim TEC -= 1.
     */
    uint64_t next_attack_us = victim->next_tx_us + victim->period_us;

    while (victim->state != ECU_STATE_BUS_OFF && next_attack_us < simulation_duration_us) {
        t = next_attack_us;
        /* step 1 */
        ecu_on_tx_error(victim);
        push_record(records, &count, max_records, t, victim, adversary);

        if (victim->state == ECU_STATE_BUS_OFF) break;

        t += FRAME_DURATION_US;

        /* Step 2+3 */
        ecu_on_tx_success(adversary);
        /* Step 4 */
        ecu_on_tx_success(victim);
        push_record(records, &count, max_records, t, victim, adversary);

        next_attack_us += victim->period_us;
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