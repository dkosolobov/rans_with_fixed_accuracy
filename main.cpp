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
#include "rans-fixed-accuracy-2.h"
#include "enwiki16kb.h"


static void test_sequence(const std::vector<uint8_t> & sequence) {
	using namespace std::chrono;

	constexpr int iters = 5;

	std::vector<uint8_t> encoded_sequence(sequence.size() * 2 + 10);
	std::vector<uint8_t> decode_buffer(sequence.size() + 10);
	
	long long ms_rans = 0, ms_rans2 = 0, res_rans = 0;
	for (int i = 0; i < iters; i++) {
		auto t1_rans = high_resolution_clock::now();
		auto rANSinfo = init_rANS(sequence);
		res_rans = encode_rANS(sequence, encoded_sequence, rANSinfo.esyms);
		auto t2_rans = high_resolution_clock::now();
		ms_rans += duration_cast<nanoseconds>(t2_rans - t1_rans).count();
		auto t1_rans_2 = high_resolution_clock::now();
		decode_rANS(rANSinfo.dsyms, rANSinfo.cum2sym, &(*(encoded_sequence.end() - res_rans)), decode_buffer.data(), sequence.size());
		auto t2_rans_2 = high_resolution_clock::now();
		ms_rans2 += duration_cast<nanoseconds>(t2_rans_2 - t1_rans_2).count();
	}
	ms_rans /= iters;
	ms_rans2 /= iters;


	long long ms_ransf = 0, ms_ransf2 = 0, res_ransf = 0;
	for (int i = 0; i < iters; i++) {
		auto t1_ransf = high_resolution_clock::now();
		auto rANSfast_info = init_rANS_fast(sequence);
		res_ransf = encode_rANS_fast(sequence, encoded_sequence, rANSfast_info.esyms);
		auto t2_ransf = high_resolution_clock::now();
		ms_ransf += duration_cast<nanoseconds>(t2_ransf - t1_ransf).count();
		auto t1_ransf_2 = high_resolution_clock::now();
		decode_rANS_fast(rANSfast_info.dsyms, rANSfast_info.cum2sym, &(*(encoded_sequence.end() - res_ransf)), decode_buffer.data(), sequence.size());
		auto t2_ransf_2 = high_resolution_clock::now();
		ms_ransf2 += duration_cast<nanoseconds>(t2_ransf_2 - t1_ransf_2).count();
	}
	ms_ransf /= iters;
	ms_ransf2 /= iters;


	long long ms_ours = 0, ms_ours2 = 0, res_ours = 0;
	std::fill(decode_buffer.begin(), decode_buffer.end(), 0);
	for (int i = 0; i < iters; i++) {
		auto t1_ours = high_resolution_clock::now();
		auto info = init_rANS_with_accuracy_3(sequence);
		res_ours = encode_rANS_with_accuracy_3(sequence, encoded_sequence, info.esyms);
		auto t2_ours = high_resolution_clock::now();
		ms_ours += duration_cast<nanoseconds>(t2_ours - t1_ours).count();
		auto t1_ours_2 = high_resolution_clock::now();
		decode_rANS(info.dsyms.data(), info.cum2sym.data(), encoded_sequence.data() + res_ours, decode_buffer.data(), decode_buffer.data() + sequence.size());
		auto t2_ours_2 = high_resolution_clock::now();
		ms_ours2 += duration_cast<nanoseconds>(t2_ours_2 - t1_ours_2).count();
	}
	ms_ours /= iters;
	ms_ours2 /= iters;
	if (!std::equal(sequence.begin(), sequence.end(), decode_buffer.begin()))
		std::cout << "ERROR! sequence decompressed incorrectly by rANS with accuracy 3" << std::endl;


	long long ms_oursX = 0, ms_ours2X = 0, res_oursX = 0;
	std::fill(decode_buffer.begin(), decode_buffer.end(), 0);
	for (int i = 0; i < iters; i++) {
		auto t1_oursX = high_resolution_clock::now();
		auto infoX = init_rANS_with_accuracy_2(sequence);
		res_oursX = encode_rANS_with_accuracy_2(sequence, encoded_sequence, infoX.esyms);
		auto t2_oursX = high_resolution_clock::now();
		ms_oursX += duration_cast<nanoseconds>(t2_oursX - t1_oursX).count();
		auto t1_ours_2X = high_resolution_clock::now();
		decode_rANS_2(infoX.dsyms.data(), infoX.cum2sym.data(), encoded_sequence.data() + res_oursX, decode_buffer.data(), decode_buffer.data() + sequence.size());
		auto t2_ours_2X = high_resolution_clock::now();
		ms_ours2X += duration_cast<nanoseconds>(t2_ours_2X - t1_ours_2X).count();
	}
	ms_oursX /= iters;
	ms_ours2X /= iters;
	if (!std::equal(sequence.begin(), sequence.end(), decode_buffer.begin()))
		std::cout << "ERROR! sequence decompressed incorrectly by rANS with accuracy 2" << std::endl;


	std::cout << "Comp/decomp time rANS with acc 3: " << ms_ours  << "/" << ms_ours2  << " ns, compressed len: " << res_ours << std::endl;
	std::cout << "Comp/decomp time rANS with acc 2: " << ms_oursX << "/" << ms_ours2X << " ns, compressed len: " << res_oursX << std::endl;
	std::cout << "Comp/decomp time rANS:            " << ms_rans  << "/" << ms_rans2  << " ns, compressed len: " << res_rans << std::endl;
	std::cout << "Comp/decomp time rANS fast:       " << ms_ransf << "/" << ms_ransf2 << " ns, compressed len: " << res_ransf << std::endl << std::endl;
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

	for (int i = 0; i < sequence.size(); i++)
		sequence[i] = enwiki16kb[i];
	test_sequence(sequence);

}

