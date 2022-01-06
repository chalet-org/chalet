/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TEST_STATE_HPP
#define CHALET_TEST_STATE_HPP

namespace chalet
{
namespace TestState
{
bool setChaletPath(const int argc, const char* const argv[]);
const std::string& chaletExec();
}
}

#endif // CHALET_TEST_STATE_HPP
