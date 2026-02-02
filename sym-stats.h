// 
// The following code is retrieved from ryg's rANS implementation (with comments revmoved)
// https://github.com/rygorous/ryg_rans
//

#pragma once
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict__
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#include <intrin.h>
#define RESTRICT __restrict
#endif

struct SymbolStats {
    uint32_t freqs[256];
    uint32_t cum_freqs[257];

    void count_freqs(uint8_t const* in, size_t nbytes);
    void calc_cum_freqs();
    void normalize_freqs(uint32_t target_total);
};
