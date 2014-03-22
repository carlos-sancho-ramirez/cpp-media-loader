/*
 * main.cpp
 *
 *  Created on: 21/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#include <iostream>
#include <string>

#include <cmath>

namespace
{
bool test(const std::string title, void (*function)(std::ostream &))
{
	try
	{
		function(std::cerr);
		std::cout << title << "... ok" << std::endl;
		return true;
	}
	catch (...)
	{
		std::cout << title << "... ko" << std::endl;
	}

	return false;
}

}

void testOK(std::ostream &stream)
{
	stream << "textOK executed" << std::endl;
}

void testKO(std::ostream &stream)
{
	stream << "textKO executed" << std::endl;
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

void test_block_matrix_dct(std::ostream &stream)
{
	block_matrix matrix;
	matrix += 64;
	//matrix.set(0,0, 64);
	test_block_matrix_dct(stream, matrix);
}

int main(int argc, char *argv[])
{
	test("test 1", testOK);
	test("test 2", testKO);
	test("test block_matrix DCT", test_block_matrix_dct);
}
