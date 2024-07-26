// project
#include "spsc.hpp"

// c++
#include <fstream>
#include <iostream>
#include <random>
#include <thread>



#define MAX_BUFF_SIZE 10
#define WRITE_LOOPS 10000
#define TEST_FILE_PATH "D:/test.txt"
#define COMPARE_FILE_PATH "D:/compare.txt"


std::vector<uint8_t> characters{
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'`', ';', '\'', ',', '.', '/',
	'~', ':', '"', '<', '>', '?'
};


bool is_file_content_same(const std::string &file1, const std::string &file2, size_t block_size = 4096) {
	std::ifstream file_stream1(file1, std::ios::binary);
	std::ifstream file_stream2(file2, std::ios::binary);

	if (!file_stream1.is_open() || !file_stream2.is_open()) {
		return false;
	}

	file_stream1.seekg(0, std::ios::end);
	file_stream2.seekg(0, std::ios::end);

	if (file_stream1.tellg() != file_stream2.tellg()) {
		return false;
	}

	file_stream1.seekg(0, std::ios::beg);
	file_stream2.seekg(0, std::ios::beg);

	std::vector<char> buf1(block_size);
	std::vector<char> buf2(block_size);
	while (file_stream1.read(buf1.data(), sizeof(char) * block_size) && file_stream2.read(buf2.data(), sizeof(char) * block_size)) {
		if (std::memcmp(buf1.data(), buf2.data(), sizeof(buf1)) != 0) {
			return false;
		}
	}

	return true;
}


bool test_spsc() {
	lock_free_spsc<uint8_t> spsc(MAX_BUFF_SIZE);
	if (spsc.is_buffer_null()) {
		return false;
	}

	uint8_t in = 'X';
	spsc.put(in);
	uint8_t p;
	spsc.peek(p);
	uint8_t out;
	spsc.get(out);
	if (in != p || in != out) {
		return false;
	}

	std::random_device rand_device;
	std::mt19937 reand_engine(rand_device());

	std::atomic<bool> produce_finished = false;

	std::thread produce_thread(
		[&spsc, &produce_finished, &reand_engine]() {
			std::ofstream file_stream(TEST_FILE_PATH);

			std::uniform_int_distribution<> size_distribution(1, MAX_BUFF_SIZE - 1);
			std::uniform_int_distribution<> char_distribution(0, (int)characters.size() - 1);
			for (size_t loops = WRITE_LOOPS; loops > 0; loops--) {
				size_t buffer_size = size_distribution(reand_engine);

				std::vector<uint8_t> buf;
				while (buf.size() < buffer_size) {
					buf.push_back(characters[char_distribution(reand_engine)]);
				}
				buf.push_back('\n');

				uint32_t offset = 0;
				while (offset < buf.size()) {
					uint32_t count = spsc.put(buf.data() + offset, (uint32_t)buf.size() - offset);
					offset += count;
				}

				file_stream.write((const char *)buf.data(), sizeof(uint8_t) * offset);
			}

			produce_finished.store(true);
		}
	);

	std::thread consume_thread(
		[&spsc, &produce_finished, &reand_engine]() {
			std::ofstream file_stream(COMPARE_FILE_PATH);

			std::uniform_int_distribution<> size_distribution(1, MAX_BUFF_SIZE - 1);
			while (!produce_finished) {
				std::vector<uint8_t> buf;
				buf.resize(size_distribution(reand_engine), 0);

				uint32_t offset = 0;
				while (offset < buf.size() && !produce_finished) {
					uint32_t count = spsc.get(buf.data() + offset, (uint32_t)buf.size() - offset);
					offset += count;
				}

				file_stream.write((const char *)buf.data(), sizeof(uint8_t) * offset);
			}

			std::vector<uint8_t> buf;
			buf.resize(MAX_BUFF_SIZE, 0);

			uint32_t offset = spsc.get(buf.data(), (uint32_t)buf.size());
			if (offset > 0) {
				file_stream.write((const char *)buf.data(), sizeof(uint8_t) * offset);
			}
		}
	);

	produce_thread.join();
	consume_thread.join();

	return is_file_content_same(TEST_FILE_PATH, COMPARE_FILE_PATH);
}



int main() {
	if (test_spsc()) {
		std::cout << "lock_free_spsc<uint8_t>(10) test ok" << std::endl;
	}
	else {
		std::cout << "lock_free_spsc<uint8_t>(10) test fail" << std::endl;
	}
}
