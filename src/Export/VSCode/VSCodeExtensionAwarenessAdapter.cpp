/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeExtensionAwarenessAdapter.hpp"

#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "System/DefinesGithub.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeExtensionAwarenessAdapter::VSCodeExtensionAwarenessAdapter(const bool inVsCodium) :
	m_vscodium(inVsCodium)
{
}

/*****************************************************************************/
void VSCodeExtensionAwarenessAdapter::initialize()
{
	m_codePath = getCodePath();

	auto extensions = getInstalledExtensions();
	m_chaletExtensionInstalled = List::contains(extensions, std::string(CHALET_VSCODE_EXTENSION));
	m_cppToolsExtensionInstalled = List::contains(extensions, std::string("ms-vscode.cpptools"));

	if (!m_chaletExtensionInstalled)
	{
		// The Chalet extension contains the type of JSON/Yaml schema resolution
		//   we want (through this process), so just force-install it
		//
		installChaletExtension();
	}
}

/*****************************************************************************/
bool VSCodeExtensionAwarenessAdapter::vscodium() const noexcept
{
	return m_vscodium;
}

/*****************************************************************************/
const std::string& VSCodeExtensionAwarenessAdapter::codePath() const noexcept
{
	return m_codePath;
}

/*****************************************************************************/
bool VSCodeExtensionAwarenessAdapter::chaletExtensionInstalled() const noexcept
{
	return m_chaletExtensionInstalled;
}
bool VSCodeExtensionAwarenessAdapter::cppToolsExtensionInstalled() const noexcept
{
	return m_cppToolsExtensionInstalled;
}

/*****************************************************************************/
bool VSCodeExtensionAwarenessAdapter::installChaletExtension()
{
	if (m_codePath.empty())
		return false;

	StringList cmd{
		m_codePath,
		"--install-extension",
		CHALET_VSCODE_EXTENSION,
		"--force",
	};

	bool result = Process::runMinimalOutput(cmd);
	if (result)
	{
		m_chaletExtensionInstalled = true;
	}
	return result;
}

/*****************************************************************************/
std::string VSCodeExtensionAwarenessAdapter::getCodePath() const
{
	auto codeShell = m_vscodium ? "codium" : "code";

	// auto project = Files::getCanonicalPath(inProject);
	auto code = Files::which(codeShell);
#if defined(CHALET_WIN32)
	if (code.empty())
	{
		if (m_vscodium)
		{
			auto programFiles = Environment::getProgramFiles();
			code = Files::getCanonicalPath(fmt::format("{}/VSCodium/VSCodium.exe", programFiles));
		}
		else
		{
			auto appData = Environment::get("APPDATA");
			code = Files::getCanonicalPath(fmt::format("{}/../Local/Programs/Microsoft VS Code/Code.exe", appData));
		}

		if (!Files::pathExists(code))
			code.clear();
	}
#endif
	return code;
}

/*****************************************************************************/
StringList VSCodeExtensionAwarenessAdapter::getInstalledExtensions() const
{
	if (m_codePath.empty())
		return StringList{};

	// Ignore errors... when this was tested, some electron v8 errors threw
	//   but extensions were still listed via stdout
	//
	auto extensionsRaw = Process::runOutput({ m_codePath, "--list-extensions" }, PipeOption::Pipe, PipeOption::Close);
	StringList extensions = String::split(extensionsRaw, '\n');
	return extensions;
}

}
