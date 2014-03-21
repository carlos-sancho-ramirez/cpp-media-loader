
#ifndef JPEG_HPP_
#define JPEG_HPP_

#include "bounded_integers.hpp"
#include "huffman_tables.hpp"
#include "bitmaps.hpp"

#include <iostream>
#include <stdexcept>

struct quantization_table
{
	enum
	{
		SIDE = 8,
		CELL_AMOUNT = SIDE * SIDE
	};

	typedef bounded_integer<0,quantization_table::SIDE - 1> side_index_t;
	typedef bounded_integer<0,quantization_table::SIDE> side_count_t;
	typedef bounded_integer<0,quantization_table::CELL_AMOUNT - 1> cell_index_t;
	typedef bounded_integer<0,quantization_table::CELL_AMOUNT> cell_count_t;

	typedef typename side_index_t::fast side_index_fast_t;
	typedef typename side_count_t::fast side_count_fast_t;
	typedef typename cell_index_t::fast cell_index_fast_t;
	typedef typename cell_count_t::fast cell_count_fast_t;

private:
	static cell_index_fast_t zigzag_position(side_index_fast_t x, side_index_fast_t y);
	unsigned char matrix[CELL_AMOUNT];

public:
	quantization_table(std::istream &stream);
	void print(std::ostream &stream);
};

template<class TABLE_TYPE>
struct table_list
{
	enum
	{
		MAX_TABLES = 16 // This is the maximum number of tables that the jpeg format allows.
	};

	typedef bounded_integer<0, MAX_TABLES - 1> index_t;
	typedef bounded_integer<0, MAX_TABLES> count_t;

	typedef typename index_t::fast index_fast_t;
	typedef typename count_t::fast count_fast_t;

	TABLE_TYPE *list[MAX_TABLES];

	table_list()
	{
		for (count_fast_t i = 0; i < MAX_TABLES; i++)
		{
			list[i] = NULL;
		}
	}
};

struct basic_channel
{
	enum channel_type_e
	{
		LUMINANCE = 1,
		CHROMINANCE_BLUE = 2,
		CHROMINANCE_RED = 3
	};

	channel_type_e channel_type;
};

struct frame_channel : public basic_channel
{
	enum
	{
		MAX_SAMPLE_ALLOWED = 15
	};

	typedef typename bounded_integer<0, MAX_SAMPLE_ALLOWED>::fast uint_fast4_t;

	quantization_table *table;
	uint_fast4_t horizontal_sample;
	uint_fast4_t vertical_sample;
};

struct scan_channel : public basic_channel
{
	huffman_table *dc_table, *ac_table;
};

struct basic_info
{
	typedef bounded_integer<0,255>::fast channel_index_t;
	typedef bounded_integer<0,256>::fast channel_count_t;

	channel_count_t channels_amount;
	virtual uint_fast16_t expected_byte_size() const = 0;
};

struct frame_info : public basic_info
{
	uint_fast16_t width;
	uint_fast16_t height;
	uint_fast8_t precision;
	frame_channel *channels;

	frame_info(std::istream &stream, const table_list<quantization_table> &tables);
	virtual uint_fast16_t expected_byte_size() const;
};

struct scan_info : public basic_info
{
	scan_channel *channels;

	scan_info(std::istream &stream, const table_list<huffman_table> &dc_tables,
			const table_list<huffman_table> &ac_tables);
	virtual uint_fast16_t expected_byte_size() const;
};

struct rgb888_color
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

namespace jpeg
{
	class invalid_file_format { };

	bitmap *decode_image(std::istream &stream) throw(invalid_file_format);
}

#endif /* JPEG_HPP_ */
