// 
// The following code is retrieved from ryg's rANS implementation (with comments revmoved)
// https://github.com/rygorous/ryg_rans
//

#pragma once

#include <vector>
#include <stdint.h>


typedef struct {
    uint32_t freq;
    uint32_t cumm_freq;
} Rans64EncSymbol;

typedef struct {
    uint32_t start;
    uint32_t freq;
} Rans64DecSymbol;

typedef struct {
    std::vector<Rans64EncSymbol> esyms;
    std::vector<Rans64DecSymbol> dsyms;
    std::vector<uint8_t> cum2sym;
} Rans64SequenceInfo;

Rans64SequenceInfo init_rANS(const std::vector<uint8_t>& sequence);
int encode_rANS(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& buf, const std::vector<Rans64EncSymbol>& esyms);
void decode_rANS(const std::vector<Rans64DecSymbol>& dsyms, const std::vector<uint8_t>& cum2sym,
    const uint8_t* rans_begin, uint8_t* dec_bytes, size_t original_size);
