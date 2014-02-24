
#include "instream.hpp"
#include "jpeg_markers.hpp"

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

scan_bit_stream::scan_bit_stream(std::istream *stream) : stream(stream), valid_bits(0)
{ }

void scan_bit_stream::prepend(const unsigned char value)
{
	valid_bits += 8;
	last = (last >> 8) + value;
}

unsigned char scan_bit_stream::next_bit() throw(unsupported_feature)
{
	if (valid_bits == 0)
	{
		last = stream->get();
		valid_bits = 8;

		if (last == jpeg_marker::MARKER)
		{
			unsigned char marker_type = stream->get();
			if (marker_type != 0)
			{
				// TODO: Supporting marker within the the scan data should be allowed
				throw unsupported_feature();
			}
		}
	}

	return (last >> --valid_bits) & 1;
}

scan_bit_stream::number_t scan_bit_stream::next_number(const number_bit_amount_t bits)
{
	number_t result = 0;
	for (number_bit_amount_t bit = 0; bit < bits; bit++)
	{
		result = (result << 1) + next_bit();
	}

	return result;
}
