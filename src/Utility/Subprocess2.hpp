/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS2_HPP
#define CHALET_SUBPROCESS2_HPP

#include "Utility/SubprocessOptions.hpp"

namespace chalet
{
struct Subprocess2
{
	Subprocess2() = default;

	int run(const StringList& inCmd, SubprocessOptions&& inOptions);

private:
	//
};

}

#endif // CHALET_SUBPROCESS2_HPP
