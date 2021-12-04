/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SANITIZE_OPTIONS_HPP
#define CHALET_SANITIZE_OPTIONS_HPP

namespace chalet
{
namespace SanitizeOptions
{
enum
{
	None = 0,
	Address = 1 << 0,
	Thread = 1 << 1,
	Memory = 1 << 2,
	Leak = 1 << 3,
	Undefined = 1 << 4,
};
using Type = uchar;
}
}

#endif // CHALET_SANITIZE_OPTIONS_HPP
