/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_LINE_LIST_OPTION_HPP
#define CHALET_COMMAND_LINE_LIST_OPTION_HPP

namespace chalet
{
enum class CommandLineListOption : ushort
{
	None,
	Commands,
	Configurations,
	AllToolchains,
	UserToolchains,
	ToolchainPresets,
	Architectures
};
}

#endif // CHALET_COMMAND_LINE_LIST_OPTION_HPP
