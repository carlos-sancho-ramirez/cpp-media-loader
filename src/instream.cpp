
#include "instream.hpp"

#include <cstring>
#include <arpa/inet.h>

unsigned int read_big_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument)
{
	if (bytes > sizeof(unsigned int) || bytes <= 0)
	{
		throw std::invalid_argument("[read_big_endian_unsigned_int] wrong amount of bytes entered");
	}

	char buffer[sizeof(unsigned int)];
	memset(buffer, 0, sizeof(buffer));
	stream.read(buffer + (sizeof(buffer) - bytes), bytes);
	return ntohl(*reinterpret_cast<unsigned int *>(buffer));
}
