
#ifndef BITMAPS_HPP_
#define BITMAPS_HPP_

struct bitmap_component
{
	enum
	{
		ALPHA, // 0=opaque, (1 << bits_per_pixel)-1=transparent
		RED,
		GREEN,
		BLUE,
		LUMINANCE, //Y from YUV or YCbCr
		CHROMINANCE_BLUE,
		CHROMINANCE_RED,
	} type;

	unsigned int bits_per_pixel;
};

struct bitmap
{
	/**
	 * Normalized value expected to be between 0 and 1 to indicate a value within a component
	 * independently of the amount of bits.
	 */
	typedef float component_value_t;

	/**
	 * Width in pixels for this bitmap
	 */
	unsigned int width;

	/**
	 * Height in pixels for this bitmap
	 */
	unsigned int height;

	/**
	 * Number of bytes for each scanline. This will be used to work out the position for a given row.
	 * This must be at least width * bytes_per_pixel
	 */
	unsigned int bytes_per_scanline;

	/**
	 * This must be at least enough to allocate the sum of the bits_per_pixel of each component.
	 * But it can be higher in case of required padding.
	 */
	unsigned int bytes_per_pixel;

	/**
	 * Number of component this bitmap has. This is usually 3 for RGB or YCbCr, 4 if alpha channel
	 * is also present, or even 1 in case of a mask or a gray scaled picture.
	 */
	unsigned int components_amount;

	/**
	 * Pointer to each component info. The order of this components within the array determines the
	 * order of the components in the memory layout.
	 */
	bitmap_component *components;
	void *data;

	void setRawPixel(int x, int y, const unsigned char *pixel) const
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

	/**
	 * Sets a pixel within the bitmap. If x or y goes outside the bitmap nothing will happen.
	 *
	 * values array with at least as elements as components declared in this bitmap.
	 * Each component is expected to be between 0 and 1. In case of lower it will be assumed as 0.
	 * In case of greater than 1, it will assumed to be 1.
	 */
	void setPixel(int x, int y, const component_value_t *values) const
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
};

#endif /* BITMAPS_HPP_ */
