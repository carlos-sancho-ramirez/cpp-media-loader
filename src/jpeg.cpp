
#include "jpeg.hpp"
#include "instream.hpp"

#include <stdint.h>

// Assumed for 8x8 matrixes
const uint_fast8_t zigzag_level_baseline[] = {0, 1, 3, 6, 10, 15, 21, 28, 35, 41, 46, 50, 53, 55, 56};

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

huffman_table::huffman_table(std::istream &stream)
{
	unsigned char symbols_per_word_size[MAX_WORD_SIZE];
	stream.read(reinterpret_cast<char *>(symbols_per_word_size), MAX_WORD_SIZE);

	int all_symbol_amount = 0;
	for (uint_fast8_t index = 0; index < MAX_WORD_SIZE; index++)
	{
		symbol_indexes[index] = all_symbol_amount;
		all_symbol_amount += symbols_per_word_size[index];
	}

	unsigned char *all_symbols = new unsigned char[all_symbol_amount];
	stream.read(reinterpret_cast<char *>(all_symbols), all_symbol_amount);

	symbols = all_symbols;
	_symbol_amount = all_symbol_amount;
}

huffman_table::~huffman_table()
{
	delete[] symbols;
}

uint_fast8_t huffman_table::symbol_amount() const
{
	return _symbol_amount;
}

frame_info::frame_info(std::istream &stream, const quantization_table_list &tables)
{
	precision = stream.get();
	height = read_big_endian_unsigned_int(stream, 2);
	width = read_big_endian_unsigned_int(stream, 2);

	channels_amount = stream.get();
	channels = new frame_channel[channels_amount];

	for (uint_fast8_t channel_index = 0; channel_index < channels_amount; channel_index++)
	{
		// Due to I am unable to find proper specifications I am not fully sure to be extracting the
		// channels properly.
		// TODO: This must be ensured that works for other configuration of channels than YCbCr and with asymetric sample (2x1, 1x2,...)

		frame_channel &channel = channels[channel_index];
		channel.channel_type = static_cast<frame_channel::channel_type_e>(stream.get());

		const uint_fast8_t samples = stream.get();
		channel.horizontal_sample = (samples >> 4) & 0x0F;
		channel.vertical_sample = samples & 0x0F;

		const uint_fast8_t quantization_table_index = stream.get();
		channel.table = tables.list[quantization_table_index];
	}
}

uint_fast16_t frame_info::expected_byte_size() const
{
	return 8 + 3 * channels_amount;
}
