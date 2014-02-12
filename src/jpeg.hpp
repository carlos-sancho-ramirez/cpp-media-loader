
#ifndef JPEG_HPP_
#define JPEG_HPP_

#include "bounded_integers.hpp"
#include <iostream>

struct quantization_table
{
	enum
	{
		SIDE = 8,
		CELL_AMOUNT = SIDE * SIDE
	};

	typedef bounded_integer<0,quantization_table::SIDE - 1> side_t;
	typedef bounded_integer<0,quantization_table::CELL_AMOUNT - 1> cell_t;

	typedef typename side_t::fast side_fast_t;
	typedef typename cell_t::fast cell_fast_t;

private:
	static cell_fast_t zigzag_position(side_fast_t x, side_fast_t y);
	unsigned char matrix[CELL_AMOUNT];

public:
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

	enum
	{
		MAX_SAMPLE_ALLOWED = 15
	};

	typedef typename bounded_integer<0, MAX_SAMPLE_ALLOWED>::fast uint_fast4_t;

	quantization_table *table;
	uint_fast4_t horizontal_sample;
	uint_fast4_t vertical_sample;
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
