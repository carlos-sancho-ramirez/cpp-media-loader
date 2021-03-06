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
#include <sstream>

namespace
{

void decode_image(bitmap &bitmap, std::ostream &stream, const std::string &filename)
{
	std::stringstream path;
	path << "test" PROJECT_PATH_FOLDER_SEPARATOR "res" PROJECT_PATH_FOLDER_SEPARATOR;
	path << filename;
	std::string path_str = path.str();

	std::ifstream in_stream(path_str);
	if (in_stream.fail())
	{
		stream << "Unable to open file " << path_str;
		throw 0;
	}

	try
	{
		jpeg::decode_image(bitmap, in_stream);
	}
	catch (jpeg::invalid_file_format)
	{
		in_stream.close();
		stream << "File " << path_str << " is not a valid JPEG file" << std::endl;
		throw 0;
	}

	in_stream.close();
}

#define ASSERT(CONDITION, MESSAGE, STREAM) \
	if (!(CONDITION)) \
	{ \
		STREAM << MESSAGE << std::endl; \
		throw 0; \
	}

void test_file(std::ostream &stream, const std::string &filename, const unsigned int expectedWidth,
		const unsigned int expectedHeight, const unsigned int expectedComponentAmount,
		const std::function<void (int column, int row, bitmap::component_value_t *components)> &lambda_check)
{
	bitmap bitmap;
	decode_image(bitmap, stream, filename);

	ASSERT(bitmap.components_amount == expectedComponentAmount, "Invalid amount of components", stream);
	ASSERT(bitmap.width == expectedWidth, "Invalid width", stream);
	ASSERT(bitmap.height == expectedHeight, "Invalid height", stream);

	shared_array<bitmap::component_value_t> components =
			shared_array<bitmap::component_value_t>::make(new bitmap::component_value_t[expectedComponentAmount]);

	for (unsigned int row = 0; row < expectedHeight; row++)
	{
		for (unsigned int column = 0; column < expectedWidth; column++)
		{
			bitmap.getPixel(column, row, components.get());
			lambda_check(column, row, components.get());
		}
	}
}

const float color_high_threshold = 0.8f;
const float color_low_threshold = 0.2f;

void test_black_8x8_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "black8x8.jpg", 8, 8, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		for (unsigned int component = 0; component < expectedComponentAmount; component++)
		{
			ASSERT(components[component] < color_low_threshold, "Found pixels that are not black", stream);
		}
	});
}

void test_white_8x8_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "white8x8.jpg", 8, 8, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		for (unsigned int component = 0; component < expectedComponentAmount; component++)
		{
			ASSERT(components[component] > color_high_threshold, "Found pixels that are not white", stream);
		}
	});
}

void test_red_8x8_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "red8x8.jpg", 8, 8, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		ASSERT(components[0] > color_high_threshold && components[1] < color_low_threshold &&
				components[2] < color_low_threshold, "Found pixels that are not red", stream);
	});
}

void test_green_8x8_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "green8x8.jpg", 8, 8, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		ASSERT(components[0] < color_low_threshold && components[1] > color_high_threshold &&
				components[2] < color_low_threshold, "Found pixels that are not green", stream);
	});
}

void test_2x2_plain_blocks(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "colors_dc16x16.jpg", 16, 16, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		if (column < 8 && row < 8)
		{
			ASSERT(components[0] > color_high_threshold && components[1] < color_low_threshold &&
					components[2] < color_low_threshold, "Upper-left corner has pixels that are not red", stream);
		}
		else if (column >= 8 && row < 8)
		{
			ASSERT(components[0] < color_low_threshold && components[1] > color_high_threshold &&
					components[2] < color_low_threshold, "Upper-right corner has pixels that are not green", stream);
		}
		else if (column < 8 && row >= 8)
		{
			ASSERT(components[0] > color_high_threshold && components[1] > color_high_threshold &&
					components[2] < color_low_threshold, "Lower-left corner has pixels that are not yellow", stream);
		}
		else
		{
			ASSERT(components[0] < color_low_threshold && components[1] < color_low_threshold &&
					components[2] > color_high_threshold, "Lower-right corner has pixels that are not blue", stream);
		}
	});
}

void test_subsample_422_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "black_white_plain_block_compressed_16x16.jpg", 16, 16, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		const bool isWhite = (((column >> 3) + (row >> 3)) & 1) != 0;

		for (unsigned int component = 0; component < expectedComponentAmount; component++)
		{
			if (isWhite)
			{
				ASSERT(components[component] > color_high_threshold, "Expected white pixel", stream);
			}
			else
			{
				ASSERT(components[component] < color_low_threshold, "Expected black pixel", stream);
			}
		}
	});
}

void test_subsample_422_Y12_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "black_white_plain_block_compressed_16x16_Y12.jpg", 16, 16, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		const bool isWhite = (((column >> 3) + (row >> 3)) & 1) != 0;

		for (unsigned int component = 0; component < expectedComponentAmount; component++)
		{
			if (isWhite)
			{
				ASSERT(components[component] > color_high_threshold, "Expected white pixel", stream);
			}
			else
			{
				ASSERT(components[component] < color_low_threshold, "Expected black pixel", stream);
			}
		}
	});
}

void test_black_white_8x8_file(std::ostream &stream)
{
	const unsigned int expectedComponentAmount = 3;
	test_file(stream, "black_white_8x8.jpg", 8, 8, expectedComponentAmount,
			[&] (int column, int row, bitmap::component_value_t *components)
	{
		const unsigned int x_slot = column / 2;
		const unsigned int y_slot = row / 2;
		const bool isWhite = ((x_slot + y_slot) & 1) != 0;

		for (unsigned int component = 0; component < expectedComponentAmount; component++)
		{
			if (isWhite)
			{
				ASSERT(components[component] > color_high_threshold,
						"Found a pixel that is not white in the white area", stream);
			}
			else
			{
				ASSERT(components[component] < color_low_threshold,
						"Found a pixel that is not black in the black area", stream);
			}
		}
	});
}

}

const test_bench_results jpeg::test_bench::run() throw()
{
	std::vector<test_result> vector;
	vector.push_back(test("test for black 8x8 matrix jpeg file", test_black_8x8_file));
	vector.push_back(test("test for white 8x8 matrix jpeg file", test_white_8x8_file));
	vector.push_back(test("test for red 8x8 matrix jpeg file", test_red_8x8_file));
	vector.push_back(test("test for green 8x8 matrix jpeg file", test_green_8x8_file));
	vector.push_back(test("test for 2x2 plain colors blocks", test_2x2_plain_blocks));
	vector.push_back(test("test for mixed black white 8x8 jpeg file", test_black_white_8x8_file));
	vector.push_back(test("test for YCbCr JPEG with subsample 2x1, 1x1, 1x1 (4:2:2) and 70% compression", test_subsample_422_file));
	vector.push_back(test("test for YCbCr JPEG with subsample 1x2, 1x1, 1x1 and 70% compression", test_subsample_422_Y12_file));

	test_result *tests = new test_result[vector.size()];
	test_bench_results result(vector.size(), tests);

	std::copy(vector.begin(), vector.end(), tests);
	return result;
}
