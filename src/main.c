#include "ecu.h"

int main() {
    FILE *f = fopen("ecus", "a");
    ECU victim, adversary;

    ecu_init(&victim, 0, "Victim Node", 0x11, 1, 10000ULL, 0);
    ecu_init(&adversary, 1, "Adversary Node", 0x7, 1, 10000ULL, 0);
    ecu_make_adversary(&adversary, 0x11);

    ecu_print(&victim, f);
    ecu_print(&adversary, f);

    fclose(f);
}