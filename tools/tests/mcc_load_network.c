/* MCC conversion progress: the round's limits and each machine's allowance. */
#include "mcc_load_network.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

/* hold every 33 ms from `now` while it lasts or for `span`; the time held */
static uint32_t hold(struct mcc_load_allowance *allowance, uint32_t *now, uint32_t span)
{
    uint32_t start = *now;
    while ((uint32_t)(*now - start) < span && mcc_load_allowance_hold(allowance, *now))
        *now += 33;
    return *now - start;
}

int main(int argc, char **argv)
{
    struct mcc_load_allowance allowance;
    uint32_t now = 0xFFFF0000u, held, total = 0, elapsed = 0;
    int round;
    if (argc != 2) return 2;
    memset(&allowance, 0, sizeof(allowance));
    if (!strcmp(argv[1], "pulse_only")) {
        /* a machine that holds every start as long as it can, rounds of five minutes */
        for (round = 0; round < 20; round++) {
            held = hold(&allowance, &now, MCC_LOAD_NETWORK_LIMIT);
            total += held;
            now += 300000u;
            elapsed += held + 300000u;
        }
        CHECK(total < elapsed / 3);
        return 0;
    }
    if (!strcmp(argv[1], "converting")) {
        /* two minutes of conversion each round, rounds of six minutes: never cut short */
        for (round = 0; round < 20; round++) {
            CHECK(hold(&allowance, &now, 120000u) >= 120000u);
            now += 360000u;
        }
        return 0;
    }
    if (!strcmp(argv[1], "limit")) {
        CHECK(hold(&allowance, &now, 10u * MCC_LOAD_NETWORK_LIMIT) <= MCC_LOAD_NETWORK_LIMIT + 33);
        CHECK(!mcc_load_allowance_hold(&allowance, now));
        /* a long wait earns no more than the limit */
        now += 100u * MCC_LOAD_NETWORK_LIMIT;
        CHECK(hold(&allowance, &now, 10u * MCC_LOAD_NETWORK_LIMIT) <= MCC_LOAD_NETWORK_LIMIT + 33);
        /* a new machine in the slot has the whole allowance */
        mcc_load_allowance_forget(&allowance);
        CHECK(hold(&allowance, &now, MCC_LOAD_NETWORK_LIMIT) >= MCC_LOAD_NETWORK_LIMIT - 33);
        return 0;
    }
    return 2;
}
