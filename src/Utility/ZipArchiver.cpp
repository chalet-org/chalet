/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/ZipArchiver.hpp"

#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ZipArchiver::ZipArchiver(const StatePrototype& inPrototype) :
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool ZipArchiver::archive(const std::string& inFilename, const StringList& inFiles, const std::string& inCwd)
{
	std::string filename;
	if (!String::endsWith(".zip", inFilename))
		filename = fmt::format("{}.zip", inFilename);
	else
		filename = inFilename;

#if defined(CHALET_WIN32)
	const auto& powershell = m_prototype.tools.powershell();
	if (powershell.empty())
	{
		Diagnostic::error("Couldn't create archive '{}' because 'powershell' was not found.", filename);
		return false;
	}
#else
	const auto& zip = m_prototype.tools.zip();
	if (zip.empty())
	{
		Diagnostic::error("Couldn't create archive '{}' because 'zip' was not found.", filename);
		return false;
	}
#endif

	auto tmpDirectory = fmt::format("{}/{}", inCwd, inFilename);
	Commands::makeDirectory(tmpDirectory);

	StringList cmd{
		zip,
		"-r",
		"-X",
		filename,
	};

	for (auto& file : inFiles)
	{
		Commands::copySilent(file, tmpDirectory);

		auto outFile = fmt::format("{}{}", inFilename, file.substr(inCwd.size()));
		cmd.emplace_back(std::move(outFile));
	}

	bool result = Commands::subprocessNoOutput(cmd, inCwd);

	Commands::removeRecursively(tmpDirectory);

	if (!result)
	{
		Diagnostic::error("Couldn't create archive '{}' because 'zip' ran into a problem.", filename);
	}

	return result;
}
}
