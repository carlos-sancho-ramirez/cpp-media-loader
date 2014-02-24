
#ifndef INSTREAM_HPP_
#define INSTREAM_HPP_

#include "unsupported_feature.hpp"
#include "bounded_integers.hpp"
#include <iostream>
#include <stdexcept>

unsigned int read_big_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument);

/**
 * Reads the stream returning it bit per bit in a suitable way for jpeg huffman tables.
 * This class also remove extra 0x00 bytes after 0xff if any.
 */
class scan_bit_stream
{
	enum
	{
		BUFFER_BYTES = 1,
		BUFFER_BITS = 8 * BUFFER_BYTES
	};

	typedef bounded_integer<0, (1 << BUFFER_BITS) - 1>::fast last_t;

	std::istream *stream;

	last_t last;
	typename bounded_integer<0, BUFFER_BITS>::fast valid_bits;

public:
	typedef unsigned int number_t;
	typedef typename bounded_integer<0, sizeof(number_t) * 8>::fast number_bit_amount_t;

public:
	scan_bit_stream(std::istream *stream);
	void prepend(const unsigned char value);
	unsigned char next_bit() throw(unsupported_feature);
	number_t next_number(const number_bit_amount_t bits);
};

#endif /* INSTREAM_HPP_ */
