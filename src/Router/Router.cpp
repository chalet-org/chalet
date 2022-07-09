/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/QueryController.hpp"
#include "Export/IProjectExporter.hpp"
#include "Process/ProcessController.hpp"
#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/TerminalTest.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Utility/Uuid.hpp"
#include "Xml/Xml.hpp"
#include "Xml/XmlFile.hpp"

#include "Libraries/Json.hpp"

#include <chrono>
#include <thread>

namespace chalet
{
namespace
{
using RouteAction = std::function<bool(Router&)>;
using RouteList = std::unordered_map<RouteType, RouteAction>;
}
/*****************************************************************************/
Router::Router(CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool Router::run()
{
	if (!parseTheme())
		return false;

	const auto& route = m_inputs.route();
	if (route.isUnknown())
	{
		Diagnostic::error("Command not recognized.");
		return false;
	}

	// Routes that don't require state
	switch (route.type())
	{
		case RouteType::Query:
			return routeQuery();

		case RouteType::TerminalTest:
			return routeTerminalTest();

		case RouteType::Init:
			return routeInit();

		case RouteType::SettingsGet:
		case RouteType::SettingsSet:
		case RouteType::SettingsUnset:
		case RouteType::SettingsGetKeys:
			return routeSettings();

#if defined(CHALET_DEBUG)
		case RouteType::Debug:
			return routeDebug();
#endif

		default: break;
	}

	return runRoutesThatRequireState();
}

/*****************************************************************************/
bool Router::runRoutesThatRequireState()
{
	const auto& route = m_inputs.route();
	if (route.isExport() && m_inputs.exportKind() == ExportKind::None)
	{
		Diagnostic::error("The requested project kind '{}' was not recognized, or is not yet supported.", m_inputs.exportKindRaw());
		return false;
	}

	auto centralState = std::make_unique<CentralState>(m_inputs);
	Unique<BuildState> buildState;

	if (!centralState->initialize())
		return false;

	if (!route.isExport())
	{
		buildState = std::make_unique<BuildState>(centralState->inputs(), *centralState);
		if (!buildState->initialize())
			return false;
	}

	bool result = false;
	switch (route.type())
	{
		case RouteType::Bundle: {
			chalet_assert(buildState != nullptr, "");
			result = routeBundle(*buildState);
			break;
		}

		case RouteType::Configure: {
			chalet_assert(buildState != nullptr, "");
			result = routeConfigure(*buildState);
			break;
		}

		case RouteType::BuildRun:
		case RouteType::Build:
		case RouteType::Rebuild:
		case RouteType::Run:
		case RouteType::Clean: {
			chalet_assert(buildState != nullptr, "");
			result = buildState->doBuild(m_inputs.route());
			break;
		}

		case RouteType::Export: {
			result = routeExport(*centralState);
			break;
		}

		default:
			break;
	}

	if (centralState != nullptr)
		centralState->saveCaches();

	return result;
}

/*****************************************************************************/
bool Router::routeConfigure(BuildState& inState)
{
	bool addLineBreak = false;
	if (inState.environment != nullptr)
	{
		addLineBreak |= inState.environment->ouptuttedDescription();
	}

	if (addLineBreak)
		Output::lineBreak();

	Output::msgConfigureCompleted(inState.workspace.metadata().name());
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::routeBundle(BuildState& inState)
{
	const auto& inputFile = m_inputs.inputFile();
	if (inState.distribution.empty())
	{
		Diagnostic::error("{}: There are no distribution targets: missing 'distribution'", inputFile);
		return false;
	}

	AppBundler bundler(inState);
	{
		CommandRoute route(RouteType::Build);
		if (!inState.doBuild(route, false))
			return false;
	}

	for (auto& target : inState.distribution)
	{
		if (!bundler.run(target))
			return false;
	}

	bundler.reportErrors();

	Output::msgBuildSuccess();
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::routeInit()
{
	ProjectInitializer initializer(m_inputs);
	initializer.run();

	return true;
}

/*****************************************************************************/
bool Router::routeSettings()
{
	const auto& route = m_inputs.route();

	SettingsManager settingsMgr(m_inputs);
	if (!settingsMgr.run(static_cast<SettingsAction>(route.type())))
		return true;

	return true;
}

/*****************************************************************************/
bool Router::routeQuery()
{
	CentralState centralState(m_inputs);
	if (!centralState.initializeForList())
		return false;

	QueryController query(centralState);
	return query.printListOfRequestedType();
}

/*****************************************************************************/
bool Router::routeTerminalTest()
{
	TerminalTest termTest;
	return termTest.run();
}

/*****************************************************************************/
bool Router::parseTheme()
{
	ThemeSettingsJsonParser themeParser(m_inputs);
	if (!themeParser.serialize())
		return false;

	return true;
}

/*****************************************************************************/
bool Router::routeExport(CentralState& inCentralState)
{
	auto projectExporter = IProjectExporter::make(m_inputs.exportKind(), inCentralState);
	if (!projectExporter->generate())
		return false;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
bool Router::routeDebug()
{
	LOG("Router::routeDebug()");

	Timer timer;

	// std::string paths;
	// Commands::forEachGlobMatch("src/*.{cpp,rc}", GlobMatch::Files, [&paths](std::string path) {
	// 	paths += path + '\n';
	// });

	// paths += '\n';
	// LOG(paths);
	// paths.clear();
	// Commands::forEachGlobMatch("src/**.{cpp,rc}", GlobMatch::Files, [&paths](std::string path) {
	// 	paths += path + '\n';
	// });

	// paths += '\n';
	// LOG(paths);

	// LOG("glob took:", timer.asString());

	XmlFile xmlFile("test.xml");

	auto& xml = xmlFile.xml;

	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("DefaultTargets", "Build");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addElement("ItemGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ProjectConfigurations");
		node.addElement("ProjectConfiguration", [](XmlElement& node2) {
			node2.addAttribute("Include", "Debug|Win32");
			node2.addElementWithText("Configuration", "Debug");
			node2.addElementWithText("Platform", "Win32");
		});
		node.addElement("ProjectConfiguration", [](XmlElement& node2) {
			node2.addAttribute("Include", "Release|Win32");
			node2.addElementWithText("Configuration", "Release");
			node2.addElementWithText("Platform", "Win32");
		});
		node.addElement("ProjectConfiguration", [](XmlElement& node2) {
			node2.addAttribute("Include", "Debug|Win32");
			node2.addElementWithText("Configuration", "Debug");
			node2.addElementWithText("Platform", "Win32");
		});
		node.addElement("ProjectConfiguration", [](XmlElement& node2) {
			node2.addAttribute("Include", "Release|x64");
			node2.addElementWithText("Configuration", "Release");
			node2.addElementWithText("Platform", "x64");
			node2.setCommented(true);
		});
		node.addElement("Test&Stuff'\"<>", [](XmlElement& node2) {
			node2.addAttribute("Test&Stuff'\"<>", "Test&Stuff'\"<>");
			node2.setText("Test&Stuff'\"<>");
		});
	});

	LOG(xml.dump(2));
	LOG("----");
	LOG("xml took:", timer.asString());
	LOG("----");

	xmlFile.save(2);

	auto xmllint = Commands::which("xmllint");
	if (!xmllint.empty())
	{
		Commands::subprocess({ xmllint, xmlFile.filename() });
	}

	LOG("----");
	timer.restart();

	auto guid = Uuid::getNil();
	LOG(guid.str());

	LOG("----");
	auto seedA = Uuid::v4().toUpperCase();
	std::string seedB("8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942");

	for (uint i = 0; i < 8; ++i)
	{
		guid = Uuid::v4();
		LOG(guid.toUpperCase());
	}

	LOG("----");
	for (uint i = 0; i < 4; ++i)
	{
		guid = Uuid::v5("test", seedA);
		LOG(guid.toUpperCase());
	}

	LOG("----");
	for (uint i = 0; i < 4; ++i)
	{
		guid = Uuid::v5("test", seedB);
		LOG(guid.toUpperCase());
	}

	LOG("----");
	LOG("uuid took:", timer.asString());
	LOG("----");
	return true;
}
#endif

}
