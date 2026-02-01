#include "agc.h"
#include <string.h>

void agc_init(agc_state *state) {
    memset(state, 0, sizeof(agc_state));
}

void agc_tick(agc_state *state) {
    // TODO: fetch, decode, execute
}
