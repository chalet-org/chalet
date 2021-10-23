/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PLATFORM_HPP
#define CHALET_PLATFORM_HPP

namespace chalet
{
namespace Platform
{
const std::string& platform() noexcept;
const StringList& notPlatforms() noexcept;
};
}

#endif // CHALET_PLATFORM_HPP
