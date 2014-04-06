
#ifndef HUFFMAN_TABLES_HPP_
#define HUFFMAN_TABLES_HPP_

#include "bounded_integers.hpp"
#include "stream_utils.hpp"
#include <iostream>

class huffman_table
{
public:
	enum
	{
		MAX_WORD_SIZE = 16
	};
	typedef bounded_integer<0, (1 << MAX_WORD_SIZE) - 1>::fast symbol_entry_t;
	typedef bounded_integer<0, 1 << MAX_WORD_SIZE>::fast symbol_entry_limit_t;
	typedef bounded_integer<0, 255>::fast symbols_per_size_t;
	typedef unsigned char symbol_value_t;
	typedef uint_fast16_t symbol_index_t;
	typedef uint_fast16_t symbol_count_t;

private:
	symbols_per_size_t symbols_per_size[MAX_WORD_SIZE];
	const symbol_value_t *symbols;
	symbol_count_t _symbol_amount;

	symbol_entry_limit_t start_entry_for_size(const unsigned int size) const;
	bool find_symbol(const symbol_entry_t &entry, const unsigned int size, symbol_value_t &symbol) const;

public:
	huffman_table(std::istream &stream);
	~huffman_table();
	symbol_count_t symbol_amount() const;
	uint_fast16_t expected_byte_size() const;

	/**
	 * Checks the stream, determines the symbol and retuns it.
	 * std::invalid_argument will be thrown if the huffman code is not present in the table.
	 */
	symbol_value_t next_symbol(bit_stream &bit_stream) const throw(std::invalid_argument);
};

#endif /* HUFFMAN_TABLES_HPP_ */
