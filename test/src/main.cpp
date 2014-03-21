/*
 * main.cpp
 *
 *  Created on: 21/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#include <iostream>
#include <string>

namespace
{
bool test(const std::string title, void (*function)())
{
	try
	{
		function();
		std::cout << title << "... ok" << std::endl;
		return true;
	}
	catch (...)
	{
		std::cout << title << "... ko" << std::endl;
	}

	return false;
}

}

void testOK()
{
	std::cout << "textOK executed" << std::endl;
}

void testKO()
{
	std::cout << "textKO executed" << std::endl;
	throw 0;
}

int main(int argc, char *argv[])
{
	test("test 1", testOK);
	test("test 2", testKO);
}
