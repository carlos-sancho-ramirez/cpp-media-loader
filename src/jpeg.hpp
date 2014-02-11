
#ifndef JPEG_HPP_
#define JPEG_HPP_

#include <stdint.h>
#include <iostream>

struct quantization_table
{
	enum
	{
		WIDTH = 8,
		HEIGHT = 8
	};

	unsigned char matrix[WIDTH * HEIGHT];

	quantization_table(std::istream &stream);
	void print(std::ostream &stream);
};

struct quantization_table_list
{
	enum
	{
		MAX_TABLES = 16 // This is the maximum number of tables that the jpeg format allows.
	};

	quantization_table *list[MAX_TABLES];
};

class huffman_table
{
public:
	enum
	{
		MAX_WORD_SIZE = 16
	};

private:
	unsigned char symbol_indexes[MAX_WORD_SIZE];
	const unsigned char *symbols;
	uint_fast8_t _symbol_amount;

public:
	huffman_table(std::istream &stream);
	~huffman_table();
	uint_fast8_t symbol_amount() const;
};

struct frame_channel
{
	enum channel_type_e
	{
		LUMINANCE = 1,
		CHROMINANCE_BLUE = 2,
		CHROMINANCE_RED = 3
	};

	quantization_table *table;
	uint_fast8_t horizontal_sample;
	uint_fast8_t vertical_sample;
	channel_type_e channel_type;
};

struct frame_info
{
	uint_fast16_t width;
	uint_fast16_t height;
	uint_fast8_t precision;
	uint_fast8_t channels_amount;
	frame_channel *channels;

	frame_info(std::istream &stream, const quantization_table_list &tables);
	uint_fast16_t expected_byte_size() const;
};

#endif /* JPEG_HPP_ */
