
#ifndef JPEG_HPP_
#define JPEG_HPP_

#include <stdint.h>
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

class huffman_table
{
public:
	enum
	{
		MAX_WORD_SIZE = 16
	};

private:
	unsigned char symbol_indexes[MAX_WORD_SIZE];
	const unsigned char *symbols;
	uint_fast8_t _symbol_amount;

public:
	huffman_table(std::istream &stream);
	~huffman_table();
	uint_fast8_t symbol_amount() const;
};

#endif /* JPEG_HPP_ */
