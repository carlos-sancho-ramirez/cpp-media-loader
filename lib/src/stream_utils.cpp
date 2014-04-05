
#include "stream_utils.hpp"
#include "jpeg_markers.hpp"

#include <cstring>

unsigned int read_big_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument)
{
	if (bytes > sizeof(unsigned int) || bytes <= 0)
	{
		throw std::invalid_argument("[read_big_endian_unsigned_int] wrong amount of bytes entered");
	}

	char buffer[sizeof(unsigned int)];
	memset(buffer, 0, sizeof(buffer));
	stream.read(buffer + (sizeof(buffer) - bytes), bytes);

	unsigned int result = 0;
	for (unsigned int index = 0; index < sizeof(buffer); index++)
	{
		int char_value = buffer[index];
		result += ((char_value < 0)? char_value + 0x100 : char_value) << ((sizeof(buffer) - index - 1) * 8);
	}

	return result;
}

unsigned int read_little_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument)
{
	if (bytes > sizeof(unsigned int) || bytes <= 0)
	{
		throw std::invalid_argument("[read_big_endian_unsigned_int] wrong amount of bytes entered");
	}

	char buffer[sizeof(unsigned int)];
	memset(buffer, 0, sizeof(buffer));
	stream.read(buffer + (sizeof(buffer) - bytes), bytes);

	unsigned int result = 0;
	for (unsigned int index = 0; index < bytes; index++)
	{
		int char_value = buffer[index];
		result += ((char_value < 0)? char_value + 0x100 : char_value) << (index * 8);
	}

	return result;
}

void write_little_endian_unsigned_int(std::ostream &stream, unsigned int value, unsigned int bytes) throw(std::invalid_argument)
{
	if (bytes > sizeof(unsigned int) || bytes <= 0)
	{
		throw std::invalid_argument("[read_big_endian_unsigned_int] wrong amount of bytes entered");
	}

	char buffer[bytes];

	for (unsigned int index = 0; index < bytes; index++)
	{
		buffer[index] = value & 0xFF;
		value >>= 8;
	}

	stream.write(buffer, bytes);
}

bit_stream::bit_stream(std::istream *stream) : stream(stream), valid_bits(0)
{ }

void bit_stream::prepend(const unsigned char value)
{
	valid_bits += 8;
	last = (last >> 8) + value;
}

unsigned char bit_stream::next_bit()
{
	if (valid_bits == 0)
	{
		last = stream->get();
		valid_bits = 8;
	}

	return (last >> --valid_bits) & 1;
}

bit_stream::raw_number_t bit_stream::next_raw_number(const number_bit_amount_t bits)
{
	raw_number_t result = 0;
	for (number_bit_amount_t bit = 0; bit < bits; bit++)
	{
		result = (result << 1) + next_bit();
	}

	return result;
}

bit_stream::number_t bit_stream::next_number(const number_bit_amount_t bits)
{
	raw_number_t raw = next_raw_number(bits);

	const bool is_negative = (raw & (1 << (bits - 1))) == 0;
	number_t result;

	if (is_negative)
	{
		result = -(raw ^ ((1 << bits) - 1));
	}
	else
	{
		result = raw;
	}

	return result;
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
