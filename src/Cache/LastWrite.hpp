/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct LastWrite
{
	std::time_t lastWrite = 0;
	bool needsUpdate = true;
};
}
