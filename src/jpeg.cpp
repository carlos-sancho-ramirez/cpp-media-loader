
#include "jpeg.hpp"
#include "instream.hpp"

#include <cmath>

// Assumed for 8x8 matrixes
const quantization_table::cell_index_fast_t zigzag_level_baseline[] =
		{0, 1, 3, 6, 10, 15, 21, 28, 35, 41, 46, 50, 53, 55, 56};

quantization_table::cell_index_fast_t quantization_table::zigzag_position(side_index_fast_t x, side_index_fast_t y)
{
	typedef bounded_integer<side_index_t::MIN_VALUE * 2, side_index_t::MAX_VALUE * 2> level_t;
	typedef typename level_t::fast level_fast_t;

	const level_fast_t level = x + y;
	const bool odd_level = level & 1;
	const cell_index_fast_t level_baseline = zigzag_level_baseline[level];
	return level_baseline + (odd_level? y : x);
}

quantization_table::quantization_table(std::istream &stream)
{
	unsigned char zigzag[CELL_AMOUNT];
	stream.read(reinterpret_cast<char *>(zigzag), sizeof(zigzag));

	cell_count_fast_t k = 0;
	for (side_count_fast_t y = 0; y < SIDE; y++)
	{
		for (side_count_fast_t x = 0; x < SIDE; x++)
		{
			matrix[k++] = zigzag[zigzag_position(x, y)];
		}
	}
}

void quantization_table::print(std::ostream &stream)
{
	cell_count_fast_t k = 0;

	for (side_count_fast_t y = 0; y < SIDE; y++)
	{
		stream << "  [";
		for (side_count_fast_t x = 0; x < SIDE - 1; x++)
		{
			stream << '\t' << static_cast<unsigned int>(matrix[k++]) << ',';
		}
		stream << '\t' << static_cast<unsigned int>(matrix[k++]) << "\t]" << std::endl;
	}
}

huffman_table::huffman_table(std::istream &stream)
{
	stream.read(reinterpret_cast<char *>(symbols_per_size), MAX_WORD_SIZE);

	unsigned int all_symbol_amount = 0;
	for (uint_fast8_t index = 0; index < MAX_WORD_SIZE; index++)
	{
		all_symbol_amount += symbols_per_size[index];
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

huffman_table::symbol_count_t huffman_table::symbol_amount() const
{
	return _symbol_amount;
}

uint_fast16_t huffman_table::expected_byte_size() const
{
	return _symbol_amount + MAX_WORD_SIZE + 3;
}

huffman_table::symbol_entry_t huffman_table::start_entry_for_size(const unsigned int size) const
{
	symbol_entry_t entry = 0;
	for (unsigned int size_index = 1; size_index < size; size_index++)
	{
		const unsigned int symbols = symbols_per_size[size_index - 1];
		entry += symbols * (1 << (MAX_WORD_SIZE - size_index));
	}

	return entry;
}

bool huffman_table::find_symbol(const symbol_entry_t &entry, const unsigned int size, symbol_value_t &symbol) const
{
	const symbol_entry_t start_for_this_size = start_entry_for_size(size);
	const symbol_entry_t start_for_next_size = start_entry_for_size(size + 1);

	if (start_for_this_size > entry || start_for_next_size <= entry)
	{
		return false;
	}

	const symbols_per_size_t relative = (entry - start_for_this_size) >> (MAX_WORD_SIZE - size);
	symbol_index_t index = relative;
	for (unsigned int i = 1; i < size; i++)
	{
		index += symbols_per_size[i - 1];
	}

	symbol = symbols[index];
	return true;
}

huffman_table::symbol_value_t huffman_table::next_symbol(scan_bit_stream &bit_stream) const throw(std::invalid_argument)
{
	unsigned int size = 0;
	symbol_entry_t entry = 0;
	while (++size < MAX_WORD_SIZE)
	{
		entry |= ((unsigned int) bit_stream.next_bit()) << (MAX_WORD_SIZE - size);
		symbol_value_t symbol;
		if (find_symbol(entry, size, symbol))
		{
			return symbol;
		}
	}

	throw std::invalid_argument("Symbol not found in huffman table");
}

frame_info::frame_info(std::istream &stream, const table_list<quantization_table> &tables)
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

scan_info::scan_info(std::istream &stream, const table_list<huffman_table> &dc_tables,
		const table_list<huffman_table> &ac_tables)
{
	channels_amount = stream.get();
	channels = new scan_channel[channels_amount];

	for (uint_fast8_t channel_index = 0; channel_index < channels_amount; channel_index++)
	{
		// Due to I am unable to find proper specifications I am not fully sure to be extracting the
		// channels properly.
		// TODO: This must be ensured that works for other configuration of channels than YCbCr and with asymetric sample (2x1, 1x2,...)

		scan_channel &channel = channels[channel_index];
		channel.channel_type = static_cast<frame_channel::channel_type_e>(stream.get());

		const uint_fast8_t table_ref = stream.get();
		const table_list<huffman_table>::index_fast_t table_index = table_ref & 0x0F;
		channel.dc_table = dc_tables.list[table_index];
		channel.ac_table = ac_tables.list[table_index];
	}

	// This is always 0x00 0x3F 0x00, but I do not know why
	char ignore_buffer[3];
	stream.read(ignore_buffer, 3);
}

uint_fast16_t scan_info::expected_byte_size() const
{
	return 6 + 2 * channels_amount;
}

scan_bit_stream::scan_bit_stream(std::istream *stream) : stream(stream), valid_bits(0)
{ }

void scan_bit_stream::prepend(const unsigned char value)
{
	valid_bits += 8;
	last = (last >> 8) + value;
}

unsigned char scan_bit_stream::next_bit() throw(unsupported_feature)
{
	if (valid_bits == 0)
	{
		last = stream->get();
		valid_bits = 8;

		if (last == jpeg_marker::MARKER)
		{
			unsigned char marker_type = stream->get();
			if (marker_type != 0)
			{
				// TODO: Supporting marker within the the scan data should be allowed
				throw unsupported_feature();
			}
		}
	}

	return (last >> --valid_bits) & 1;
}

scan_bit_stream::number_t scan_bit_stream::next_number(const number_bit_amount_t bits)
{
	number_t result = 0;
	for (number_bit_amount_t bit = 0; bit < bits; bit++)
	{
		result = (result << 1) + next_bit();
	}

	return result;
}

namespace {
struct block_matrix
{
	enum
	{
		SIDE = 8,
		CELLS = SIDE * SIDE
	};

	typedef typename bounded_integer<0, SIDE - 1>::fast side_index_fast_t;
	typedef typename bounded_integer<0, SIDE>::fast side_count_fast_t;

	typedef typename bounded_integer<0, CELLS - 1>::fast cell_index_fast_t;
	typedef typename bounded_integer<0, CELLS>::fast cell_count_fast_t;

	typedef double element_t;

private:
	element_t matrix[CELLS];

public:
	block_matrix()
	{
		cell_count_fast_t index;
		for (index = 0; index < CELLS; index++)
		{
			matrix[index] = 0.0f;
		}
	}

	static const cell_index_fast_t zigzag_to_real[];

	void set_at_zigzag(const cell_index_fast_t index, float value)
	{
		const cell_index_fast_t position = zigzag_to_real[index];
		matrix[position] = value;
	}

	cell_index_fast_t get_index(const side_index_fast_t x, const side_index_fast_t y) const
	{
		return y * SIDE + x;
	}

	element_t get(const side_index_fast_t x, const side_index_fast_t y) const
	{
		return matrix[get_index(x,y)];
	}

	void set(const side_index_fast_t x, const side_index_fast_t y, const element_t value)
	{
		matrix[get_index(x,y)] = value;
	}

	block_matrix operator*(const element_t value) const
	{
		block_matrix result;

		for (cell_count_fast_t index=0; index < CELLS; index++)
		{
			result.matrix[index] = matrix[index] * value;
		}

		return result;
	}

	block_matrix &operator=(const block_matrix &other)
	{
		for (cell_count_fast_t index=0; index < CELLS; index++)
		{
			matrix[index] = other.matrix[index];
		}

		return *this;
	}

	block_matrix &operator+=(const element_t &value)
	{
		for (cell_count_fast_t index=0; index < CELLS; index++)
		{
			matrix[index] += value;
		}

		return *this;
	}

	block_matrix &operator-=(const element_t &value)
	{
		for (cell_count_fast_t index=0; index < CELLS; index++)
		{
			matrix[index] -= value;
		}

		return *this;
	}

	block_matrix operator+(const block_matrix &other) const
	{
		block_matrix result;

		for (cell_count_fast_t index=0; index < CELLS; index++)
		{
			result.matrix[index] = matrix[index] + other.matrix[index];
		}

		return result;
	}

	block_matrix operator-(const block_matrix &other) const
	{
		block_matrix result;

		for (cell_count_fast_t index=0; index < CELLS; index++)
		{
			result.matrix[index] = matrix[index] - other.matrix[index];
		}

		return result;
	}
};

const block_matrix::cell_index_fast_t block_matrix::zigzag_to_real[] =
{
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,	53, 60, 61, 54, 47, 55, 62, 63
};

const double PI = 3.141592653589793;

void apply_DCT_single_element(block_matrix &dest, const block_matrix &orig,
		const block_matrix::side_index_fast_t u, const block_matrix::side_index_fast_t v)
{
	const block_matrix::element_t multiplying_arg = PI / (2 * block_matrix::SIDE);
	//const block_matrix::element_t c_u = sqrt(((u == 0)? 1.0 : 2.0) / block_matrix::SIDE);
	//const block_matrix::element_t c_v = sqrt(((v == 0)? 1.0 : 2.0) / block_matrix::SIDE);
	const block_matrix::element_t c_u = sqrt(2.0 / block_matrix::SIDE);
	const block_matrix::element_t c_v = sqrt(2.0 / block_matrix::SIDE);

	block_matrix::element_t result = 0;
	for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
	{
		for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
		{
			const block_matrix::element_t h_cos_arg = (2 * u + 1) * x;
			const block_matrix::element_t v_cos_arg = (2 * v + 1) * y;
			const block_matrix::element_t element = orig.get(x,y) *
					cos(multiplying_arg * h_cos_arg) * cos(multiplying_arg * v_cos_arg);
			result += element;
		}
	}

	result *= c_u * c_v;
	dest.set(u, v, result);
}

block_matrix apply_DCT(const block_matrix &input)
{
	block_matrix result;

	for (block_matrix::side_count_fast_t v = 0; v < block_matrix::SIDE; v++)
	{
		for (block_matrix::side_count_fast_t u = 0; u < block_matrix::SIDE; u++)
		{
			apply_DCT_single_element(result, input, u, v);
		}
	}

	return result;
}

}

rgb888_color *decode_into_RGB_image(scan_bit_stream &stream, frame_info &frame, scan_info &scan)
{
	unsigned int pixels = frame.width;
	pixels *= frame.height;
	rgb888_color *bitmap = new rgb888_color[pixels];

	/*
	uint_fast16_t x_position = 0;
	uint_fast16_t y_position = 0;
	*/

	block_matrix *matrices = new block_matrix[scan.channels_amount];

	for (scan_info::channel_count_t channel=0; channel<scan.channels_amount; channel++)
	{
		const scan_channel &scan_channel = scan.channels[channel];
		huffman_table::symbol_value_t dc_length = scan_channel.dc_table->next_symbol(stream);
		const scan_bit_stream::number_t number = stream.next_number(dc_length);

		int dc_value = number;
		const bool is_negative = (number & (1 << (dc_length - 1))) == 0;
		if (is_negative) dc_value = -(number ^ ((1 << dc_length) - 1));

		block_matrix dct_matrix;
		dct_matrix.set_at_zigzag(0, dc_value);
		matrices[channel] = apply_DCT(dct_matrix) * (4.0 / block_matrix::SIDE);

		huffman_table::symbol_value_t ac_symbol = scan_channel.ac_table->next_symbol(stream);
		// TODO: AC values handling

		// Temporal code to check all is fine
		std::cout << "First DC value has " << static_cast<unsigned int>(dc_length) << " bits and value "
						<< dc_value << " (number=" << static_cast<unsigned int>(number) << ")" << std::endl;
		std::cout << "First AC value has value " << static_cast<unsigned int>(ac_symbol) << std::endl;

		std::cout << "DCT channel matrix is:" << std::endl;
		for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
		{
			std::cout << "  [";
			for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
			{
				std::cout << dct_matrix.get(x, y) << ",\t";
			}
			std::cout << "]" << std::endl;
		}

		std::cout << "Resulting channel matrix is:" << std::endl;
		for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
		{
			std::cout << "  [";
			for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
			{
				std::cout << matrices[channel].get(x, y) << ",\t";
			}
			std::cout << "]" << std::endl;
		}
	}

	// Assumed it is YCbCr
	if (scan.channels_amount == 3)
	{
		matrices[0] += 128;

		block_matrix red = matrices[0] + (matrices[2] * 1.402);
		block_matrix green = matrices[0] - (matrices[1] * 0.034414) - (matrices[2] * 0.71414);
		block_matrix blue = matrices[0] + (matrices[1] * 1.772);

		std::cout << "Resulting red matrix is:" << std::endl;
		for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
		{
			std::cout << "  [";
			for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
			{
				std::cout << red.get(x, y) << ",\t";
			}
			std::cout << "]" << std::endl;
		}

		std::cout << "Resulting green matrix is:" << std::endl;
		for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
		{
			std::cout << "  [";
			for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
			{
				std::cout << green.get(x, y) << ",\t";
			}
			std::cout << "]" << std::endl;
		}

		std::cout << "Resulting blue matrix is:" << std::endl;
		for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
		{
			std::cout << "  [";
			for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
			{
				std::cout << blue.get(x, y) << ",\t";
			}
			std::cout << "]" << std::endl;
		}
	}

	delete[] matrices;

	// TODO: This should be repeated until filling the whole image
	return bitmap;
}
