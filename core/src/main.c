#include <stdio.h>
#include "agc.h"

int main(void) {
    agc_state state;
    agc_init(&state);
    printf("AGC initialized. Running 3 ticks...\n");
    for (int i = 0; i < 3; ++i) {
        agc_tick(&state);
    }
    printf("Done.\n");
    return 0;
}
