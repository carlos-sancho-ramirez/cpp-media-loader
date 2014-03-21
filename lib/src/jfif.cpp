
#include "jfif.hpp"
#include <cstring>
#include <arpa/inet.h>

jfif::info::info(std::istream &stream)
{
	const long size = sizeof(raw_info);
	char *char_info = reinterpret_cast<char *>(raw_info);
	stream.read(char_info, size);
}

bool jfif::info::is_valid() const
{
	std::cout << "Chain to contrast: " << reinterpret_cast<const char *>(raw_info) << std::endl;
	return strcmp(reinterpret_cast<const char *>(raw_info), "JFIF") == 0;
}

uint_fast8_t jfif::info::major_version() const
{
	return raw_info[5];
}

uint_fast8_t jfif::info::minor_version() const
{
	return raw_info[6];
}

jfif::density_units_e jfif::info::density_units() const
{
	return static_cast<jfif::density_units_e>(raw_info[7]);
}

uint_fast16_t jfif::info::x_density() const
{
	return ntohs(*reinterpret_cast<const uint16_t *>(raw_info + 8));
}

uint_fast16_t jfif::info::y_density() const
{
	return ntohs(*reinterpret_cast<const uint16_t *>(raw_info + 10));
}
