/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/FileArchiver.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleArchiveTarget.hpp"
#include "Terminal/Files.hpp"
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
bool FileArchiver::archive(const BundleArchiveTarget& inTarget, const std::string& inBaseName, const StringList& inIncludes, const StringList& inExcludes)
{
	m_format = inTarget.format();
	auto exactpath = Files::getAbsolutePath(Files::getCanonicalPath(m_outputDirectory));
	m_outputFilename = fmt::format("{}/{}", exactpath, inTarget.getOutputFilename(inBaseName));

	if (Files::pathExists(m_outputFilename))
		Files::remove(m_outputFilename);

	auto resolvedIncludes = getResolvedIncludes(inIncludes);

	m_tmpDirectory = makeTemporaryDirectory(inBaseName);

	if (!copyIncludestoTemporaryDirectory(resolvedIncludes, inExcludes, m_tmpDirectory))
		return false;

	StringList cmd;
	if (m_format == ArchiveFormat::Zip)
	{
		if (!zipIsValid())
			return false;

		if (!getZipFormatCommand(cmd, inBaseName, resolvedIncludes))
		{
			Diagnostic::error("Couldn't create archive '{}' because there were no input files.", m_outputFilename);
			return false;
		}
	}
	else if (m_format == ArchiveFormat::Tar)
	{
		if (!tarIsValid())
			return false;

		if (!getTarFormatCommand(cmd, inBaseName, resolvedIncludes))
		{
			Diagnostic::error("Couldn't create archive '{}' because there were no input files.", m_outputFilename);
			return false;
		}
	}

	if (cmd.empty())
	{
		Diagnostic::error("Invalid archive format requested: {}.", static_cast<i32>(m_format));
		return false;
	}

	bool result = Files::subprocessMinimalOutput(cmd, m_tmpDirectory);
	Files::removeRecursively(m_tmpDirectory);
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
	if (powershell.empty() || !Files::pathExists(powershell))
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
	if (zip.empty() || !Files::pathExists(zip))
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
	if (tar.empty() || !Files::pathExists(tar))
	{
		Diagnostic::error("Couldn't create archive '{}' because 'tar' was not found.", m_outputFilename);
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
StringList FileArchiver::getResolvedIncludes(const StringList& inIncludes) const
{
	StringList ret;
	StringList tmp;
	for (auto& include : inIncludes)
	{
		if (String::equals('*', include))
		{
			Files::addPathToListWithGlob(fmt::format("{}/*", m_outputDirectory), tmp, GlobMatch::FilesAndFoldersExact);
		}
		else
		{
			auto filePath = fmt::format("{}/{}", m_outputDirectory, include);
			if (Files::pathExists(filePath))
			{
				List::addIfDoesNotExist(tmp, std::move(filePath));
			}
			else if (Files::pathExists(include))
			{
				List::addIfDoesNotExist(ret, include);
			}
			else
			{
				Files::forEachGlobMatch(filePath, GlobMatch::FilesAndFoldersExact, [&ret](std::string inPath) {
					List::addIfDoesNotExist(ret, std::move(inPath));
				});

				Files::forEachGlobMatch(include, GlobMatch::FilesAndFoldersExact, [&ret](std::string inPath) {
					List::addIfDoesNotExist(ret, std::move(inPath));
				});
			}
		}
	}

	for (auto& include : tmp)
	{
		List::addIfDoesNotExist(ret, include.substr(m_outputDirectory.size() + 1));
	}

	return ret;
}

/*****************************************************************************/
std::string FileArchiver::makeTemporaryDirectory(const std::string& inBaseName) const
{
	auto ret = fmt::format("{}/{}", m_outputDirectory, inBaseName);
	if (Files::pathExists(ret))
		Files::removeRecursively(ret);

	Files::makeDirectory(ret);

	return ret;
}

/*****************************************************************************/
bool FileArchiver::copyIncludestoTemporaryDirectory(const StringList& inIncludes, const StringList& inExcludes, const std::string& inDirectory) const
{
	if (inDirectory.empty())
		return false;

	for (auto& file : inIncludes)
	{
		if (List::contains(inExcludes, file))
			continue;

		// if (!Files::pathExists(file))
		// 	continue;

		auto resolved = fmt::format("{}/{}", m_outputDirectory, file);
		if (!Files::pathExists(resolved))
			resolved = file;

		if (!Files::copySilent(resolved, inDirectory))
		{
			Diagnostic::error("File not found: {}", file);
			Diagnostic::error("Couldn't create archive '{}'.", m_outputFilename);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool FileArchiver::getZipFormatCommand(StringList& outCmd, const std::string& inBaseName, const StringList& inIncludes) const
{
	outCmd.clear();

	UNUSED(inBaseName);

#if defined(CHALET_WIN32)
	StringList pwshCmd{
		"Compress-Archive",
		"-Force",
		"-Path",
		"./*",
		"-DestinationPath",
		m_outputFilename,
	};

	// Note: We need to hide the weird MS progress dialog (Write-Progress),
	//   which is done with $ProgressPreference

	const auto& powershell = m_state.tools.powershell();
	outCmd.emplace_back(powershell);
	outCmd.emplace_back("-Command");
	outCmd.emplace_back("$ProgressPreference = \"SilentlyContinue\";");
	outCmd.emplace_back(fmt::format("{};", String::join(pwshCmd)));
	outCmd.emplace_back("$ProgressPreference = \"Continue\";");

	UNUSED(inIncludes);

	return !inIncludes.empty();
#else
	const auto& zip = m_state.tools.zip();
	outCmd.emplace_back(zip);
	outCmd.emplace_back("-r");
	outCmd.emplace_back("-X");
	outCmd.emplace_back(m_outputFilename);
	outCmd.emplace_back("--symlinks");
	outCmd.emplace_back(".");
	outCmd.emplace_back("-i");

	auto files = getIncludesForCommand(inIncludes);
	for (auto& file : files)
	{
		outCmd.emplace_back(file);
	}

	return !files.empty();
#endif
}

/*****************************************************************************/
bool FileArchiver::getTarFormatCommand(StringList& outCmd, const std::string& inBaseName, const StringList& inIncludes) const
{
	outCmd.clear();

	UNUSED(inBaseName);

#if defined(CHALET_WIN32)
	const auto& powershell = m_state.tools.powershell();
	outCmd.emplace_back(powershell);
	outCmd.emplace_back("tar");
#else
	const auto& tar = m_state.tools.tar();
	outCmd.emplace_back(tar);
#endif
	outCmd.emplace_back("-c");
	outCmd.emplace_back("-z");
	outCmd.emplace_back("-f");
	outCmd.emplace_back(m_outputFilename);
	// outCmd.emplace_back(fmt::format("--directory={}", inBaseName));

	auto files = getIncludesForCommand(inIncludes);
	for (auto& file : files)
	{
		outCmd.emplace_back(file);
	}

	return !files.empty();
}

/*****************************************************************************/
StringList FileArchiver::getIncludesForCommand(const StringList& inIncludes) const
{
	StringList ret;

	auto cwd = Files::getWorkingDirectory();
	Files::changeWorkingDirectory(m_tmpDirectory);

	for (auto& file : inIncludes)
	{
		auto filename = String::getPathFilename(file);
		if (Files::pathIsDirectory(filename))
		{
			Files::addPathToListWithGlob(fmt::format("{}/*", filename), ret, GlobMatch::FilesAndFolders);
		}
		else
		{
			if (Files::pathExists(filename))
				List::addIfDoesNotExist(ret, std::move(filename));
			else
				List::addIfDoesNotExist(ret, file);
		}
	}

	Files::changeWorkingDirectory(cwd);

	return ret;
}
}
