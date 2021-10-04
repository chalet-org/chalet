/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ListPrinter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/StatePrototype.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ListPrinter::ListPrinter(const CommandLineInputs& inInputs, const StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool ListPrinter::printListOfRequestedType()
{
	CommandLineListOption listOption = m_inputs.listOption();
	if (listOption == CommandLineListOption::None)
	{
		Diagnostic::error("Requested list type was invalid");
		return false;
	}

	StringList output;
	if (listOption == CommandLineListOption::Commands)
	{
		output = m_inputs.commandList();
	}
	else if (listOption == CommandLineListOption::Configurations)
	{
		output = m_prototype.getBuildConfigurationList();
	}
	else if (listOption == CommandLineListOption::ToolchainPresets)
	{
		output = m_inputs.getToolchainPresets();
	}
	else if (listOption == CommandLineListOption::UserToolchains)
	{
		output = m_prototype.getUserToolchainList();
	}
	else if (listOption == CommandLineListOption::AllToolchains)
	{
		StringList presets = m_inputs.getToolchainPresets();
		StringList userToolchains = m_prototype.getUserToolchainList();
		output = List::combine(std::move(userToolchains), std::move(presets));
	}
	else if (listOption == CommandLineListOption::Architectures)
	{
		output = getArchitectures();
	}

	std::cout << String::join(output) << std::endl;

	return true;
}

/*****************************************************************************/
StringList ListPrinter::getArchitectures() const
{
	StringList ret{ "auto" };

#if defined(CHALET_MACOS)
	ret.emplace_back("universal");
	ret.emplace_back("x86_64");
	ret.emplace_back("arm64");
#elif defined(CHALET_WIN32)
	ret.emplace_back("x64");
	ret.emplace_back("x64_x86");
	ret.emplace_back("x64_arm");
	ret.emplace_back("x64_arm64");
	ret.emplace_back("x86_x64");
	ret.emplace_back("x86");
	ret.emplace_back("x86_arm");
	ret.emplace_back("x86_arm64");

	ret.emplace_back("i686");
	ret.emplace_back("x86_64");
	ret.emplace_back("arm");
	ret.emplace_back("arm64");
#else
	ret.emplace_back("i686");
	ret.emplace_back("x86_64");
	ret.emplace_back("arm");
	ret.emplace_back("arm64");
#endif

	return ret;
}
}
