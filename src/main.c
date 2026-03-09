#include "ecu.h"
#include "can_bus.h"
#include <stdlib.h>

/* Simulation constants */
#define SIM_DURATION_US 200000ULL /* 200 ms hard cap */
#define MSG_PERIOD_US 10000ULL /* 10 ms message period */
#define MAX_RECORDS 100000

#define BG_09_OFFSET_US (FRAME_DURATION_US + IFS_US)
#define VICTIM_OFFSET_US (2 * (FRAME_DURATION_US + IFS_US))

static void write_csv(const char *path, const sim_record_t *records, int n) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror(path);
        return;
    }

    fprintf(f, "time_ms,victim_tec,adversary_tec,victim_state,adversary_state\n");
    for (int i = 0; i < n; i++) {
        fprintf(f, "%.4f,%u,%u,%d,%d\n",
                records[i].time_us / 1000.0,
                records[i].victim_tec,
                records[i].adv_tec,
                (int)records[i].victim_state,
                (int)records[i].adv_state);
    }
    fclose(f);
    printf("Results written to: %s (%d records)\n", path, n);
}

static void run_simulation(const char *label, const char *csv_path, bool use_prec) {
    printf("\n=== %s ===\n", label);

    CAN_Bus bus;
    ECU node_a_07, node_a_09, victim, adversary;

    bus_init(&bus, 500000);
    
    /* Bg traffic: node A transmits 0x07 then 0x09 each period */
    /* Te adversary uses 0x09 as the preceded ID trigger */
    bus_add_bg_msg(&bus, 0x07, MSG_PERIOD_US, 0);
    bus_add_bg_msg(&bus, 0x09, MSG_PERIOD_US, BG_09_OFFSET_US);

    /* Node A registered for summary display */
    ecu_init(&node_a_07, 0, "NodeA-0x07", 0x07, 1, MSG_PERIOD_US, 0);
    ecu_init(&node_a_09, 1, "NodeA-0x09", 0x09, 1, MSG_PERIOD_US, BG_09_OFFSET_US);
    bus_add_node(&bus, &node_a_07);
    bus_add_node(&bus, &node_a_09);

    /* Node Victim sends 0x11 every 10 ms, starting after 0x09 */
    ecu_init(&victim, 2, "Victim", 0x11, 1, MSG_PERIOD_US, VICTIM_OFFSET_US);
    bus_add_node(&bus, &victim);

    /* node Adversary: attacks id=0x11 with DLC=0 */
    ecu_init(&adversary, 3, "Adversary", 0x11, 0, MSG_PERIOD_US, VICTIM_OFFSET_US);
    ecu_make_adversary(&adversary, 0x11);
    if (use_prec) {
        ecu_set_preceded_id(&adversary, 0x09);
    }

    sim_record_t *records = malloc(MAX_RECORDS * sizeof(*records));
    if (!records) {
        fprintf(stderr, "malloc failed\n");
        return;
    }

    int nrec = bus_simulate_attack(&bus, &victim, &adversary, SIM_DURATION_US, records, MAX_RECORDS);
    /* Key events */
    double busoff_ms = -1.0;
    double ep_ms = -1.0;
    for (int i = 0; i < nrec; i++) {
        if (ep_ms < 0 && records[i].victim_state == ECU_STATE_ERROR_PASSIVE) {
            ep_ms = records[i].time_us / 1000.0;
        }
        if (busoff_ms < 0 && records[i].victim_state == ECU_STATE_BUS_OFF) {
            busoff_ms = records[i].time_us / 1000.0;
        }
    }

    printf("Error Passive at: %.3f ms\n", ep_ms >= 0 ? ep_ms : -1.0);
    if (busoff_ms >= 0) {
        printf("Bus-Off at: %.3f ms\n", busoff_ms);
    } else {
        printf("Bus-Off NOT reached within %.0f ms\n", SIM_DURATION_US / 1000.0);
    }
    printf("Final victim TEC: %u (state: %s)\n", victim.tec, ecu_state_name(victim.state));
    printf("Final adversary TEC: %u (state: %s)\n", adversary.tec, ecu_state_name(adversary.state));

    if (csv_path) write_csv(csv_path, records, nrec);
    free(records);
}

int main() {

    /* Run simulation */

    /* Run 1: no preceded ID, period based sync attack */
    run_simulation("Period based attack", "results.csv", false);

    /* Run 2: preceded ID based attack */
    run_simulation("Preceded ID based attack", "preceded_id_attack.csv", true);
}