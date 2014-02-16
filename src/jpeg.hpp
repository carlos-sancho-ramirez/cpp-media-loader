
#ifndef JPEG_HPP_
#define JPEG_HPP_

#include "bounded_integers.hpp"
#include <iostream>
#include <stdexcept>

namespace jpeg_marker
{
	enum jpeg_marker_e
	{
		START_OF_FRAME_BASELINE_DCT = 0xC0,
		START_OF_FRAME_PROGRESSIVE_DCT = 0xC2,
		HUFFMAN_TABLE = 0xC4,
		RESTART_BASE = 0xD0, // RSTn where n goes from 0 to 7 (0xD0 - 0xD7)
		START_OF_IMAGE = 0xD8,
		END_OF_IMAGE = 0xD9,
		START_OF_SCAN = 0xDA,
		QUANTIZATION_TABLE = 0xDB,
		RESTART_INTERVAL = 0xDD,

		APPLICATION_BASELINE = 0xE0, // APPn where n goes from 0 to 7 (0xE0 - 0xE7)
		JFIF = 0xE0, // APP0 = JFIF
		EXIF = 0xE1, // APP1 = EXIF
		COMMENT = 0xFE, // Plain text comment
		MARKER = 0xFF
	};
}

/**
 * Thrown when this program is unable to continue due to a lack of implementation.
 */
struct unsupported_feature
{ };

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

/**
 * Reads the stream returning it bit per bit in a suitable way for jpeg huffman tables.
 * This class also remove extra 0x00 bytes after 0xff if any.
 */
class scan_bit_stream
{
	enum
	{
		BUFFER_BYTES = 1,
		BUFFER_BITS = 8 * BUFFER_BYTES
	};

	typedef bounded_integer<0, (1 << BUFFER_BITS) - 1>::fast last_t;

	std::istream *stream;

	last_t last;
	typename bounded_integer<0, BUFFER_BITS>::fast valid_bits;

public:
	typedef unsigned int number_t;
	typedef typename bounded_integer<0, sizeof(number_t) * 8>::fast number_bit_amount_t;

public:
	scan_bit_stream(std::istream *stream);
	void prepend(const unsigned char value);
	unsigned char next_bit() throw(unsupported_feature);
	number_t next_number(const number_bit_amount_t bits);
};

class huffman_table
{
public:
	enum
	{
		MAX_WORD_SIZE = 16
	};
	typedef bounded_integer<0, (1 << MAX_WORD_SIZE) - 1>::fast symbol_entry_t;
	typedef bounded_integer<0, 255>::fast symbols_per_size_t;
	typedef unsigned char symbol_value_t;
	typedef uint_fast16_t symbol_index_t;
	typedef uint_fast16_t symbol_count_t;

private:
	symbols_per_size_t symbols_per_size[MAX_WORD_SIZE];
	const symbol_value_t *symbols;
	symbol_count_t _symbol_amount;

	symbol_entry_t start_entry_for_size(const unsigned int size) const;
	bool find_symbol(const symbol_entry_t &entry, const unsigned int size, symbol_value_t &symbol) const;

public:
	huffman_table(std::istream &stream);
	~huffman_table();
	symbol_count_t symbol_amount() const;
	uint_fast16_t expected_byte_size() const;

	/**
	 * Checks the stream, determines the symbol and retuns it.
	 * std::invalid_argument will be thrown if the huffman code is not present in the table.
	 */
	symbol_value_t next_symbol(scan_bit_stream &bit_stream) const throw(std::invalid_argument);
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

rgb888_color *decode_into_RGB_image(scan_bit_stream &stream, frame_info &frame, scan_info &scan);

#endif /* JPEG_HPP_ */
