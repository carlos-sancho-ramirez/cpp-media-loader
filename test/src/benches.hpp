/*
 * benches.hpp
 *
 *  Created on: 27/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#ifndef BENCHES_HPP_
#define BENCHES_HPP_

#include "test_common.hpp"

#define TEST_BENCH_DECLARATION(NS) \
	namespace NS \
	{ \
		class test_bench : public ::test_bench \
		{ \
		public: \
			virtual const test_bench_results run() throw(); \
		}; \
	}

TEST_BENCH_DECLARATION(jpeg)
TEST_BENCH_DECLARATION(huffman_tables)

#endif /* BENCHES_HPP_ */
