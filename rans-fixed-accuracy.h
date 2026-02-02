#pragma once

#include <vector>
#include <stdint.h>

#include "sym-stats.h"

struct EncSymInfo {
	uint32_t delta;
	uint16_t cumm_freq;
	uint16_t freq;
};

struct DecSymInfo {
	uint32_t cumm_freq;
	uint32_t freq;
};

typedef struct {
	std::vector<EncSymInfo> esyms;
	std::vector<DecSymInfo> dsyms;
	std::vector<uint8_t> cum2sym;
} SequenceInfo;


SequenceInfo init_rANS_with_accuracy_3(const std::vector<uint8_t>& sequence);
int encode_rANS_with_accuracy_3(const std::vector<uint8_t>& sequence, std::vector<uint8_t>& buf, const std::vector<EncSymInfo>& esyms);
void decode_rANS(const DecSymInfo* dsyms_data, const uint8_t* cum2sym_data, const uint8_t* buffer_end, uint8_t* out_buf, uint8_t* out_end);
