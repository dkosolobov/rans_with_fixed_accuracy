// 
// The following code is retrieved from ryg's rANS implementation (with comments revmoved)
// https://github.com/rygorous/ryg_rans
//

#include <stdint.h>
#include <vector>

#include "rans-fast.h"
#include "sym-stats.h"

#if defined(_MSC_VER)
#include <intrin.h>
static inline uint64_t Rans64MulHi(uint64_t a, uint64_t b) {
    return __umulh(a, b);
}
#elif defined(__GNUC__)
static inline uint64_t Rans64MulHi(uint64_t a, uint64_t b) {
    return (uint64_t)(((unsigned __int128)a * b) >> 64);
}
#endif

static constexpr uint64_t RANS64_L = 1ull << 31;
static constexpr uint32_t prob_bits = 14;
typedef uint64_t Rans64State;


//
// Rncoding
//

static inline void Rans64EncPutSymbol(Rans64State* r, uint32_t** pptr, RansFast64EncSymbol const* sym, uint32_t scale_bits) {
    uint64_t x = *r;
    uint64_t x_max = ((RANS64_L >> scale_bits) << 32) * sym->freq;
    if (x >= x_max) {
        *pptr -= 1;
        **pptr = (uint32_t)x;
        x >>= 32;
    }

    uint64_t q = Rans64MulHi(x, sym->rcp_freq) >> sym->rcp_shift;
    *r = x + sym->bias + q * sym->cmpl_freq;    //*/
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

int encode_rANS_fast(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& buf, const std::vector<RansFast64EncSymbol>& esyms) {
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
// Initiializtion
//

static inline void Rans64EncSymbolInit(RansFast64EncSymbol* s, uint32_t start, uint32_t freq, uint32_t scale_bits) {
    s->freq = freq;
    s->cmpl_freq = ((1 << scale_bits) - freq);
    if (freq < 2) {
        s->rcp_freq = ~0ull;
        s->rcp_shift = 0;
        s->bias = start + (1 << scale_bits) - 1;
    }
    else {
        uint32_t shift = 0;
        uint64_t x0, x1, t0, t1;
        while (freq > (1u << shift))
            shift++;

        x0 = freq - 1;
        x1 = 1ull << (shift + 31);

        t1 = x1 / freq;
        x0 += (x1 % freq) << 32;
        t0 = x0 / freq;

        s->rcp_freq = t0 + (t1 << 32);
        s->rcp_shift = shift - 1;

        s->bias = start;
    }
}

RansFast64SequenceInfo init_rANS_fast(const std::vector<uint8_t>& sequence) {
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

    std::vector<RansFast64EncSymbol> esyms(256);
    std::vector<Rans64DecSymbol> dsyms(256);
    for (int i = 0; i < 256; i++) {
        Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], prob_bits);
        Rans64DecSymbolInit(&dsyms[i], stats.cum_freqs[i], stats.freqs[i]);
    }
    return { .esyms = esyms, .dsyms = dsyms, .cum2sym = cum2sym };
}


//
// Decoding (the same as in the usual rANS)
//

#include "rans.h"

void decode_rANS_fast(const std::vector<Rans64DecSymbol>& dsyms, const std::vector<uint8_t>& cum2sym,
    const uint8_t* rans_begin, uint8_t* dec_bytes, size_t original_size
) {
    decode_rANS(dsyms, cum2sym, rans_begin, dec_bytes, original_size);
}
