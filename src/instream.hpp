
#ifndef INSTREAM_HPP_
#define INSTREAM_HPP_

#include <iostream>
#include <stdexcept>

unsigned int read_big_endian_unsigned_int(std::istream &stream, unsigned int bytes) throw(std::invalid_argument);

#endif /* INSTREAM_HPP_ */
