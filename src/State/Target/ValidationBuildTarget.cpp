/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/ValidationBuildTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ValidationBuildTarget::ValidationBuildTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::Validation)
{
}

/*****************************************************************************/
bool ValidationBuildTarget::initialize()
{
	if (!IBuildTarget::initialize())
		return false;

	Path::sanitize(m_schema);

	if (!m_state.replaceVariablesInString(m_schema, this))
		return false;

	const auto globMessage = "Check that they exist and glob patterns can be resolved";
	if (!processEachPathList(std::move(m_files), [this](std::string&& inValue) {
			return Commands::addPathToListWithGlob(std::move(inValue), m_files, GlobMatch::FilesAndFolders);
		}))
	{
		Diagnostic::error("There was a problem resolving the files to validate for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ValidationBuildTarget::validate()
{
	if (m_schema.empty() || !Commands::pathExists(m_schema))
	{
		Diagnostic::error("Schema file for the validation target '{}' doesn't exist: {}", this->name(), m_schema);
		return false;
	}

	for (auto& file : m_files)
	{
		if (file.empty() || !Commands::pathExists(file))
		{
			Diagnostic::error("File for the validation target '{}' doesn't exist: {}", this->name(), file);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string ValidationBuildTarget::getHash() const
{
	auto files = String::join(m_files);

	auto hashable = Hash::getHashableString(this->name(), m_schema, files);

	return Hash::string(hashable);
}

/*****************************************************************************/
const std::string& ValidationBuildTarget::schema() const noexcept
{
	return m_schema;
}

void ValidationBuildTarget::setSchema(std::string&& inValue) noexcept
{
	m_schema = std::move(inValue);
}

/*****************************************************************************/
const StringList& ValidationBuildTarget::files() const noexcept
{
	return m_files;
}

void ValidationBuildTarget::addFiles(StringList&& inList)
{
	List::forEach(inList, this, &ValidationBuildTarget::addFile);
}

void ValidationBuildTarget::addFile(std::string&& inValue)
{
	m_files.emplace_back(std::move(inValue));
}
}