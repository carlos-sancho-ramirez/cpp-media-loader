
#ifndef BMP_HPP_
#define BMP_HPP_

#include <stdint.h>
#include <iostream>

#include "bitmaps.hpp"

struct bmp_header
{
	enum
	{
		HEADER_SIZE = 14
	};

	char signature[2];
	uint32_t file_size;
	// 4 bytes reserved

	/**
	 * Offset to where the data starts within the file
	 */
	uint32_t bitmap_offset;

	bmp_header() : signature({'B','M'}) { }

	bmp_header(std::istream &stream);
	void write_into_stream(std::ostream &stream) const;
};

struct dib_header
{
	enum
	{
		HEADER_SIZE = 40
	};

	uint32_t header_size;
	int32_t width;
	int32_t height;
	uint16_t color_planes;
	uint16_t bits_per_pixel;
	enum : uint32_t
	{
		BI_RGB = 0, // No compression
		BI_RLE8 = 1, // RLE 8 bits/pixel
		BI_RLE4 = 2, // RLE 4 bits/pixel
		BI_BITFIELDS = 3, // Bit fields or huffman 1D compression
		BI_JPEG = 4, // JPEG or RLE-24
		BI_PNG = 5, // PNG
		BI_ALPHABITFIELDS = 6
	} compression_method;
	uint32_t raw_data_size;
	uint32_t h_pixel_per_meter;
	uint32_t v_pixel_per_meter;
	uint32_t colors_in_palette;
	uint32_t important_colors; // Usually ignored

	dib_header() : header_size(HEADER_SIZE), color_planes(1), compression_method(BI_RGB),
			h_pixel_per_meter(2835), // 72ppi
			v_pixel_per_meter(2835), // 72ppi
			colors_in_palette(0), // No palette
			important_colors(0)
	{ }

	void write_into_stream(std::ostream &stream) const;
};

namespace bmp
{
	/**
	 * Thrown when a requested operation is unavailable or it is not supported.
	 */
	class unsupported_operation { };

	void encode_image(bitmap &bitmap, std::ostream &stream);
}

#endif /* BMP_HPP_ */
