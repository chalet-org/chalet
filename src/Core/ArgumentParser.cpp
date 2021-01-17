/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentParser.hpp"

#include "Builder/BuildManager.hpp"
#include "Core/ArgumentPatterns.hpp"
// #include "Core/ArgumentMap.hpp"
#include "Libraries/Format.hpp"
#include "Router/Route.hpp"
#include "State/CommandLineInputs.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ArgumentParser::ArgumentParser(CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool ArgumentParser::run(const int argc, const char* const argv[])
{
	if (argc < 1)
		return false;

	StringList arguments;
	for (int i = 0; i < argc; ++i)
	{
		std::string arg(argv[i] ? argv[i] : "");
		// LOG("\"", arg, "\"");

		if (String::contains(" ", arg))
		{
			auto list = String::split(arg, " ");
			for (auto& it : list)
			{
				arguments.push_back(std::move(it));
			}
		}
		else
		{
			arguments.push_back(std::move(arg));
		}
	}

	ArgumentPatterns patterns;
	bool result = patterns.parse(arguments);
	if (!result)
		return false;

	if (patterns.arguments().size() == 0)
		return false;

	m_inputs.setAppPath(arguments.front());

	Route command = patterns.route();
	m_inputs.setCommand(command);

	for (auto& [key, value] : patterns.arguments())
	{
		auto kind = value.kind();
		switch (kind)
		{
			case Variant::Kind::String: {
				if (key == ArgumentPatterns::kArgConfiguration)
				{
					auto configuration = value.asString();
					m_inputs.setBuildFromCommandLine(std::move(configuration));
				}
				else if (key == ArgumentPatterns::kArgRunProject)
				{
					auto runProject = value.asString();
					m_inputs.setRunProject(std::move(runProject));
				}
				else if (String::equals(key, "-i") || String::equals(key, "--input"))
				{
					auto inputFile = value.asString();
					if (!inputFile.empty())
						CommandLineInputs::setFile(std::move(inputFile));
				}
				else if (key == ArgumentPatterns::kArgInitName)
				{
					auto name = value.asString();
					m_inputs.setInitProjectName(std::move(name));
				}
				else if (key == ArgumentPatterns::kArgInitPath)
				{
					auto path = value.asString();
					m_inputs.setInitPath(std::move(path));
				}
				break;
			}
			case Variant::Kind::StringList: {
				if (key == ArgumentPatterns::kArgRunArguments)
				{
					auto runArgs = String::join(value.asStringList());
					m_inputs.setRunOptions(std::move(runArgs));
				}
				break;
			}

			case Variant::Kind::Boolean: // might do something with this in the future
			default:
				break;
		}
	}

	// LOG(CommandLineInputs::file(), " ", inputFile);

	return true;
}

}
