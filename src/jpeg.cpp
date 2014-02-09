
#include "jpeg.hpp"
#include <stdint.h>

// Assumed for 8x8 matrixes
const uint_fast8_t zigzag_level_baseline[] = {0, 1, 3, 6, 10, 15, 21, 28, 36, 43, 49, 54, 58, 61, 63};

uint_fast8_t zigzag_position(uint_fast8_t x, uint_fast8_t y)
{
	const uint_fast8_t level = x + y;
	const bool odd_level = level & 1;
	const uint_fast8_t level_baseline = zigzag_level_baseline[level];
	return level_baseline + (odd_level? y : x);
}

quantization_table::quantization_table(std::istream &stream)
{
	unsigned char zigzag[WIDTH * HEIGHT];
	stream.read(reinterpret_cast<char *>(zigzag), sizeof(zigzag));

	uint_fast8_t k = 0;
	for (uint_fast8_t y = 0; y < HEIGHT; y++)
	{
		for (uint_fast8_t x = 0; x < WIDTH; x++)
		{
			matrix[k++] = zigzag[zigzag_position(x, y)];
		}
	}
}

void quantization_table::print(std::ostream &stream)
{
	uint_fast8_t k = 0;

	for (uint_fast8_t y = 0; y < HEIGHT; y++)
	{
		stream << "  [";
		for (uint_fast8_t x = 0; x < WIDTH - 1; x++)
		{
			stream << '\t' << static_cast<unsigned int>(matrix[k++]) << ',';
		}
		stream << '\t' << static_cast<unsigned int>(matrix[k++]) << "\t]" << std::endl;
	}
}
