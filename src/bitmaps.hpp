
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
	unsigned char *data;

	void getRawPixel(int x, int y, unsigned char * const pixel) const;

	/**
	 * Fills the value array given as a parameter with normalized values for all components in the
	 * specified pixel. All components will be 0 if the coordinates goes outside the bitmap.
	 */
	void getPixel(int x, int y, component_value_t * const value) const;

	void setRawPixel(int x, int y, const unsigned char * const pixel) const;

	/**
	 * Sets a pixel within the bitmap. If x or y goes outside the bitmap nothing will happen.
	 *
	 * values array with at least as elements as components declared in this bitmap.
	 * Each component is expected to be between 0 and 1. In case of lower it will be assumed as 0.
	 * In case of greater than 1, it will assumed to be 1.
	 */
	void setPixel(int x, int y, const component_value_t * const values) const;
};

#endif /* BITMAPS_HPP_ */
