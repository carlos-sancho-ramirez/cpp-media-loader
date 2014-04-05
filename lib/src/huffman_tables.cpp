
#include "huffman_tables.hpp"

huffman_table::huffman_table(std::istream &stream)
{
	stream.read(reinterpret_cast<char *>(symbols_per_size), MAX_WORD_SIZE);

	unsigned int all_symbol_amount = 0;
	for (uint_fast8_t index = 0; index < MAX_WORD_SIZE; index++)
	{
		all_symbol_amount += symbols_per_size[index];
	}

	unsigned char *all_symbols = new unsigned char[all_symbol_amount];
	stream.read(reinterpret_cast<char *>(all_symbols), all_symbol_amount);

	symbols = all_symbols;
	_symbol_amount = all_symbol_amount;
}

huffman_table::~huffman_table()
{
	delete[] symbols;
}

huffman_table::symbol_count_t huffman_table::symbol_amount() const
{
	return _symbol_amount;
}

uint_fast16_t huffman_table::expected_byte_size() const
{
	return _symbol_amount + MAX_WORD_SIZE + 3;
}

huffman_table::symbol_entry_t huffman_table::start_entry_for_size(const unsigned int size) const
{
	symbol_entry_t entry = 0;
	for (unsigned int size_index = 1; size_index < size; size_index++)
	{
		const unsigned int symbols = symbols_per_size[size_index - 1];
		entry += symbols * (1 << (MAX_WORD_SIZE - size_index));
	}

	return entry;
}

bool huffman_table::find_symbol(const symbol_entry_t &entry, const unsigned int size, symbol_value_t &symbol) const
{
	const symbol_entry_t start_for_this_size = start_entry_for_size(size);
	const symbol_entry_t start_for_next_size = start_entry_for_size(size + 1);

	if (start_for_this_size > entry || start_for_next_size <= entry)
	{
		return false;
	}

	const symbols_per_size_t relative = (entry - start_for_this_size) >> (MAX_WORD_SIZE - size);
	symbol_index_t index = relative;
	for (unsigned int i = 1; i < size; i++)
	{
		index += symbols_per_size[i - 1];
	}

	symbol = symbols[index];
	return true;
}

huffman_table::symbol_value_t huffman_table::next_symbol(bit_stream &bit_stream) const throw(std::invalid_argument)
{
	unsigned int size = 0;
	symbol_entry_t entry = 0;
	while (++size <= MAX_WORD_SIZE)
	{
		entry |= ((unsigned int) bit_stream.next_bit()) << (MAX_WORD_SIZE - size);
		symbol_value_t symbol;
		if (find_symbol(entry, size, symbol))
		{
			return symbol;
		}
	}

	throw std::invalid_argument("Symbol not found in huffman table");
}



