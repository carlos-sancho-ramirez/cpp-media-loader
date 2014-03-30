
#include "conf.h"

#include "jpeg.hpp"
#include "bmp.hpp"

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

	if (argc < 3)
	{
		std::cout << "Syntax: " << argv[0] << " <origin-file-name> <destination-file-name>" << std::endl;
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

	bitmap bitmap;
	try
	{
		jpeg::decode_image(bitmap, in_stream);
	}
	catch (jpeg::invalid_file_format)
	{
		in_stream.close();
		std::cerr << "File " << argv[1] << " is not a valid JPEG file" << std::endl;
		return program_result::INVALID_FILE_FORMAT;
	}

	in_stream.close();

	// Write the result in BMP format
	std::ofstream out_stream(argv[2]);
	int result = program_result::OK;
	if (out_stream.fail())
	{
		std::cout << "Unable to create file " << argv[2] << std::endl;
		result = program_result::IO_ERROR;
	}
	else
	{
		std::cout << "Writing file " << argv[2] << std::endl;
		bmp::encode_image(bitmap, out_stream);
		out_stream.close();
	}

	return result;
}
