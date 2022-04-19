/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/FileArchiver.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleArchiveTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
FileArchiver::FileArchiver(const BuildState& inState) :
	m_state(inState),
	m_outputDirectory(m_state.inputs.distributionDirectory())
{
}

/*****************************************************************************/
bool FileArchiver::archive(const BundleArchiveTarget& inTarget, const std::string& inBaseName, const StringList& inFiles, const StringList& inExcludes)
{
	m_format = inTarget.format();
	m_outputFilename = inTarget.getOutputFilename(inBaseName);

	StringList cmd;
	if (m_format == ArchiveFormat::Zip)
	{
		if (!zipIsValid())
			return false;

		cmd = getZipFormatCommand(inBaseName, inFiles);
	}
	else if (m_format == ArchiveFormat::Tar)
	{
		if (!tarIsValid())
			return false;

		cmd = getTarFormatCommand(inBaseName, inFiles);
	}

	if (cmd.empty())
	{
		Diagnostic::error("Invalid archive format requested: {}.", static_cast<int>(m_format));
		return false;
	}

	m_tmpDirectory = makeTemporaryDirectory(inBaseName, inFiles, inExcludes);

	bool result = Commands::subprocessMinimalOutput(cmd, m_outputDirectory);
	Commands::removeRecursively(m_tmpDirectory);
	if (!result)
	{
		Diagnostic::error("Couldn't create archive '{}' because '{}' ran into a problem.", m_outputFilename, cmd.front());
	}

	return result;
}

/*****************************************************************************/
bool FileArchiver::powerShellIsValid() const
{
	const auto& powershell = m_state.tools.powershell();
	if (powershell.empty() || !Commands::pathExists(powershell))
	{
		Diagnostic::error("Couldn't create archive '{}' because 'powershell' was not found.", m_outputFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool FileArchiver::zipIsValid() const
{
#if defined(CHALET_WIN32)
	if (!powerShellIsValid())
		return false;
#else
	const auto& zip = m_state.tools.zip();
	if (zip.empty() || !Commands::pathExists(zip))
	{
		Diagnostic::error("Couldn't create archive '{}' because 'zip' was not found.", m_outputFilename);
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
bool FileArchiver::tarIsValid() const
{
#if defined(CHALET_WIN32)
	if (!powerShellIsValid())
		return false;
#else
	const auto& tar = m_state.tools.tar();
	if (tar.empty() || !Commands::pathExists(tar))
	{
		Diagnostic::error("Couldn't create archive '{}' because 'tar' was not found.", m_outputFilename);
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
std::string FileArchiver::makeTemporaryDirectory(const std::string& inBaseName, const StringList& inFiles, const StringList& inExcludes) const
{
	auto ret = fmt::format("{}/{}", m_outputDirectory, inBaseName);
	Commands::makeDirectory(ret);

	for (auto& file : inFiles)
	{
		if (List::contains(inExcludes, file))
			continue;

		Commands::copySilent(file, ret);
	}

	return ret;
}

/*****************************************************************************/
StringList FileArchiver::getZipFormatCommand(const std::string& inBaseName, const StringList& inFiles) const
{
	StringList cmd;

#if defined(CHALET_WIN32)
	StringList pwshCmd{
		"Compress-Archive",
		"-Force",
		"-Path",
		inBaseName, // (m_tmpDirectory)
		"-DestinationPath",
		m_outputFilename,
	};

	// Note: We need to hide the weird MS progress dialog (Write-Progress),
	//   which is done with $ProgressPreference

	const auto& powershell = m_state.tools.powershell();
	cmd.emplace_back(powershell);
	cmd.emplace_back("-Command");
	cmd.emplace_back("$ProgressPreference = \"SilentlyContinue\";");
	cmd.emplace_back(fmt::format("{};", String::join(pwshCmd)));
	cmd.emplace_back("$ProgressPreference = \"Continue\";");

	UNUSED(inFiles);

#else
	const auto& zip = m_state.tools.zip();
	cmd.emplace_back(zip);
	cmd.emplace_back("-r");
	cmd.emplace_back("-X");
	cmd.emplace_back(m_outputFilename);

	for (auto& file : inFiles)
	{
		auto outFile = fmt::format("{}{}", inBaseName, file.substr(m_outputDirectory.size()));
		cmd.emplace_back(std::move(outFile));
	}
#endif

	return cmd;
}

/*****************************************************************************/
StringList FileArchiver::getTarFormatCommand(const std::string& inBaseName, const StringList& inFiles) const
{
	StringList cmd;

#if defined(CHALET_WIN32)
	const auto& powershell = m_state.tools.powershell();
	cmd.emplace_back(powershell);
	cmd.emplace_back("tar");
#else
	const auto& tar = m_state.tools.tar();
	cmd.emplace_back(tar);
#endif
	cmd.emplace_back("-c");
	cmd.emplace_back("-z");
	cmd.emplace_back("-f");
	cmd.emplace_back(m_outputFilename);
	cmd.emplace_back(fmt::format("--directory={}", inBaseName));
#if defined(CHALET_WIN32)
	cmd.emplace_back("*");
#else
	cmd.emplace_back(".");
#endif

	UNUSED(inFiles);

	return cmd;
}
}
