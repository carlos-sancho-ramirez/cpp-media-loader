
#include "conf.h"
#include <iostream>
#include <fstream>

#include <stdint.h>
#include <arpa/inet.h>

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
		START_OF_IMAGE = 0xD8,
		END_OF_IMAGE = 0xD9,

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
		int marker_type = in_stream.get();
		uint16_t network_size;
		in_stream >> network_size;
		uint_fast16_t size = ntohs(size);

		in_stream.ignore(size - 2);

		std::cout << "Found section of type " << marker_type << " and size " << size << std::endl;
	}

	in_stream.close();
	return program_result::OK;
}
