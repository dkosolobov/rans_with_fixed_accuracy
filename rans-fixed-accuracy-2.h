#pragma once

#include <vector>
#include <stdint.h>

#include "sym-stats.h"

struct EncSymInfo_2 {
	uint32_t delta;
	uint16_t cumm_freq;
	uint16_t freq;
};

struct DecSymInfo_2 {
	uint32_t cumm_freq;
	uint32_t freq;
};

typedef struct {
	std::vector<EncSymInfo_2> esyms;
	std::vector<DecSymInfo_2> dsyms;
	std::vector<uint8_t> cum2sym;
} SequenceInfo_2;


SequenceInfo_2 init_rANS_with_accuracy_2(const std::vector<uint8_t>& sequence);
int encode_rANS_with_accuracy_2(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& buf, const std::vector<EncSymInfo_2>& esyms);
void decode_rANS_2(const DecSymInfo_2* dsyms_data, const uint8_t* cum2sym_data, const uint8_t* buffer_end, uint8_t* out_buf, uint8_t* out_end);

