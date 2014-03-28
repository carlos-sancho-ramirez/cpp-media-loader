/*
 * jpeg.cpp
 *
 *  Created on: 27/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#include "benches.hpp"

#include "jpeg.hpp"
#include "bitmaps.hpp"
#include "smart_pointers.hpp"

#include <fstream>
#include <vector>
#include <algorithm>

namespace
{

#define OPEN_FILE(STREAM, FILENAME) \
	std::ifstream in_stream("test" PROJECT_PATH_FOLDER_SEPARATOR "res" PROJECT_PATH_FOLDER_SEPARATOR FILENAME); \
	if (in_stream.fail()) \
	{ \
		STREAM << "Unable to open file " << FILENAME << std::endl; \
		throw 0; \
	}

#define DECODE_IMAGE(STREAM, FILENAME) \
	OPEN_FILE(STREAM, FILENAME) \
	\
	bitmap *bitmap = NULL; \
	try \
	{ \
		bitmap = jpeg::decode_image(in_stream); \
	} \
	catch (jpeg::invalid_file_format) \
	{ \
		in_stream.close(); \
		stream << "File " FILENAME " is not a valid JPEG file" << std::endl; \
		throw 0; \
	} \
	\
	in_stream.close()

#define ASSERT(CONDITION, MESSAGE, STREAM) \
	if (!(CONDITION)) \
	{ \
		STREAM << MESSAGE << std::endl; \
		delete bitmap; \
		throw 0; \
	}

void test_black_8x8_file(std::ostream &stream)
{
	const unsigned int expectedWidth = 8;
	const unsigned int expectedHeight = 8;
	const unsigned int expectedComponentAmount = 3;

	DECODE_IMAGE(stream, "black8x8.jpg");

	ASSERT(bitmap->components_amount == expectedComponentAmount, "Invalid amount of components", stream);
	ASSERT(bitmap->width == expectedWidth, "Expected width was 8", stream);
	ASSERT(bitmap->height == expectedHeight, "Expected height was 8", stream);

	shared_array<bitmap::component_value_t> components =
			shared_array<bitmap::component_value_t>::make(new bitmap::component_value_t[expectedComponentAmount]);

	for (unsigned int row = 0; row < expectedHeight; row++)
	{
		for (unsigned int column = 0; column < expectedWidth; column++)
		{
			bitmap->getPixel(column, row, components.get());

			for (unsigned int component = 0; component < expectedComponentAmount; component++)
			{
				ASSERT(components[component] < 0.25f, "Found pixels that are not black", stream);
			}
		}
	}

	delete bitmap;
}

}

const test_bench_results jpeg::test_bench::run() throw()
{
	std::vector<test_result> vector;
	vector.push_back(test("test for black 8x8 matrix jpeg file", test_black_8x8_file));

	test_result *tests = new test_result[vector.size()];
	test_bench_results result(vector.size(), tests);

	std::copy(vector.begin(), vector.end(), tests);
	return result;
}
