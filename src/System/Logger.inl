/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/Logger.hpp"

namespace chalet
{
namespace priv
{
/*****************************************************************************/
// This voodoo allows you to write: LOG("first", second, third)
//
template <typename... Args>
Logger::Logger(const char* const inFile, const std::string& inFunction, Args&&... args) :
	Logger(inFile, inFunction)
{
	stream.precision(std::numeric_limits<f64>::digits);
	((stream << std::forward<Args>(args) << ' '), ...);
}
}
}