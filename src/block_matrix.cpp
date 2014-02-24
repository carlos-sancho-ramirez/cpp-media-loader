
#include "block_matrix.hpp"

block_matrix::block_matrix()
{
	cell_count_fast_t index;
	for (index = 0; index < CELLS; index++)
	{
		matrix[index] = 0.0f;
	}
}

const block_matrix::cell_index_fast_t block_matrix::zigzag_to_real[] =
{
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,	53, 60, 61, 54, 47, 55, 62, 63
};

void block_matrix::set_at_zigzag(const cell_index_fast_t index, float value)
{
	const cell_index_fast_t position = zigzag_to_real[index];
	matrix[position] = value;
}

block_matrix::cell_index_fast_t block_matrix::get_index(const side_index_fast_t x, const side_index_fast_t y) const
{
	return y * SIDE + x;
}

block_matrix::element_t block_matrix::get(const side_index_fast_t x, const side_index_fast_t y) const
{
	return matrix[get_index(x,y)];
}

void block_matrix::set(const side_index_fast_t x, const side_index_fast_t y, const element_t value)
{
	matrix[get_index(x,y)] = value;
}

block_matrix block_matrix::operator*(const element_t value) const
{
	block_matrix result;

	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		result.matrix[index] = matrix[index] * value;
	}

	return result;
}

block_matrix &block_matrix::operator=(const block_matrix &other)
{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] = other.matrix[index];
	}

	return *this;
}

block_matrix &block_matrix::operator+=(const element_t &value)
			{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] += value;
	}

	return *this;
			}

block_matrix &block_matrix::operator-=(const element_t &value)
			{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] -= value;
	}

	return *this;
			}

block_matrix block_matrix::operator+(const block_matrix &other) const
{
	block_matrix result;

	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		result.matrix[index] = matrix[index] + other.matrix[index];
	}

	return result;
}

block_matrix block_matrix::operator-(const block_matrix &other) const
{
	block_matrix result;

	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		result.matrix[index] = matrix[index] - other.matrix[index];
	}

	return result;
}
