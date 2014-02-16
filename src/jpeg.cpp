
#include "jpeg.hpp"
#include "instream.hpp"

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

rgb888_color *decode_into_RGB_image(scan_bit_stream &stream, frame_info &frame, scan_info &scan)
{
	unsigned int pixels = frame.width;
	pixels *= frame.height;
	rgb888_color *bitmap = new rgb888_color[pixels];

	/*
	uint_fast16_t x_position = 0;
	uint_fast16_t y_position = 0;
	*/

	for (scan_info::channel_count_t channel=0; channel<scan.channels_amount; channel++)
	{
		huffman_table::symbol_value_t dc_length = scan.channels[channel].dc_table->next_symbol(stream);
		const scan_bit_stream::number_t number = stream.next_number(dc_length);

		int dc_value = number;
		const bool is_negative = (number & (1 << (dc_length - 1))) == 0;
		if (is_negative) dc_value = -(number ^ ((1 << dc_length) - 1));

		// Temporal code to check all is fine
		std::cout << "First DC value has " << static_cast<unsigned int>(dc_length) << " bits and value "
				<< dc_value << " (number=" << static_cast<unsigned int>(number) << ")" << std::endl;
		return bitmap;
	}

	return bitmap;
}
