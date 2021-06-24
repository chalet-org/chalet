/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LAST_WRITE_HPP
#define CHALET_LAST_WRITE_HPP

namespace chalet
{
struct LastWrite
{
	std::time_t lastWrite = 0;
	bool needsUpdate = true;
};
}

#endif // CHALET_LAST_WRITE_HPP
