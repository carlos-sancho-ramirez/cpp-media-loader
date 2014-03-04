
#include "bitmaps.hpp"

void bitmap::getRawPixel(int x, int y, unsigned char * const pixel) const
{
	if (x >= 0 && static_cast<unsigned int>(x) < width && y >= 0 && static_cast<unsigned int>(y) < height)
	{
		unsigned int position = y * bytes_per_scanline + x * bytes_per_pixel;
		unsigned char *array = static_cast<unsigned char *>(data);

		for (unsigned int index = 0; index < bytes_per_pixel; index++)
		{
			pixel[index] = array[position++];
		}
	}
	else
	{
		for (unsigned int index = 0; index < bytes_per_pixel; index++)
		{
			pixel[index] = 0;
		}
	}
}

void bitmap::setRawPixel(int x, int y, const unsigned char *pixel) const
{
	if (x >= 0 && static_cast<unsigned int>(x) < width && y >= 0 && static_cast<unsigned int>(y) < height)
	{
		unsigned int position = y * bytes_per_scanline + x * bytes_per_pixel;
		unsigned char *array = static_cast<unsigned char *>(data);

		for (unsigned int index = 0; index < bytes_per_pixel; index++)
		{
			array[position++] = pixel[index];
		}
	}
}

void bitmap::getPixel(int x, int y, component_value_t *values) const
{
	unsigned char *pixel_buffer = new unsigned char[bytes_per_pixel];
	getRawPixel(x, y, pixel_buffer);

	unsigned int uint_value;
	unsigned char used_bits = 0;
	unsigned char used_bytes = 0;

	for (unsigned int index = 0; index < components_amount; index++)
	{
		uint_value = 0;
		const unsigned char bits = components[index].bits_per_pixel;

		unsigned char remaining_bits = bits;
		while (remaining_bits > 0)
		{
			if (used_bits > 0)
			{
				const unsigned char final_bits = used_bits + remaining_bits;
				if (final_bits < 8)
				{
					unsigned int value = pixel_buffer[used_bytes];
					value >>= used_bits;
					value &= (1 << remaining_bits) - 1;
					uint_value = value;

					used_bits = final_bits;
					remaining_bits = 0;
				}
				else
				{
					const unsigned int added_bits = 8 - used_bits;
					unsigned int value = pixel_buffer[used_bytes++];
					value >>= used_bits;
					value &= (1 << added_bits) - 1;
					uint_value = value;

					used_bits = 0;
					remaining_bits -= added_bits;
				}
			}
			else {
				if (remaining_bits < 8)
				{
					unsigned int value = pixel_buffer[used_bytes];
					value &= (1 << remaining_bits) - 1;
					value <<= bits - remaining_bits;
					uint_value += value;

					used_bits = remaining_bits;
					remaining_bits = 0;
				}
				else
				{
					unsigned int value = pixel_buffer[used_bytes++];
					value <<= bits - remaining_bits;
					uint_value += value;

					remaining_bits -= 8;
				}
			}
		}

		component_value_t float_value = uint_value;
		values[index] = float_value / ((1 << bits) - 1);
	}

	delete []pixel_buffer;
}

void bitmap::setPixel(int x, int y, const component_value_t *values) const
{
	unsigned char *pixel_buffer = new unsigned char[bytes_per_pixel];
	unsigned int uint_value;
	unsigned char used_bits = 0;
	unsigned char used_bytes = 0;

	for (unsigned int index = 0; index < bytes_per_pixel; index++)
	{
		pixel_buffer[index] = 0;
	}

	for (unsigned int index = 0; index < components_amount; index++)
	{
		const component_value_t float_value = values[index];
		const unsigned char bits = components[index].bits_per_pixel;
		const unsigned int max_value = (1 << bits) - 1;

		if (float_value > 0 && float_value < 1)
		{
			uint_value = float_value * max_value;
		}
		else if (float_value < 0.5)
		{
			uint_value = 0;
		}
		else
		{
			uint_value = max_value;
		}

		unsigned char remaining_bits = bits;
		while (remaining_bits > 0)
		{
			if (used_bits > 0)
			{
				const unsigned char final_bits = used_bits + remaining_bits;
				if (final_bits < 8)
				{
					unsigned char value = pixel_buffer[used_bytes];
					unsigned char mask = (1 << final_bits) - (1 << used_bits);
					value &= 0xFF ^ mask;
					value |= (uint_value << used_bits) & mask;
					pixel_buffer[used_bytes] = value;

					used_bits = final_bits;
					remaining_bits = 0;
				}
				else
				{
					unsigned char value = pixel_buffer[used_bytes];
					value &= 0xFF ^ ((1 << used_bits) - 1);
					value |= (uint_value << used_bits) & 0xFF;
					pixel_buffer[used_bytes++] = value;

					const unsigned char added_bits = 8 - used_bits;

					uint_value >>= added_bits;
					used_bits = 0;
					remaining_bits -= added_bits;
				}
			}
			else {
				if (remaining_bits < 8)
				{
					pixel_buffer[used_bytes] = uint_value & ((1 << remaining_bits) - 1);
					used_bits = remaining_bits;
					remaining_bits = 0;
				}
				else
				{
					pixel_buffer[used_bytes++] = uint_value & 0xFF;
					uint_value >>= 8;
					remaining_bits -= 8;
				}
			}
		}
	}

	setRawPixel(x, y, pixel_buffer);

	delete []pixel_buffer;
}
