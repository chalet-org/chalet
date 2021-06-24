/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CONFIG_ACTION_HPP
#define CHALET_CONFIG_ACTION_HPP

namespace chalet
{
enum class ConfigAction : ushort
{
	Get,
	Set,
	Unset,
};
}

#endif // CHALET_CONFIG_ACTION_HPP
