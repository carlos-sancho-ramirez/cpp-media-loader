
#ifndef BLOCK_MATRIX_HPP_
#define BLOCK_MATRIX_HPP_

#include "bounded_integers.hpp"

struct block_matrix
{
	enum
	{
		SIDE = 8,
		CELLS = SIDE * SIDE
	};

	typedef typename bounded_integer<0, SIDE - 1>::fast side_index_fast_t;
	typedef typename bounded_integer<0, SIDE>::fast side_count_fast_t;

	typedef typename bounded_integer<0, CELLS - 1>::fast cell_index_fast_t;
	typedef typename bounded_integer<0, CELLS>::fast cell_count_fast_t;

	typedef double element_t;

private:
	element_t matrix[CELLS];

public:
	block_matrix();

	static const cell_index_fast_t zigzag_to_real[];

	void set_at_zigzag(const cell_index_fast_t index, float value);

	cell_index_fast_t get_index(const side_index_fast_t x, const side_index_fast_t y) const;
	element_t get(const side_index_fast_t x, const side_index_fast_t y) const;
	void set(const side_index_fast_t x, const side_index_fast_t y, const element_t value);

	block_matrix operator*(const element_t value) const;
	block_matrix &operator=(const block_matrix &other);
	block_matrix &operator+=(const element_t &value);
	block_matrix &operator-=(const element_t &value);
	block_matrix operator+(const block_matrix &other) const;
	block_matrix operator-(const block_matrix &other) const;
};

#endif /* BLOCK_MATRIX_HPP_ */
