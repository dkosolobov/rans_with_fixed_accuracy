#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <bit>
#include <stdint.h>
#include <intrin.h>
#include <immintrin.h>

#include "rans.h"
#include "rans-fast.h"
#include "rans-fixed-accuracy.h"


static void test_sequence(const std::vector<uint8_t> & sequence) {
	using namespace std::chrono;

	std::vector<uint8_t> encoded_sequence(sequence.size() * 2 + 10);
	std::vector<uint8_t> decode_buffer(sequence.size() + 10);
	
	auto rANSinfo = init_rANS(sequence);

	auto t1_rans = high_resolution_clock::now();
	int res_rans = encode_rANS(sequence, encoded_sequence, rANSinfo.esyms);
	auto t2_rans = high_resolution_clock::now();
	auto ms_rans = duration_cast<nanoseconds>(t2_rans - t1_rans);
	t1_rans = high_resolution_clock::now();
	decode_rANS(rANSinfo.dsyms, rANSinfo.cum2sym, &(*(encoded_sequence.end() - res_rans)), decode_buffer.data(), sequence.size());
	t2_rans = high_resolution_clock::now();
	auto ms_rans2 = duration_cast<nanoseconds>(t2_rans - t1_rans);

	auto rANSfast_info = init_rANS_fast(sequence);

	auto t1_ransf = high_resolution_clock::now();
	int res_ransf = encode_rANS_fast(sequence, encoded_sequence, rANSfast_info.esyms);
	auto t2_ransf = high_resolution_clock::now();
	auto ms_ransf = duration_cast<nanoseconds>(t2_ransf - t1_ransf);
	t1_ransf = high_resolution_clock::now();
	decode_rANS_fast(rANSinfo.dsyms, rANSinfo.cum2sym, &(*(encoded_sequence.end() - res_rans)), decode_buffer.data(), sequence.size());
	t2_ransf = high_resolution_clock::now();
	auto ms_ransf2 = duration_cast<nanoseconds>(t2_ransf - t1_ransf);

	auto info = init_rANS_with_accuracy_3(sequence);

	auto t1_ours = high_resolution_clock::now();
	int res_ours = encode_rANS_with_accuracy_3(sequence, encoded_sequence, info.esyms);
	auto t2_ours = high_resolution_clock::now();
	auto ms_ours = duration_cast<nanoseconds>(t2_ours - t1_ours);
	t1_ours = high_resolution_clock::now();
	decode_rANS(info.dsyms.data(), info.cum2sym.data(), encoded_sequence.data() + res_ours, decode_buffer.data(), decode_buffer.data() + sequence.size());
	t2_ours = high_resolution_clock::now();
	auto ms_ours2 = duration_cast<nanoseconds>(t2_ours - t1_ours);

	if (!std::equal(sequence.begin(), sequence.end(), decode_buffer.begin()))
		std::cout << "ERROR! sequence decompressed incorrectly by rANS with accuracy 3" << std::endl;


	std::cout << "Comp/decomp time rANS with acc 3: " << ms_ours.count() << "/" << ms_ours2.count() << " ns, compressed len: " << res_ours << std::endl;
	std::cout << "Comp/decomp time rANS:            " << ms_rans.count() << "/" << ms_rans2.count() << " ns, compressed len: " << res_rans << std::endl;
	std::cout << "Comp/decomp time rANS fast:       " << ms_ransf.count() << "/" << ms_ransf2.count() << " ns, compressed len: " << res_ransf << std::endl << std::endl;
}

int main() {
	std::vector<uint8_t> sequence(1 << 16);
	
	std::default_random_engine gen;

	std::geometric_distribution<int> dist0(0.7);
	for (int i = 0; i < sequence.size(); i++)
		sequence[i] = dist0(gen) % 256;
	test_sequence(sequence);

	std::geometric_distribution<int> dist1(0.3);
	for (int i = 0; i < sequence.size(); i++)
		sequence[i] = dist1(gen) % 256;
	test_sequence(sequence);

	std::uniform_int_distribution<int> dist2(0, 255);
	for (int i = 0; i < sequence.size(); i++)
		sequence[i] = dist2(gen) % 256;
	test_sequence(sequence);
}
