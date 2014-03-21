
#include "bmp.hpp"
#include "stream_utils.hpp"

bmp_header::bmp_header(std::istream &stream)
{
	stream.read(signature, 2);
	file_size = read_little_endian_unsigned_int(stream, 4);
	stream.ignore(4);
	bitmap_offset = read_little_endian_unsigned_int(stream, 4);
}

void bmp_header::write_into_stream(std::ostream &stream) const
{
	stream.write(signature, 2);
	write_little_endian_unsigned_int(stream, file_size, 4);
	write_little_endian_unsigned_int(stream, 0, 4);
	write_little_endian_unsigned_int(stream, bitmap_offset, 4);
}

void dib_header::write_into_stream(std::ostream &stream) const
{
	write_little_endian_unsigned_int(stream, header_size, sizeof(header_size));
	write_little_endian_unsigned_int(stream, width, sizeof(width));
	write_little_endian_unsigned_int(stream, height, sizeof(height));
	write_little_endian_unsigned_int(stream, color_planes, sizeof(color_planes));
	write_little_endian_unsigned_int(stream, bits_per_pixel, sizeof(bits_per_pixel));
	write_little_endian_unsigned_int(stream, compression_method, sizeof(compression_method));
	write_little_endian_unsigned_int(stream, raw_data_size, sizeof(raw_data_size));
	write_little_endian_unsigned_int(stream, h_pixel_per_meter, sizeof(h_pixel_per_meter));
	write_little_endian_unsigned_int(stream, v_pixel_per_meter, sizeof(v_pixel_per_meter));
	write_little_endian_unsigned_int(stream, colors_in_palette, sizeof(colors_in_palette));
	write_little_endian_unsigned_int(stream, important_colors, sizeof(important_colors));
}

void bmp::encode_image(bitmap &bitmap, std::ostream &stream)
{
	const unsigned int component_amount = bitmap.components_amount;

	// Currently only RGB is supported and it is assumed in the bitmap data
	const uint_fast32_t bytes_per_line = bitmap.bytes_per_pixel * bitmap.width;
	const uint_fast32_t raw_data_size = bytes_per_line * bitmap.height;

	dib_header dib_header;
	dib_header.bits_per_pixel = 8 * component_amount; // TODO: This is not true for gray-scaled and other configurations
	dib_header.width = bitmap.width;
	dib_header.height = bitmap.height;
	dib_header.raw_data_size = raw_data_size;

	bmp_header bmp_header;
	bmp_header.bitmap_offset = dib_header.header_size + bmp_header::HEADER_SIZE;
	bmp_header.file_size = bmp_header.bitmap_offset + raw_data_size;

	bmp_header.write_into_stream(stream);
	dib_header.write_into_stream(stream);

	bitmap::component_value_t components[bitmap.components_amount];

	for (int_fast32_t row = dib_header.height - 1; row >= 0; row--)
	{
		for (int_fast32_t column = 0; column < dib_header.width; column++)
		{
			bitmap.getPixel(column, row, components);

			for (int_fast32_t component_index = static_cast<int_fast32_t>(component_amount - 1); component_index >= 0; component_index--)
			{
				const bitmap::component_value_t value = components[component_index];
				unsigned int int_value = (value > 0 && value < 1)? value * 0xFF : ((value < 0.5)? 0 : 0xFF);
				write_little_endian_unsigned_int(stream, int_value, 1);
			}
		}
	}
}
