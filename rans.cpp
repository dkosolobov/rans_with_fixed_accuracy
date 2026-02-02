// 
// The following code is retrieved from ryg's rANS implementation (with comments revmoved)
// https://github.com/rygorous/ryg_rans
//

#include <stdint.h>
#include <vector>

#include "rans.h"
#include "sym-stats.h"


static constexpr uint64_t RANS64_L = 1ull << 31;
static constexpr uint32_t prob_bits = 14;
typedef uint64_t Rans64State;


//
// Encoding
//

static inline void Rans64EncPutSymbol(Rans64State* r, uint32_t** pptr, Rans64EncSymbol const* sym, uint32_t scale_bits) {
    uint64_t x = *r;
    uint64_t x_max = ((RANS64_L >> scale_bits) << 32) * sym->freq; // this turns into a shift.
    if (x >= x_max) {
        *pptr -= 1;
        **pptr = (uint32_t)x;
        x >>= 32;
    }
    *r = ((x / sym->freq) << scale_bits) + (x % sym->freq) + sym->cumm_freq;
}

static inline void Rans64EncFlush(Rans64State* r, uint32_t** pptr) {
    uint64_t x = *r;

    *pptr -= 2;
    (*pptr)[0] = (uint32_t)(x >> 0);
    (*pptr)[1] = (uint32_t)(x >> 32);
}

static inline void Rans64DecSymbolInit(Rans64DecSymbol* s, uint32_t start, uint32_t freq) {
    s->start = start;
    s->freq = freq;
}

int encode_rANS(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& buf, const std::vector<Rans64EncSymbol>& esyms) {
    size_t in_size = sequence.size();
    const uint8_t* in_bytes = sequence.data();

    Rans64State rans = RANS64_L;

    uint32_t* out_end = (uint32_t*)(buf.data() + buf.size());
    uint32_t* ptr = out_end;
    for (size_t i = in_size; i > 0; i--) {
        int s = in_bytes[i - 1];
        Rans64EncPutSymbol(&rans, &ptr, &esyms[s], prob_bits);
    }
    Rans64EncFlush(&rans, &ptr);
    uint32_t* rans_begin = ptr;

    return (int)((uint8_t*)out_end - (uint8_t*)rans_begin);
}


//
// Initialization
//

static inline void Rans64EncSymbolInit(Rans64EncSymbol* s, uint32_t start, uint32_t freq, uint32_t scale_bits) {
    s->freq = freq;
    s->cumm_freq = start;
}

Rans64SequenceInfo init_rANS(const std::vector<uint8_t>& sequence) {
    size_t in_size = sequence.size();
    const uint8_t* in_bytes = sequence.data();

    static const uint32_t prob_scale = 1 << prob_bits;

    SymbolStats stats;
    stats.count_freqs(in_bytes, in_size);
    stats.normalize_freqs(prob_scale);

    std::vector<uint8_t> cum2sym(prob_scale);
    for (int s = 0; s < 256; s++)
        for (uint32_t i = stats.cum_freqs[s]; i < stats.cum_freqs[s + 1]; i++)
            cum2sym[i] = s;

    std::vector<Rans64EncSymbol> esyms(256);
    std::vector<Rans64DecSymbol> dsyms(256);
    for (int i = 0; i < 256; i++) {
        Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], prob_bits);
        Rans64DecSymbolInit(&dsyms[i], stats.cum_freqs[i], stats.freqs[i]);
    }
    return { .esyms = esyms, .dsyms = dsyms, .cum2sym = cum2sym };
}


//
// Decoding
//

static inline void Rans64DecInit(Rans64State* r, uint32_t** pptr) {
    uint64_t x;

    x = (uint64_t)((*pptr)[0]) << 0;
    x |= (uint64_t)((*pptr)[1]) << 32;
    *pptr += 2;
    *r = x;
}

static inline uint32_t Rans64DecGet(Rans64State* r, uint32_t scale_bits) {
    return *r & ((1u << scale_bits) - 1);
}

void decode_rANS(const std::vector<Rans64DecSymbol> & dsyms, const std::vector<uint8_t> & cum2sym, 
    const uint8_t * rans_begin, uint8_t *dec_bytes, size_t original_size
) {
    Rans64State rans;
    uint32_t* ptr = (uint32_t *)rans_begin;
    Rans64DecInit(&rans, &ptr);

    for (size_t i = 0; i < original_size; i++) {
        uint32_t s = cum2sym[Rans64DecGet(&rans, prob_bits)];
        dec_bytes[i] = (uint8_t)s;

        uint64_t mask = (1ull << prob_bits) - 1;

        uint64_t x = rans;
        x = dsyms[s].freq * (x >> prob_bits) + (x & mask) - dsyms[s].start;

        if (x < RANS64_L) {
            x = (x << 32) | *ptr;
            ptr += 1;
        }

        rans = x;
    }
}