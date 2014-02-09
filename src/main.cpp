
#include "conf.h"

#include "jfif.hpp"
#include "instream.hpp"
#include "jpeg.hpp"

#include <iostream>
#include <fstream>

#include <stdint.h>

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

	if (in_stream.get() != jpeg_marker::MARKER || in_stream.get() != jpeg_marker::START_OF_IMAGE)
	{
		in_stream.close();
		std::cout << "File " << argv[1] << " is not a valid JPEG file" << std::endl;
		return program_result::INVALID_FILE_FORMAT;
	}

	while (in_stream.good() && in_stream.get() == jpeg_marker::MARKER)
	{
		const uint_fast8_t marker_type = in_stream.get();
		const uint_fast16_t size = read_big_endian_unsigned_int(in_stream, 2);

		switch (marker_type)
		{
		case jpeg_marker::QUANTIZATION_TABLE:
			if (size == quantization_table::WIDTH * quantization_table::HEIGHT + 3)
			{
				const uint_fast8_t table_id = in_stream.get();
				std::cout << "Found quatization table with id "
						<< static_cast<unsigned int>(table_id) << std::endl;
				quantization_table(in_stream).print(std::cout);
			}
			else
			{
				std::cout << "Found invalid quatization table. Expected size was "
						<< static_cast<unsigned int>(quantization_table::WIDTH) << 'x'
						<< static_cast<unsigned int>(quantization_table::HEIGHT) << std::endl;
			}
			break;

		case jpeg_marker::HUFFMAN_TABLE:
			do
			{
				const uint_fast8_t table_id = in_stream.get();
				const uint_fast8_t symbol_amount = huffman_table(in_stream).symbol_amount();
				const uint_fast16_t expected_size = symbol_amount + huffman_table::MAX_WORD_SIZE + 3;
				if (size == expected_size)
				{
					std::cout << "Found huffman table with id " << static_cast<unsigned int>(table_id)
								<< " (" << static_cast<unsigned int>(symbol_amount) << " symbols found)" << std::endl;
				}
				else
				{
					std::cout << "Found invalid huffman table. Expected size for it was "
							<< (expected_size - 3) << " but size is actually " << (size - 3) << std::endl;
				}
			} while(0);
			break;

		case jpeg_marker::JFIF:
			do {
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

		default:
			in_stream.ignore(size - 2);
			std::cout << "Found section of type " << static_cast<unsigned int>(marker_type) << " and size " << size << std::endl;
		}
	}

	in_stream.close();
	return program_result::OK;
}
