/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Application.hpp"

int main(const int argc, const char* argv[])
{
	chalet::Application app;
	return app.run(argc, argv);
}
