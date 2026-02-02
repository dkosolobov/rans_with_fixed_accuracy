#include <vector>
#include <bit>
#include <stdint.h>

#include "sym-stats.h"
#include "rans-fixed-accuracy.h"

static constexpr int STATE_BITS = 14;	//16;
static constexpr int ACCURACY_BITS = 3;	// hardcoded and after change first calls to div_high should be removed/added
static constexpr int ALL_BITS = STATE_BITS + ACCURACY_BITS;
static constexpr int ACCURACY_MASK = (1 << (ACCURACY_BITS + 1)) - 1;
static constexpr int STATE_MASK = (1 << STATE_BITS) - 1;

static_assert(STATE_BITS * 3 + ACCURACY_BITS + 8 <= 64, "Ensure three iterations of encode_symbol without flush_bits");
static_assert(ALL_BITS < 32 - 7, "");

alignas(128) static const uint32_t bit_masks[] = { 0, 0x1, 0x3, 0x7, 0xF, 0x1F,  0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF,
	0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, 0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF,
	0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF };


//
// Encoding
//

static inline void emit_bits(uint64_t& output_word, uint8_t& ptr, uint32_t bits, int count) {
	output_word |= (uint64_t)(bits & bit_masks[count]) << ptr;
	ptr += count;
}

static inline void flush_bits(uint64_t& output_word, uint8_t& ptr, uint8_t*& buffer) {
	int bytes_num = ptr >> 3;
	memcpy(buffer, &output_word, sizeof(uint64_t));
	ptr &= 7;
	output_word >>= bytes_num << 3;
	buffer += bytes_num;
}

static inline void div_high(uint32_t freq, uint32_t& x, uint32_t& rem, int rem_bit) {
	uint32_t x_sub = x - (freq << rem_bit);
	if ((int32_t)x_sub >= 0)
		x = x_sub;
	rem |= x_sub & (1 << (rem_bit + ALL_BITS));
}

static inline uint32_t encode_symbol(const EncSymInfo& sym_inf, uint32_t x, uint64_t& output_word, uint8_t& ptr) {
	uint32_t cumm_freq = sym_inf.cumm_freq;
	uint32_t freq = sym_inf.freq;
	uint32_t delta = sym_inf.delta;
	int shift = (x + delta) >> (ALL_BITS + 1);			// Collet's trick
	emit_bits(output_word, ptr, x, shift);
	x >>= shift;
	x -= freq << ACCURACY_BITS;

	uint32_t rem = 0;
	static_assert(ACCURACY_BITS == 3, "To increase/decrease accuracy, more/less div_high should be invoked");
	div_high(freq, x, rem, 2);
	div_high(freq, x, rem, 1);
	div_high(freq, x, rem, 0);
	rem = (rem ^ (ACCURACY_MASK << ALL_BITS)) >> ACCURACY_BITS;
	return x + cumm_freq + rem;
}

int encode_rANS_with_accuracy_3(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& output, const std::vector<EncSymInfo>& sym_table) {
	uint32_t x = 1 << ALL_BITS;
	uint64_t output_word = 0;
	uint8_t ptr = 0;		// bit pointer inside the output_word
	const uint8_t* reverse_seq = sequence.data() + sequence.size();
	const uint8_t* sequence_data = sequence.data();
	uint8_t* buffer = output.data();

	while (reverse_seq >= sequence_data + 3) {
		x = encode_symbol(sym_table[*--reverse_seq], x, output_word, ptr);
		x = encode_symbol(sym_table[*--reverse_seq], x, output_word, ptr);
		x = encode_symbol(sym_table[*--reverse_seq], x, output_word, ptr);
		flush_bits(output_word, ptr, buffer);
	}
	while (reverse_seq > sequence_data) {
		x = encode_symbol(sym_table[*--reverse_seq], x, output_word, ptr);
		flush_bits(output_word, ptr, buffer);
	}

	uint32_t z = (x << ptr) | (uint32_t)output_word;  // after flush_bits at most 7 bits in output_word are used
	memcpy(buffer, &z, sizeof(uint32_t));
	buffer += 4;
	return buffer - output.data();
}

//
// Initialization
//

SequenceInfo init_rANS_with_accuracy_3(const std::vector<uint8_t>& sequence) {
	SymbolStats stats;
	stats.count_freqs(sequence.data(), sequence.size());
	stats.normalize_freqs(1 << STATE_BITS);

	std::vector<uint8_t> cum2sym(1 << STATE_BITS);
	for (int s = 0; s < 256; s++)
		for (uint32_t i = stats.cum_freqs[s]; i < stats.cum_freqs[s + 1]; i++)
			cum2sym[i] = s;

	std::vector<EncSymInfo> esyms(256);
	std::vector<DecSymInfo> dsyms(256);

	for (int j = 0; j < 256; j++) {
		dsyms[j].freq = esyms[j].freq = stats.freqs[j];
		dsyms[j].cumm_freq = esyms[j].cumm_freq = stats.cum_freqs[j];
		uint32_t shift = STATE_BITS - std::bit_width(esyms[j].freq) + 1;
		esyms[j].delta = (shift << (ALL_BITS + 1)) - (esyms[j].freq << (shift + ACCURACY_BITS));
	}
	return { .esyms = esyms, .dsyms = dsyms, .cum2sym = cum2sym };
}


//
// Decoding
//

static inline uint32_t read_bits(uint64_t& word, uint8_t& ptr, const uint8_t* RESTRICT & buffer_end, uint8_t count) {
	ptr -= count;
	return (word >> ptr) & bit_masks[count];
}

static inline void read_buffer(uint64_t& word, uint8_t& ptr, const uint8_t* RESTRICT & buffer_end) {
	if (STATE_BITS > ptr) {
		buffer_end -= 4;
		uint32_t buf;
		memcpy(&buf, buffer_end, 4);
		word = (word << 32) | buf;
		ptr += 32;
	}
}

void decode_rANS(const DecSymInfo * dsyms_data, const uint8_t * cum2sym_data,
	const uint8_t* buffer_end, uint8_t* out_buf, uint8_t* out_end
) {
	buffer_end -= 4;
	uint32_t x;
	memcpy(&x, buffer_end, 4);
	uint8_t bit = std::bit_width(x) - 1;
	uint8_t ptr = bit - ALL_BITS;
	uint64_t input_word = x & bit_masks[ptr];
	x = x >> ptr;
	read_buffer(input_word, ptr, buffer_end);

	while (out_buf != out_end) {
		uint32_t y = x & STATE_MASK;

		int sym = cum2sym_data[y];
		*out_buf = sym;
		out_buf++;

		uint32_t rem = y - dsyms_data[sym].cumm_freq;
		uint32_t z = dsyms_data[sym].freq * (x >> STATE_BITS) + rem;
		int shift = ALL_BITS - (std::bit_width(z) - 1);
		x = (z << shift) + read_bits(input_word, ptr, buffer_end, shift);
		read_buffer(input_word, ptr, buffer_end);
	}
}
