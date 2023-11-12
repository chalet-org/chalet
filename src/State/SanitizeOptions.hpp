/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace SanitizeOptions
{
enum
{
	None = 0,
	Address = 1 << 0,
	HardwareAddress = 1 << 1,
	Thread = 1 << 2,
	Memory = 1 << 3,
	Leak = 1 << 4,
	UndefinedBehavior = 1 << 5,
};
using Type = uchar;
}
}
