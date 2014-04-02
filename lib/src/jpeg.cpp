
#include "jpeg.hpp"
#include "stream_utils.hpp"
#include "huffman_tables.hpp"
#include "block_matrix.hpp"
#include "jpeg_markers.hpp"
#include "jfif.hpp"
#include "stream_utils.hpp"

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

void quantization_table::multiply_block(block_matrix &block) const
{
	cell_count_fast_t index = 0;
	for (block_matrix::side_count_fast_t row = 0; row < block_matrix::SIDE; row++)
	{
		for (block_matrix::side_count_fast_t column = 0; column < block_matrix::SIDE; column++)
		{
			block_matrix::element_t value = block.get(column, row);
			value *= matrix[index++];
			block.set(column, row, value);
		}
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

void setImageBlock(const bitmap &bitmap, int x_pos, int y_pos, const block_matrix * const components)
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

void decode_scan_data(bitmap &bitmap, scan_bit_stream &stream, frame_info &frame, scan_info &scan)
{
	unsigned int pixels = frame.width;
	pixels *= frame.height;

	uint_fast16_t x_position = 0;
	uint_fast16_t y_position = 0;

	unsigned int matrices_per_iteration = 0;
	unsigned int h_matrices_per_iteration = 1;
	unsigned int v_matrices_per_iteration = 1;
	for (frame_info::channel_count_t index = 0; index < frame.channels_amount; index++)
	{
		const frame_channel &channel = frame.channels[index];
		const frame_channel::uint_fast4_t h_sample = channel.horizontal_sample;
		const frame_channel::uint_fast4_t v_sample = channel.vertical_sample;

		matrices_per_iteration += h_sample * v_sample;

		if (h_sample > h_matrices_per_iteration)
		{
			h_matrices_per_iteration = h_sample;
		}

		if (v_sample > v_matrices_per_iteration)
		{
			v_matrices_per_iteration = v_sample;
		}
	}

	shared_array<block_matrix> matrices = shared_array<block_matrix>::make(new block_matrix[matrices_per_iteration]);
	shared_array<int> dc_values = shared_array<int>::make(new int[scan.channels_amount]);
	for (scan_info::channel_count_t index = 0; index < scan.channels_amount; index++)
	{
		dc_values[index] = 0;
	}

	while (y_position < frame.height)
	{
		unsigned int matrix_index = 0;

		for (scan_info::channel_count_t channel=0; channel<scan.channels_amount; channel++)
		{
			const frame_channel &frame_channel = frame.channels[channel];
			for (frame_channel::uint_fast4_t v_sample = 0; v_sample < frame_channel.vertical_sample; v_sample++)
			{
				for (frame_channel::uint_fast4_t h_sample = 0; h_sample < frame_channel.horizontal_sample; h_sample++)
				{
					const scan_channel &scan_channel = scan.channels[channel];
					huffman_table::symbol_value_t dc_length = scan_channel.dc_table->next_symbol(stream);
					scan_bit_stream::number_t dc_value = stream.next_number(dc_length);

					dc_value += dc_values[channel];
					dc_values[channel] = dc_value;

					block_matrix dct_matrix;
					dct_matrix.set_at_zigzag(0, dc_value);

					unsigned char ac_length;
					unsigned char previous_zeroes;
					block_matrix::cell_index_fast_t read_cells = 0;
					do
					{
						huffman_table::symbol_value_t ac_symbol = scan_channel.ac_table->next_symbol(stream);
						ac_length = ac_symbol & 0x0F;
						previous_zeroes = (ac_symbol >> 4) & 0x0F;

						if (previous_zeroes + read_cells + 1 >= block_matrix::CELLS)
						{
							throw jpeg::invalid_file_format();
						}
						read_cells += previous_zeroes + 1;

						if (ac_length != 0)
						{
							const scan_bit_stream::number_t ac_value = stream.next_number(ac_length);
							dct_matrix.set_at_zigzag(read_cells, ac_value);
						}
					} while((ac_length > 0 || previous_zeroes > 0) && read_cells < block_matrix::CELLS - 1);

					frame_channel.table->multiply_block(dct_matrix);
					matrices[matrix_index++] = dct_matrix.extract_inverse_dct();

					/*
					// Temporal code to check all is fine
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
					*/
				}
			}
		}

		// Assumed it is YCbCr
		if (scan.channels_amount == 3)
		{
			block_matrix ycbcr_components[3];
			block_matrix rgb_components[3];

			for (unsigned int y_on_iteration = 0; y_on_iteration < v_matrices_per_iteration; y_on_iteration++)
			{
				for (unsigned int x_on_iteration = 0; x_on_iteration < h_matrices_per_iteration; x_on_iteration++)
				{
					// TODO: ycbcr should be filled stretching matrices
					unsigned int channel_matrix_index = 0;
					for (frame_info::channel_count_t channel_index = 0; channel_index < frame.channels_amount; channel_index++)
					{
						const frame_channel &channel = frame.channels[channel_index];
						const frame_channel::uint_fast4_t h_sample = channel.horizontal_sample;
						const frame_channel::uint_fast4_t v_sample = channel.vertical_sample;

						unsigned int x = (x_on_iteration < h_sample)? x_on_iteration : h_sample - 1;
						unsigned int y = (y_on_iteration < v_sample)? y_on_iteration : v_sample - 1;

						ycbcr_components[channel_index] = matrices[channel_matrix_index + y * h_sample + x];
						channel_matrix_index += h_sample * v_sample;
					}

					ycbcr_components[0] += 128;

					rgb_components[0] = ycbcr_components[0] + (ycbcr_components[2] * 1.402);
					rgb_components[1] = ycbcr_components[0] - (ycbcr_components[1] * 0.034414) - (ycbcr_components[2] * 0.71414);
					rgb_components[2] = ycbcr_components[0] + (ycbcr_components[1] * 1.772);

					// Normalizing values from 0..255 to 0..1
					for (scan_info::channel_count_t channel = 0; channel < scan.channels_amount; channel++)
					{
						rgb_components[channel] /= 255;
					}

					setImageBlock(bitmap, x_position + x_on_iteration * block_matrix::SIDE, y_position + y_on_iteration * block_matrix::SIDE, rgb_components);

					/*
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
					*/
				}
			}
		}

		x_position += block_matrix::SIDE * h_matrices_per_iteration;
		if (x_position >= frame.width)
		{
			x_position = 0;
			y_position += block_matrix::SIDE * v_matrices_per_iteration;
		}
	}
}
}

void jpeg::decode_image(bitmap &bitmap, std::istream &stream) throw(invalid_file_format)
{
	if (stream.get() != jpeg_marker::MARKER || stream.get() != jpeg_marker::START_OF_IMAGE)
	{
		throw invalid_file_format();
	}

	table_list<quantization_table> tables;
	table_list<huffman_table> dc_tables;
	table_list<huffman_table> ac_tables;

	frame_info *current_frame = NULL;
	scan_info *current_scan = NULL;

	unsigned char value;
	while (stream.good() && (value = stream.get()) == jpeg_marker::MARKER)
	{
		const uint_fast8_t marker_type = stream.get();
		const uint_fast16_t size = read_big_endian_unsigned_int(stream, 2);

		switch (marker_type)
		{
		case jpeg_marker::COMMENT:
			do
			{
				char *comment = new char[size - 2];
				stream.read(comment, size - 2);

				std::cout << "Found comment: " << comment << std::endl;
				delete[] comment;
			} while(0);
			break;

		case jpeg_marker::QUANTIZATION_TABLE:
			if (size == quantization_table::CELL_AMOUNT + 3)
			{
				const table_list<quantization_table>::count_fast_t max_tables =
						table_list<quantization_table>::MAX_TABLES;

				const uint_fast8_t table_id = stream.get();
				if (table_id >= max_tables)
				{
					std::cout << "Found invalid quantization table id. It is "
							<< static_cast<unsigned int>(table_id) << " but id can be only between 0 and "
							<< static_cast<unsigned int>(max_tables - 1) << std::endl;
				}
				else
				{
					std::cout << "Found quantization table with id "
							<< static_cast<unsigned int>(table_id) << std::endl;
					tables.list[table_id] = new quantization_table(stream);
					//tables.list[table_id]->print(std::cout);
				}
			}
			else
			{
				std::cout << "Found invalid quantization table. Expected size was "
						<< static_cast<unsigned int>(quantization_table::SIDE) << 'x'
						<< static_cast<unsigned int>(quantization_table::SIDE) << std::endl;
			}
			break;

		case jpeg_marker::START_OF_FRAME_BASELINE_DCT:
			do
			{
				current_frame = new frame_info(stream, tables);

				if (current_frame->expected_byte_size() == size)
				{
					std::cout << "Found frame for an image with "
							<< static_cast<unsigned int>(current_frame->channels_amount)
							<< " channels and resolution "
							<< static_cast<unsigned int>(current_frame->width) << 'x'
							<< static_cast<unsigned int>(current_frame->height) << std::endl;
				}
				else
				{
					std::cout << "Found invalid frame. Expected size was "
							<< static_cast<unsigned int>(current_frame->expected_byte_size()) << " but actually was "
							<< static_cast<unsigned int>(size) << std::endl;
				}
			} while(0);
			break;

		case jpeg_marker::HUFFMAN_TABLE:
			do
			{
				const uint_fast8_t table_ref = stream.get();
				const table_list<huffman_table>::index_fast_t table_id = table_ref & 0x0F;
				bool is_ac = (table_ref & 0x10) != 0;

				huffman_table *table = new huffman_table(stream);
				const uint_fast8_t symbol_amount = table->symbol_amount();
				const uint_fast16_t expected_size = table->expected_byte_size();
				if (size == expected_size)
				{
					(is_ac? ac_tables : dc_tables).list[table_id] = table;
					std::cout << "Found huffman table with id " << static_cast<unsigned int>(table_id)
								<< " for " << (is_ac? "ac" : "dc") << " (" << static_cast<unsigned int>(symbol_amount) << " symbols found)" << std::endl;
				}
				else
				{
					std::cout << "Found invalid huffman table. Expected size for it was "
							<< expected_size << " but size is actually " << size << std::endl;
				}
			} while(0);
			break;

		case jpeg_marker::JFIF:
			do
			{
				jfif::info jfif_info(stream);
				stream.ignore(size - 2 - jfif::info::SIZE_IN_FILE);
				if (jfif_info.is_valid())
				{
					const char *density_units = "unknown";
					switch (jfif_info.density_units())
					{
					case jfif::PIXELS_PER_CENTIMETRE:
						density_units = "pixels/cm";
						break;

					case jfif::PIXELS_PER_INCH:
						density_units = "pixels/inch";
						break;

					default:
						;
					}

					std::cout << "Found JFIF v" << static_cast<unsigned int>(jfif_info.major_version())
								<< "." << static_cast<unsigned int>(jfif_info.minor_version())
								<< " section:" << std::endl
								<< " Density units: " << density_units << std::endl
								<< " X Density: " << jfif_info.x_density() << std::endl
								<< " Y Density: " << jfif_info.y_density() << std::endl;
				}
				else
				{
					std::cout << "Found JFIF section but it is not valid" << std::endl;
				}
			} while(0);
			break;

		case jpeg_marker::START_OF_SCAN:
			do
			{
				current_scan = new scan_info(stream, dc_tables, ac_tables);

				if (current_scan->expected_byte_size() == size)
				{
					std::cout << "Found scan for an image with "
							<< static_cast<unsigned int>(current_scan->channels_amount)
							<< " channels" << std::endl;
				}
				else
				{
					std::cout << "Found invalid scan. Expected size was "
							<< static_cast<unsigned int>(current_scan->expected_byte_size()) << " but actually was "
							<< static_cast<unsigned int>(size) << std::endl;
				}
			} while(0);
			break;

		default:
			stream.ignore(size - 2);
			std::cout << "Found section of type " << static_cast<unsigned int>(marker_type) << " and size " << size << std::endl;
		}
	}

	// Preparing bitmap to allocate the image (RGB, 8 bits per channel)
	shared_array<unsigned char> image_raw_data = shared_array<unsigned char>::make(new unsigned char[current_frame->width * current_frame->height * 3]);
	shared_array<bitmap_component> bitmap_components = shared_array<bitmap_component>::make(new bitmap_component[3]);

	bitmap_components[0].type = bitmap_component::RED;
	bitmap_components[0].bits_per_pixel = 8;
	bitmap_components[1].type = bitmap_component::GREEN;
	bitmap_components[1].bits_per_pixel = 8;
	bitmap_components[2].type = bitmap_component::BLUE;
	bitmap_components[2].bits_per_pixel = 8;

	bitmap.bytes_per_pixel = 3;
	bitmap.width = current_frame->width;
	bitmap.height = current_frame->height;
	bitmap.bytes_per_scanline = (((bitmap.bytes_per_pixel * bitmap.width) + 3) >> 2) << 2;
	bitmap.components_amount = 3;
	bitmap.components = bitmap_components;
	bitmap.data = image_raw_data;

	// Scan of data begins here
	scan_bit_stream bit_stream = (&stream);
	bit_stream.prepend(value);

	decode_scan_data(bitmap, bit_stream, *current_frame, *current_scan);

	// Freeing JPEG related resources
	if (current_scan != NULL)
	{
		delete current_scan;
	}

	if (current_frame != NULL)
	{
		delete current_frame;
	}

	for (table_list<quantization_table>::count_fast_t i = 0; i < table_list<quantization_table>::MAX_TABLES; i++)
	{
		if (tables.list[i] != NULL)
		{
			delete tables.list[i];
		}
	}

	for (table_list<huffman_table>::count_fast_t i = 0; i < table_list<huffman_table>::MAX_TABLES; i++)
	{
		if (dc_tables.list[i] != NULL)
		{
			delete dc_tables.list[i];
		}

		if (ac_tables.list[i] != NULL)
		{
			delete ac_tables.list[i];
		}
	}

	if (stream.get() != jpeg_marker::MARKER || stream.get() != jpeg_marker::END_OF_IMAGE)
	{
		throw invalid_file_format();
	}
}
