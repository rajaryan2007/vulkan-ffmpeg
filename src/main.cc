#include <iostream>
#include "Application.hh"
#include "demo.hh"



int main() {

	Demo demo1;;

	try
	{
		demo1.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
  
	return EXIT_SUCCESS;

}



