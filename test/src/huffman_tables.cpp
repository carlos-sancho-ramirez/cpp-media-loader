/*
 * huffman.cpp
 *
 *  Created on: 05/04/2014
 *      Author: Carlos Sancho Ramirez
 */

#include "benches.hpp"
#include "huffman_tables.hpp"

#include <sstream>
#include <vector>

void test_plain_sentences(std::ostream &stream)
{
	const char expected_result[] = "Huffman is able to compress this sentence.";

	char sizes[] =
	{
		'\0', '\0', '\003', '\b', '\0', '\b', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
	};

	char symbols[] =
	{
		' ', 'e', 's', 'n', 't', 'f', 'm', 'a', 'i', 'o',
		'c', 'H', 'u', 'b', 'l', 'p', 'r', 'h', '.'
	};

	std::stringstream table_stream;
	for (unsigned int i = 0; i < sizeof(sizes); i++)
	{
		table_stream << sizes[i];
	}

	for (unsigned int i = 0; i < sizeof(symbols); i++)
	{
		table_stream << symbols[i];
	}

	huffman_table table(table_stream);

	char compressed_sentence[] =
	{
			static_cast<char>(0xE3), static_cast<char>(0x98), static_cast<char>(0x89),
			static_cast<char>(0xA6), static_cast<char>(0x16), static_cast<char>(0x85),
			static_cast<char>(0x75), static_cast<char>(0xD9), static_cast<char>(0x0F),
			static_cast<char>(0x83), static_cast<char>(0x72), static_cast<char>(0x7C),
			static_cast<char>(0xF4), static_cast<char>(0xA4), static_cast<char>(0x1F),
			static_cast<char>(0xEB), static_cast<char>(0x41), static_cast<char>(0x16),
			static_cast<char>(0x72), static_cast<char>(0xDA), static_cast<char>(0x7F)
	};

	std::stringstream raw_data_stream;
	for (unsigned int i = 0; i < sizeof(compressed_sentence); i++)
	{
		raw_data_stream << compressed_sentence[i];
	}

	bit_stream data_stream(&raw_data_stream);
	char expected_char = 0;
	unsigned int i = 0;
	while ( (expected_char = expected_result[i++]) != '\0' )
	{
		const char value = static_cast<unsigned char>(table.next_symbol(data_stream));
		if (value != expected_char)
		{
			stream << "Character at position " << i << " has value " << value << " but "
					<< expected_char << " was expected";
			throw 0;
		}
	}
}

const test_bench_results huffman_tables::test_bench::run() throw()
{
	std::vector<test_result> vector;
	vector.push_back(test("Test plain sentences compressed by Huffman", test_plain_sentences));

	test_result *tests = new test_result[vector.size()];
	test_bench_results result(vector.size(), tests);

	std::copy(vector.begin(), vector.end(), tests);
	return result;
}
