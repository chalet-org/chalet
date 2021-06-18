/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentParser.hpp"

#include "Core/ArgumentPatterns.hpp"
// #include "Core/ArgumentMap.hpp"
#include "Core/CommandLineInputs.hpp"

#include "Router/Route.hpp"
#include "Terminal/Output.hpp"
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
				if (key == patterns.argConfiguration())
				{
					m_inputs.setBuildFromCommandLine(std::move(value));
				}
				else if (key == patterns.argRunProject())
				{
					m_inputs.setRunProject(std::move(value));
				}
				else if (String::equals({ "-i", "--input-file" }, key))
				{
					m_inputs.setBuildFile(std::move(value));
				}
				else if (String::equals({ "-o", "--output-path" }, key))
				{
					m_inputs.setBuildPath(std::move(value));
				}
				else if (String::equals({ "-t", "--toolchain" }, key))
				{
					m_inputs.setToolchainPreference(std::move(value));
				}
				else if (String::equals({ "-g", "--generator" }, key))
				{
					m_inputs.setGenerator(std::move(value));
				}
				else if (String::equals({ "-e", "--envfile" }, key))
				{
					m_inputs.setEnvFile(std::move(value));
				}
				else if (String::equals({ "-a", "--arch" }, key))
				{
					m_inputs.setArchRaw(value);
					if (String::contains(',', value))
					{
						auto split = String::split(value, ',');
						m_inputs.setTargetArchitecture(std::move(split.front()));
					}
					else
					{
						m_inputs.setTargetArchitecture(std::move(value));
					}
				}
				else if (key == patterns.argInitPath())
				{
					m_inputs.setInitPath(std::move(value));
				}
				break;
			}

			case Variant::Kind::StringList: {
				if (key == patterns.argRunArguments())
				{
					auto runArgs = String::join(rawValue.asStringList());
					m_inputs.setRunOptions(std::move(runArgs));
				}
				break;
			}

			case Variant::Kind::Boolean: {
				bool value = rawValue.asBool();
				if (String::equals("--save-schema", key))
				{
					m_inputs.setSaveSchemaToFile(value);
				}
				else if (String::equals("--quieter", key))
				{
					Output::setQuietNonBuild(value);
				}
				break;
			}
			default:
				break;
		}
	}

	return true;
}
}
