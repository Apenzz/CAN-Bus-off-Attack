#include "ecu.h"
#include "can_bus.h"
#include <stdlib.h>

/* Simulation constants */
#define SIM_DURATION_US 200000ULL /* 200 ms hard cap */
#define MSG_PERIOD_US 10000ULL /* 10 ms message period */
#define MAX_RECORDS 100000

int main() {
    CAN_Bus bus;
    ECU node_a, victim, adversary;

    /* bus initialization */
    bus_init(&bus, 500000);

    ecu_init(&node_a, 0, "Node A", 0x07, 1, MSG_PERIOD_US, 0);
    bus_add_node(&bus, &node_a);
    ecu_init(&victim, 1, "Victim Node", 0x11, 1, MSG_PERIOD_US, 0);
    bus_add_node(&bus, &victim);
    ecu_init(&adversary, 2, "Adversary Node", 0x11, 0, MSG_PERIOD_US, 0);
    ecu_make_adversary(&adversary, 0x11);
    bus_add_node(&bus, &adversary);

    printf("\nBus-Off Attack Simulation\n");
    bus_print_summary(&bus, stdout);

    /* allocate record buffer */
    sim_record_t *records = malloc(MAX_RECORDS * sizeof *records);
    if (!records) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    /* Run simulation */
    printf("Running simulation (duration limit: %.0f ms)...\n\n",
            SIM_DURATION_US / 1000.0);

    int nrec = bus_simulate_attack(&bus, &victim, &adversary, SIM_DURATION_US, records, MAX_RECORDS);

    printf("Final ECU States\n");
    ecu_print(&victim, stdout);
    ecu_print(&adversary, stdout);

    /* Find bus-off event */
    double busoff_ms = -1.0;
    for (int i = 0; i < nrec; i++) {
        if (records[i].victim_state == ECU_STATE_BUS_OFF) {
            busoff_ms = records[i].time_us / 1000.0;
            break;
        }
    }
    if (busoff_ms >= 0.0) {
        printf("Victim entered Bus-off at t = %.3f ms\n", busoff_ms);
    } else {
        printf("Victim did NOT enter bus-off within the simulation window.\n");
    }

    /* Phase transition */
    for (int i = 0; i < nrec; i++) {
        if (records[i].victim_state == ECU_STATE_ERROR_PASSIVE && records[i - 1 > 0 ? i - 1 : 0].victim_state == ECU_STATE_ERROR_ACTIVE) {
            printf("Victim entered error passive at t = %.3f ms (TEC=%u)\n",
                    records[i].time_us / 1000.0,
                    records[i].victim_tec);
            break;
        }
    }
    printf("Total data points recorded: %d\n\n", nrec);
}