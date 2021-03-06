/*
 * main.cpp
 *
 *  Created on: 21/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#include "benches.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include <cmath>

namespace
{
unsigned int passed_tests = 0;
unsigned int failed_tests = 0;

bool test(const std::string title, void (*function)(std::ostream &))
{
	std::stringstream test_stream;
	try
	{
		function(test_stream);
		std::cout << title << "... ok" << std::endl;
		passed_tests++;
		return true;
	}
	catch (...)
	{
		std::cout << title << "... ko" << std::endl;
		std::cerr << test_stream.str() << std::endl;
		failed_tests++;
	}

	return false;
}

}

void testOK(std::ostream &stream)
{
	stream << "textOK executed. And passed, of course!" << std::endl;
}

void testKO(std::ostream &stream)
{
	stream << "textKO executed. And failed, of course!" << std::endl;
	throw 0;
}

#include "block_matrix.hpp"

const block_matrix::element_t element_tolerance = 0.05;

void test_block_matrix_dct(std::ostream &stream, block_matrix &matrix)
{
	block_matrix frequency_space = matrix.extract_dct();
	block_matrix normal_space = frequency_space.extract_inverse_dct();

	stream << matrix.dump() << std::endl;
	stream << frequency_space.dump() << std::endl;
	stream << normal_space.dump() << std::endl;

	for (block_matrix::side_count_fast_t row = 0; row < block_matrix::SIDE; row++)
	{
		for (block_matrix::side_count_fast_t column = 0; column < block_matrix::SIDE; column++)
		{
			block_matrix::element_t orig = matrix.get(column, row);
			block_matrix::element_t final = normal_space.get(column, row);

			if (abs(orig - final) > element_tolerance)
			{
				stream << "For position (" << static_cast<unsigned int>(column) << ','
						<< static_cast<unsigned int>(column) << ") expected value was "
						<< orig << " but actually it was " << final << std::endl;
				throw 0;
			}
		}
	}
}

void test_plain_block_matrix_dct(std::ostream &stream)
{
	block_matrix matrix;
	//matrix -= 43; Y for red
	matrix += 127; //Cr for red
	//matrix.set(0,0, 64);
	test_block_matrix_dct(stream, matrix);
}

void test_black_white_block_matrix_dct(std::ostream &stream)
{
	block_matrix matrix;

	for (block_matrix::side_count_fast_t row = 0; row < block_matrix::SIDE; row++)
	{
		for (block_matrix::side_count_fast_t column = 0; column < block_matrix::SIDE; column++)
		{
			unsigned int x_slot = column >> 1;
			unsigned int y_slot = row >> 1;
			bool isWhite = ((x_slot + y_slot) & 1) != 0;

			matrix.set(column, row, isWhite? 127 : -128);
		}
	}

	test_block_matrix_dct(stream, matrix);
}

int main(int argc, char *argv[])
{
	std::cout << "C++ Media Loader" << std::endl
			<< "This software is under the MIT License" << std::endl
			<< "Version " PROJECT_VERSION_STR << std::endl
			<< std::endl
			<< "Running tests:" << std::endl;

	test("trivial test to test the test bench", testOK);
	//test("test 2", testKO);
	test("test plain block_matrix DCT", test_plain_block_matrix_dct);
	test("test black and white block_matrix DCT", test_black_white_block_matrix_dct);

	const test_bench_results huffman_tables_results = huffman_tables::test_bench().run();
	const unsigned int total_huffman_tables_tests = huffman_tables_results.total();
	for (unsigned int index = 0; index < total_huffman_tables_tests; index++)
	{
		const test_result result = huffman_tables_results.tests[index];
		std::cout << result.title << "... " << (result.passed? "ok" : "ko") << std::endl;
		if (!result.passed)
		{
			std::cerr << result.log << std::endl;
		}
	}

	passed_tests += huffman_tables_results.passed();
	failed_tests += huffman_tables_results.failed();

	const test_bench_results jpeg_results = jpeg::test_bench().run();
	const unsigned int total_jpeg_tests = jpeg_results.total();
	for (unsigned int index = 0; index < total_jpeg_tests; index++)
	{
		const test_result result = jpeg_results.tests[index];
		std::cout << result.title << "... " << (result.passed? "ok" : "ko") << std::endl;
		if (!result.passed)
		{
			std::cerr << result.log << std::endl;
		}
	}

	passed_tests += jpeg_results.passed();
	failed_tests += jpeg_results.failed();

	std::cout << "Total amount of tests run: " << passed_tests + failed_tests << std::endl
			<< "Total amount of passed tests: " << passed_tests << std::endl
			<< "Total amount of failed tests: " << failed_tests << std::endl;

	return (failed_tests != 0)? 1 : 0;
}
