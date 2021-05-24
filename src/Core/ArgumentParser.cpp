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
		// LOG('"', arg, '"');
		String::replaceAll(arg, '"', 0);
		String::replaceAll(arg, '\'', 0);

		if (String::contains(' ', arg))
		{
			auto list = String::split(arg);
			for (auto& it : list)
			{
				arguments.push_back(std::move(it));
			}
		}
		else
		{
			if (String::startsWith("--", arg) && String::contains('=', arg))
			{
				auto list = String::split(arg, '=');
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

	for (auto& [key, rawValue] : patterns.arguments())
	{
		auto kind = rawValue.kind();
		switch (kind)
		{
			case Variant::Kind::String: {
				auto value = rawValue.asString();
				if (key == ArgumentPatterns::kArgConfiguration)
				{
					m_inputs.setBuildFromCommandLine(std::move(value));
				}
				else if (key == ArgumentPatterns::kArgRunProject)
				{
					m_inputs.setRunProject(std::move(value));
				}
				else if (String::equals(key, "-i") || String::equals(key, "--input"))
				{
					if (!value.empty())
						CommandLineInputs::setFile(std::move(value));
				}
				else if (String::equals(key, "-g") || String::equals(key, "--generator"))
				{
					m_inputs.setGenerator(std::move(value));
				}
				else if (String::equals(key, "-e") || String::equals(key, "--envfile"))
				{
					if (!value.empty())
						m_inputs.setEnvFile(std::move(value));
				}
				else if (key == ArgumentPatterns::kArgInitName)
				{
					m_inputs.setInitProjectName(std::move(value));
				}
				else if (key == ArgumentPatterns::kArgInitPath)
				{
					m_inputs.setInitPath(std::move(value));
				}
				break;
			}

			case Variant::Kind::StringList: {
				if (key == ArgumentPatterns::kArgRunArguments)
				{
					auto runArgs = String::join(rawValue.asStringList());
					m_inputs.setRunOptions(std::move(runArgs));
				}
				break;
			}

			case Variant::Kind::Boolean: {
				bool value = rawValue.asBool();
				if (String::equals(key, "--save-schema"))
				{
					m_inputs.setSaveSchemaToFile(value);
				}
				break;
			}
			default:
				break;
		}
	}

	// LOG(CommandLineInputs::file(), ' ', inputFile);

	return true;
}

}
