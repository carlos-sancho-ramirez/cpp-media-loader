
#ifndef STREAM_UTILS_HPP_
#define STREAM_UTILS_HPP_

#include "unsupported_feature.hpp"
#include "bounded_integers.hpp"
#include <iostream>
#include <stdexcept>

unsigned int read_big_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument);
unsigned int read_little_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument);

void write_little_endian_unsigned_int(std::ostream &stream, unsigned int value, unsigned int bytes) throw(std::invalid_argument);

class bit_stream
{
protected:
	enum
	{
		BUFFER_BYTES = 1,
		BUFFER_BITS = 8 * BUFFER_BYTES
	};

	typedef bounded_integer<0, (1 << BUFFER_BITS) - 1>::fast last_t;

	std::istream *stream;

	last_t last;
	typename bounded_integer<0, BUFFER_BITS>::fast valid_bits;

	typedef int raw_number_t;

public:
	typedef int number_t;
	typedef typename bounded_integer<0, sizeof(number_t) * 8>::fast number_bit_amount_t;

public:
	bit_stream(std::istream *stream);

	/**
	 * Adds the value at the beginning of the internal buffer. So, this value will be returned again
	 * bit a bit.
	 */
	void prepend(const unsigned char value);

	/**
	 * Returns 0 or 1 depending on the bit value
	 */
	virtual unsigned char next_bit();

private:
	raw_number_t next_raw_number(const number_bit_amount_t bits);

public:
	/**
	 * Extracts the following number from the scan stream that matches the given amount of bits.
	 * The number is processed as the JPEG standard suggests. If the first bit is 1 that will mean
	 * that the number is positive and matches extractly the raw number, but in case of 0, it will
	 * be negative and must be negated to get the expected value.
	 */
	number_t next_number(const number_bit_amount_t bits);
};

/**
 * Reads the stream returning it bit per bit in a suitable way for jpeg huffman tables.
 * This class also remove extra 0x00 bytes after 0xff if any.
 */
class scan_bit_stream : public bit_stream
{
public:
	scan_bit_stream(std::istream *stream) : bit_stream(stream) { }

	/**
	 * Does that same that its parent method but skipping every 0x00 byte after 0xFF.
	 */
	virtual unsigned char next_bit() throw(unsupported_feature);
};

#endif /* STREAM_UTILS_HPP_ */
