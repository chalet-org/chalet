/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/ZipArchiver.hpp"

#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ZipArchiver::ZipArchiver(const StatePrototype& inPrototype) :
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool ZipArchiver::archive(const std::string& inFilename, const StringList& inFiles, const std::string& inCwd, const StringList& inExcludes)
{
	std::string filename;
	if (!String::endsWith(".zip", inFilename))
		filename = fmt::format("{}.zip", inFilename);
	else
		filename = inFilename;

#if defined(CHALET_WIN32)
	const auto& powershell = m_prototype.tools.powershell();
	if (powershell.empty() || !Commands::pathExists(powershell))
	{
		Diagnostic::error("Couldn't create archive '{}' because 'powershell' was not found.", filename);
		return false;
	}
#else
	const auto& zip = m_prototype.tools.zip();
	if (zip.empty() || !Commands::pathExists(zip))
	{
		Diagnostic::error("Couldn't create archive '{}' because 'zip' was not found.", filename);
		return false;
	}
#endif

	auto tmpDirectory = fmt::format("{}/{}", inCwd, inFilename);
	Commands::makeDirectory(tmpDirectory);

#if defined(CHALET_WIN32)
	for (auto& file : inFiles)
	{
		if (List::contains(inExcludes, file))
			continue;

		Commands::copySilent(file, tmpDirectory);
	}

	StringList cmd{
		powershell,
		"-Command",
		"Compress-Archive",
		"-Force",
		"-Path",
		inFilename,
		"-DestinationPath",
		filename,
	};
#else
	StringList cmd{
		zip,
		"-r",
		"-X",
		filename,
	};

	for (auto& file : inFiles)
	{
		if (List::contains(inExcludes, file))
			continue;

		Commands::copySilent(file, tmpDirectory);

		auto outFile = fmt::format("{}{}", inFilename, file.substr(inCwd.size()));
		cmd.emplace_back(std::move(outFile));
	}
#endif

	bool result = Commands::subprocessMinimalOutput(cmd, inCwd);

	Commands::removeRecursively(tmpDirectory);

	if (!result)
	{
		Diagnostic::error("Couldn't create archive '{}' because '{}' ran into a problem.", filename, cmd.front());
	}

	return result;
}
}