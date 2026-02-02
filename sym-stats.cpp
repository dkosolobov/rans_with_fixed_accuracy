// 
// The following code is retrieved from ryg's rANS implementation (with comments revmoved)
// https://github.com/rygorous/ryg_rans
//

#include <stdint.h>
#include "sym-stats.h"

void SymbolStats::count_freqs(uint8_t const* in, size_t nbytes) {
    for (int i = 0; i < 256; i++)
        freqs[i] = 0;

    for (size_t i = 0; i < nbytes; i++)
        freqs[in[i]]++;
}

void SymbolStats::calc_cum_freqs() {
    cum_freqs[0] = 0;
    for (int i = 0; i < 256; i++)
        cum_freqs[i + 1] = cum_freqs[i] + freqs[i];
}

void SymbolStats::normalize_freqs(uint32_t target_total) {
    calc_cum_freqs();
    uint32_t cur_total = cum_freqs[256];

    for (int i = 1; i <= 256; i++)
        cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i]) / cur_total;

    for (int i = 0; i < 256; i++) {
        if (freqs[i] && cum_freqs[i + 1] == cum_freqs[i]) {
            uint32_t best_freq = ~0u;
            int best_steal = -1;
            for (int j = 0; j < 256; j++) {
                uint32_t freq = cum_freqs[j + 1] - cum_freqs[j];
                if (freq > 1 && freq < best_freq) {
                    best_freq = freq;
                    best_steal = j;
                }
            }
            if (best_steal < i) {
                for (int j = best_steal + 1; j <= i; j++)
                    cum_freqs[j]--;
            } else {
                for (int j = i + 1; j <= best_steal; j++)
                    cum_freqs[j]++;
            }
        }
    }

    for (int i = 0; i < 256; i++) {
        freqs[i] = cum_freqs[i + 1] - cum_freqs[i];
    }
}
