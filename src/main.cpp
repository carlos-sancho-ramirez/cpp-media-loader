
#include "conf.h"

#include "jfif.hpp"
#include "instream.hpp"
#include "jpeg.hpp"
#include "jpeg_markers.hpp"

#include <iostream>
#include <fstream>

namespace program_result
{
	enum program_result_e
	{
		OK = 0,
		INVALID_ARGUMENTS = 1,
		IO_ERROR = 2,
		INVALID_FILE_FORMAT = 3
	};
}

int main(int argc, char *argv[])
{
	std::cout << "C++ Media Loader" << std::endl
			<< "This software is under the MIT License" << std::endl
			<< "Version " PROJECT_VERSION_STR << std::endl << std::endl;

	if (argc < 2)
	{
		std::cout << "Syntax: " << argv[0] << " <file-name>" << std::endl;
		return program_result::INVALID_ARGUMENTS;
	}

	std::ifstream in_stream(argv[1]);
	if (in_stream.fail())
	{
		std::cout << "Unable to process file " << argv[1] << std::endl;
		return program_result::IO_ERROR;
	}
	else
	{
		std::cout << "Processing file " << argv[1] << std::endl;
	}

	if (in_stream.get() != jpeg_marker::MARKER || in_stream.get() != jpeg_marker::START_OF_IMAGE)
	{
		in_stream.close();
		std::cout << "File " << argv[1] << " is not a valid JPEG file" << std::endl;
		return program_result::INVALID_FILE_FORMAT;
	}

	table_list<quantization_table> tables;
	table_list<huffman_table> dc_tables;
	table_list<huffman_table> ac_tables;

	frame_info *current_frame = NULL;
	scan_info *current_scan = NULL;

	unsigned char value;
	while (in_stream.good() && (value = in_stream.get()) == jpeg_marker::MARKER)
	{
		const uint_fast8_t marker_type = in_stream.get();
		const uint_fast16_t size = read_big_endian_unsigned_int(in_stream, 2);

		switch (marker_type)
		{
		case jpeg_marker::COMMENT:
			do
			{
				char *comment = new char[size - 2];
				in_stream.read(comment, size - 2);

				std::cout << "Found comment: " << comment << std::endl;
				delete[] comment;
			} while(0);
			break;

		case jpeg_marker::QUANTIZATION_TABLE:
			if (size == quantization_table::CELL_AMOUNT + 3)
			{
				const table_list<quantization_table>::count_fast_t max_tables =
						table_list<quantization_table>::MAX_TABLES;

				const uint_fast8_t table_id = in_stream.get();
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
					tables.list[table_id] = new quantization_table(in_stream);
					tables.list[table_id]->print(std::cout);
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
				current_frame = new frame_info(in_stream, tables);

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
				const uint_fast8_t table_ref = in_stream.get();
				const table_list<huffman_table>::index_fast_t table_id = table_ref & 0x0F;
				bool is_ac = (table_ref & 0x10) != 0;

				huffman_table *table = new huffman_table(in_stream);
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
				jfif::info jfif_info(in_stream);
				in_stream.ignore(size - 2 - jfif::info::SIZE_IN_FILE);
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
				current_scan = new scan_info(in_stream, dc_tables, ac_tables);

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
			in_stream.ignore(size - 2);
			std::cout << "Found section of type " << static_cast<unsigned int>(marker_type) << " and size " << size << std::endl;
		}
	}

	// Preparing bitmap to allocate the image (RGB, 8 bits per channel)
	unsigned char *image_raw_data = new unsigned char[current_frame->width * current_frame->height * 3];

	bitmap_component bitmap_components[3];
	bitmap_components[0].type = bitmap_component::RED;
	bitmap_components[0].bits_per_pixel = 8;
	bitmap_components[1].type = bitmap_component::GREEN;
	bitmap_components[1].bits_per_pixel = 8;
	bitmap_components[2].type = bitmap_component::BLUE;
	bitmap_components[2].bits_per_pixel = 8;

	bitmap bitmap;
	bitmap.bytes_per_pixel = 3;
	bitmap.width = current_frame->width;
	bitmap.height = current_frame->height;
	bitmap.bytes_per_scanline = (((bitmap.bytes_per_pixel * bitmap.width) + 3) >> 2) << 2;
	bitmap.components_amount = 3;
	bitmap.components = bitmap_components;
	bitmap.data = image_raw_data;

	// Scan of data begins here
	scan_bit_stream bit_stream = (&in_stream);
	bit_stream.prepend(value);

	jpeg::decode_image(bitmap, bit_stream, *current_frame, *current_scan);

	if (in_stream.get() != jpeg_marker::MARKER || in_stream.get() != jpeg_marker::END_OF_IMAGE)
	{
		in_stream.close();
		std::cout << "File " << argv[1] << " is not a valid JPEG file" << std::endl;
		return program_result::INVALID_FILE_FORMAT;
	}

	// Freeing resources
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

	in_stream.close();
	delete []image_raw_data;
	return program_result::OK;
}
