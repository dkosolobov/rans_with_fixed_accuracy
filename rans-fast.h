#pragma once

#include <vector>
// 
// The following code is retrieved from ryg's rANS implementation (with comments revmoved)
// https://github.com/rygorous/ryg_rans
//

#pragma once

#include <stdint.h>
#include "rans.h"

typedef struct {
    uint64_t rcp_freq;  // Fixed-point reciprocal frequency
    uint32_t freq;
    uint32_t bias;
    uint32_t cmpl_freq; // Complement of frequency: (1 << scale_bits) - freq
    uint32_t rcp_shift;
} RansFast64EncSymbol;

typedef struct {
    std::vector<RansFast64EncSymbol> esyms;
    std::vector<Rans64DecSymbol> dsyms;
    std::vector<uint8_t> cum2sym;
} RansFast64SequenceInfo;

RansFast64SequenceInfo init_rANS_fast(const std::vector<uint8_t>& sequence);
int encode_rANS_fast(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& buf, const std::vector<RansFast64EncSymbol>& esyms);
void decode_rANS_fast(const std::vector<Rans64DecSymbol>& dsyms, const std::vector<uint8_t>& cum2sym,
    const uint8_t* rans_begin, uint8_t* dec_bytes, size_t original_size);
