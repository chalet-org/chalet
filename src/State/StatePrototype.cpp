/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/StatePrototype.hpp"

#include "BuildJson/BuildJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Libraries/Format.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Distribution/ScriptDistTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
StatePrototype::StatePrototype(const CommandLineInputs& inInputs, std::string inFilename) :
	m_inputs(inInputs),
	m_filename(std::move(inFilename)),
	m_buildJson(std::make_unique<JsonFile>(m_filename))
{
}

/*****************************************************************************/
bool StatePrototype::initialize()
{
	// Note: existence of m_filename is checked by Router (before the cache is made)

	Timer timer;
	Diagnostic::info(fmt::format("Validating Build File [{}]", m_filename), false);

	Json buildJsonSchema = Schema::getBuildJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(buildJsonSchema, "schema/chalet.schema.json");
	}

	// TODO: schema versioning
	if (!m_buildJson->validate(std::move(buildJsonSchema)))
	{
		Diagnostic::error(fmt::format("{}: There was an error validating the file against its schema.", m_filename));
		return false;
	}

	if (!parseRequired(m_buildJson->json))
		return false;

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
JsonFile& StatePrototype::jsonFile() noexcept
{
	return *m_buildJson;
}

/*****************************************************************************/
const std::string& StatePrototype::filename() const noexcept
{
	return m_buildJson->filename();
}

/*****************************************************************************/
const StringList& StatePrototype::allowedBuildConfigurations() const noexcept
{
	return m_allowedBuildConfigurations;
}

/*****************************************************************************/
const StringList& StatePrototype::requiredBuildConfigurations() const noexcept
{
	return m_requiredBuildConfigurations;
}

/*****************************************************************************/
const StringList& StatePrototype::requiredArchitectures() const noexcept
{
	return m_requiredArchitectures;
}

/*****************************************************************************/
bool StatePrototype::parseRequired(const Json& inNode)
{

	if (!inNode.is_object())
		return false;

	if (inNode.contains(kKeyConfigurations))
	{
		// TODO: Get all build configurations + data
		auto& configurationsNode = inNode.at(kKeyConfigurations);
		if (configurationsNode.is_object())
		{
			for (auto& [name, _] : configurationsNode.items())
			{
				m_allowedBuildConfigurations.push_back(name);
			}
		}
	}

	if (!parseDistribution(inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool StatePrototype::parseDistribution(const Json& inNode)
{
	if (!inNode.contains(kKeyDistribution))
		return true;

	const Json& distributionJson = inNode.at(kKeyDistribution);
	if (!distributionJson.is_object() || distributionJson.size() == 0)
	{
		Diagnostic::error(fmt::format("{}: '{}' must contain at least one bundle or script.", m_filename, kKeyDistribution));
		return false;
	}

	for (auto& [name, targetJson] : distributionJson.items())
	{
		if (!targetJson.is_object())
		{
			Diagnostic::error(fmt::format("{}: distribution bundle '{}' must be an object.", m_filename, name));
			return false;
		}

		DistTargetType type = DistTargetType::DistributionBundle;
		if (m_buildJson->containsKeyThatStartsWith(targetJson, "script"))
		{
			type = DistTargetType::Script;
		}

		DistributionTarget target = IDistTarget::make(type);
		target->setName(name);

		if (target->isScript())
		{
			if (!parseScript(static_cast<ScriptDistTarget&>(*target), targetJson))
				continue;
		}
		else
		{
			if (!parseBundle(static_cast<BundleTarget&>(*target), targetJson))
				return false;
		}

		distribution.push_back(std::move(target));
	}

	return true;
}

/*****************************************************************************/
bool StatePrototype::parseScript(ScriptDistTarget& outScript, const Json& inNode)
{
	const std::string key{ "script" };

	if (StringList list; assignStringListFromConfig(list, inNode, key))
		outScript.addScripts(std::move(list));
	else if (std::string val; assignStringFromConfig(val, inNode, key))
		outScript.addScript(std::move(val));
	else
		return false;

	if (std::string val; assignStringFromConfig(val, inNode, "description"))
		outScript.setDescription(std::move(val));

	// if (!parsePlatformConfigExclusions(outProject, inNode))
	// 	return false;

	return true;
}

/*****************************************************************************/
bool StatePrototype::parseBundle(BundleTarget& outBundle, const Json& inNode)
{
	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "configuration"))
	{
		outBundle.setConfiguration(std::move(val));
		List::addIfDoesNotExist(m_requiredBuildConfigurations, outBundle.configuration());
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "description"))
		outBundle.setDescription(std::move(val));

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "outDir"))
		outBundle.setOutDir(std::move(val));

	if (std::string val; m_buildJson->assignStringAndValidate(val, inNode, "mainProject"))
		outBundle.setMainProject(std::move(val));

	if (bool val; parseKeyFromConfig(val, inNode, "includeDependentSharedLibraries"))
		outBundle.setIncludeDependentSharedLibraries(val);

	if (StringList list; assignStringListFromConfig(list, inNode, "projects"))
		outBundle.addProjects(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "dependencies"))
		outBundle.addDependencies(std::move(list));

	if (StringList list; assignStringListFromConfig(list, inNode, "exclude"))
		outBundle.addExcludes(std::move(list));

#if defined(CHALET_LINUX)
	return parseBundleLinux(outBundle, inNode);
#elif defined(CHALET_MACOS)
	return parseBundleMacOS(outBundle, inNode);
#elif defined(CHALET_WIN32)
	return parseBundleWindows(outBundle, inNode);
#else
	#error "Unrecognized platform"
	return false;
#endif
}

/*****************************************************************************/
bool StatePrototype::parseBundleLinux(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("linux"))
		return true;

	const Json& linuxNode = inNode.at("linux");
	if (!linuxNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}.linux' must be an object.", m_filename, kKeyDistribution));
		return false;
	}

	BundleLinux linuxBundle;

	int assigned = 0;
	if (std::string val; m_buildJson->assignStringAndValidate(val, linuxNode, "icon"))
	{
		linuxBundle.setIcon(std::move(val));
		assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, linuxNode, "desktopEntry"))
	{
		linuxBundle.setDesktopEntry(std::move(val));
		assigned++;
	}

	if (assigned == 0)
		return false; // not an error

	if (assigned == 1)
	{
		Diagnostic::error(fmt::format("{}: '{bundle}.linux.icon' & '{bundle}.linux.desktopEntry' are both required.",
			m_filename,
			fmt::arg("bundle", kKeyDistribution)));
		return false;
	}

	outBundle.setLinuxBundle(std::move(linuxBundle));

	return true;
}

/*****************************************************************************/
bool StatePrototype::parseBundleMacOS(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("macos"))
		return true;

	const Json& macosNode = inNode.at("macos");
	if (!macosNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}.macos' must be an object.", m_filename, kKeyDistribution));
		return false;
	}

	BundleMacOS macosBundle;

	// int assigned = 0;
	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "bundleName"))
	{
		macosBundle.setBundleName(std::move(val));
		// assigned++;
	}

	if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, "icon"))
		macosBundle.setIcon(std::move(val));

	const std::string kInfoPropertyList{ "infoPropertyList" };
	if (macosNode.contains(kInfoPropertyList))
	{
		auto& infoPlistNode = macosNode.at(kInfoPropertyList);
		if (infoPlistNode.is_object())
		{
			macosBundle.setInfoPropertyListContent(infoPlistNode.dump());
			// assigned++;
		}
		else
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, kInfoPropertyList))
			{
				macosBundle.setInfoPropertyList(std::move(val));
				// assigned++;
			}
		}
	}

	if (bool val = false; m_buildJson->assignFromKey(val, macosNode, "universalBinary"))
	{
		macosBundle.setUniversalBinary(val);

		List::addIfDoesNotExist(m_requiredArchitectures, std::string("x86_64-apple-darwin"));
		List::addIfDoesNotExist(m_requiredArchitectures, std::string("arm64-apple-darwin"));
	}

	if (bool val = false; m_buildJson->assignFromKey(val, macosNode, "makeDmg"))
		macosBundle.setMakeDmg(val);

	const std::string kDmgBackground{ "dmgBackground" };
	if (macosNode.contains(kDmgBackground))
	{
		const Json& dmgBackground = macosNode[kDmgBackground];
		if (dmgBackground.is_object())
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, dmgBackground, "1x"))
				macosBundle.setDmgBackground1x(std::move(val));

			if (std::string val; m_buildJson->assignStringAndValidate(val, dmgBackground, "2x"))
				macosBundle.setDmgBackground2x(std::move(val));
		}
		else
		{
			if (std::string val; m_buildJson->assignStringAndValidate(val, macosNode, kDmgBackground))
				macosBundle.setDmgBackground1x(std::move(val));
		}
	}

	// if (assigned == 0)
	// 	return false; // not an error

	// if (assigned >= 1 && assigned < 2)
	// {
	// 	Diagnostic::error(fmt::format("{}: '{bundle}.macos.bundleName' is required.",
	// 		m_filename,
	// 		fmt::arg("bundle", kKeyDistribution)));
	// 	return false;
	// }

	outBundle.setMacosBundle(std::move(macosBundle));

	return true;
}

/*****************************************************************************/
bool StatePrototype::parseBundleWindows(BundleTarget& outBundle, const Json& inNode)
{
	if (!inNode.contains("windows"))
		return true;

	const Json& windowsNode = inNode.at("windows");
	if (!windowsNode.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}.windows' must be an object.", m_filename, kKeyDistribution));
		return false;
	}

	BundleWindows windowsBundle;

	// int assigned = 0;
	// if (std::string val; m_buildJson->assignStringAndValidate(val, windowsNode, "icon"))
	// {
	// 	windowsBundle.setIcon(val);
	// 	assigned++;
	// }

	// if (assigned == 0)
	// 	return false;

	outBundle.setWindowsBundle(std::move(windowsBundle));

	return true;
}

/*****************************************************************************/
/*****************************************************************************/
bool StatePrototype::assignStringFromConfig(std::string& outVariable, const Json& inNode, const std::string& inKey, const std::string& inDefault)
{
	bool res = m_buildJson->assignStringAndValidate(outVariable, inNode, inKey, inDefault);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}.{}", inKey, platform), inDefault);

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson->assignStringAndValidate(outVariable, inNode, fmt::format("{}.!{}", inKey, notPlatform), inDefault);
	}

	return res;
}

/*****************************************************************************/
bool StatePrototype::assignStringListFromConfig(StringList& outList, const Json& inNode, const std::string& inKey)
{
	bool res = m_buildJson->assignStringListAndValidate(outList, inNode, inKey);

	const auto& platform = m_inputs.platform();

	res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}.{}", inKey, platform));

	for (auto& notPlatform : m_inputs.notPlatforms())
	{
		res |= m_buildJson->assignStringListAndValidate(outList, inNode, fmt::format("{}.!{}", inKey, notPlatform));
	}

	return res;
}

}
