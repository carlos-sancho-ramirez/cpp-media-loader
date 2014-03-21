
#ifndef JFIF_HPP_
#define JFIF_HPP_

#include <stdint.h>
#include <iostream>

namespace jfif
{
	enum density_units_e
	{
		NO_UNITS = 0,
		PIXELS_PER_INCH = 1,
		PIXELS_PER_CENTIMETRE = 2
	};

	struct info
	{
		enum info_size_e
		{
			SIZE_IN_FILE = 12
		};

	private:
		unsigned char raw_info[SIZE_IN_FILE];

	public:
		info(std::istream &stream);

		bool is_valid() const;
		uint_fast8_t major_version() const;
		uint_fast8_t minor_version() const;
		density_units_e density_units() const;
		uint_fast16_t x_density() const;
		uint_fast16_t y_density() const;
	};
}

#endif /* JFIF_HPP_ */
