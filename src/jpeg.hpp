
#ifndef JPEG_HPP_
#define JPEG_HPP_

#include <iostream>

struct quantization_table
{
	enum
	{
		WIDTH = 8,
		HEIGHT = 8
	};

	unsigned char matrix[WIDTH * HEIGHT];

	quantization_table(std::istream &stream);
	void print(std::ostream &stream);
};


#endif /* JPEG_HPP_ */
