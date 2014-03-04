
#include "jpeg.hpp"
#include "stream_utils.hpp"
#include "huffman_tables.hpp"
#include "block_matrix.hpp"
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

namespace {
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

void setImageBlock(const bitmap &bitmap, int x_pos, int y_pos, block_matrix *components)
{
	const unsigned int width = bitmap.width;
	const unsigned int height = bitmap.height;
	const unsigned int component_amount = bitmap.components_amount;

	bitmap::component_value_t *component_buffer = new bitmap::component_value_t[component_amount];

	for (block_matrix::side_count_fast_t row = 0; row < block_matrix::SIDE; row++)
	{
		const int bitmap_row = row + y_pos;
		if (bitmap_row >= 0 && static_cast<unsigned int>(bitmap_row) < height)
		{
			for (block_matrix::side_count_fast_t column = 0; column < block_matrix::SIDE; column++)
			{
				const int bitmap_column = column + x_pos;
				if (bitmap_column >= 0 && static_cast<unsigned int>(bitmap_column) < width)
				{
					for (unsigned int index = 0; index < component_amount; index++)
					{
						component_buffer[index] = components[index].get(column, row);
					}
					bitmap.setPixel(bitmap_column, bitmap_row, component_buffer);
				}
			}
		}
	}

	delete []component_buffer;
}

}

void jpeg::decode_image(bitmap &bitmap, scan_bit_stream &stream, frame_info &frame, scan_info &scan)
{
	unsigned int pixels = frame.width;
	pixels *= frame.height;

	// TODO: This should be repeated until filling the whole image

	uint_fast16_t x_position = 0;
	uint_fast16_t y_position = 0;

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

		block_matrix *rgb_components = new block_matrix[3];
		rgb_components[0] = matrices[0] + (matrices[2] * 1.402);
		rgb_components[1] = matrices[0] - (matrices[1] * 0.034414) - (matrices[2] * 0.71414);
		rgb_components[2] = matrices[0] + (matrices[1] * 1.772);

		setImageBlock(bitmap, x_position, y_position, rgb_components);

		for (unsigned int index = 0; index < 3; index++)
		{
			std::cout << "Resulting matrix " << index << " is:" << std::endl;
			for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
			{
				std::cout << "  [";
				for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
				{
					std::cout << rgb_components[index].get(x, y) << ",\t";
				}
				std::cout << "]" << std::endl;
			}
		}
	}

	delete[] matrices;
}
