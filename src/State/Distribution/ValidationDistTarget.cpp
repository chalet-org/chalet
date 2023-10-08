/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/ValidationDistTarget.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ValidationDistTarget::ValidationDistTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::Validation)
{
}

/*****************************************************************************/
bool ValidationDistTarget::initialize()
{
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
bool ValidationDistTarget::validate()
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
const std::string& ValidationDistTarget::schema() const noexcept
{
	return m_schema;
}

void ValidationDistTarget::setSchema(std::string&& inValue) noexcept
{
	m_schema = std::move(inValue);
}

/*****************************************************************************/
const StringList& ValidationDistTarget::files() const noexcept
{
	return m_files;
}

void ValidationDistTarget::addFiles(StringList&& inList)
{
	List::forEach(inList, this, &ValidationDistTarget::addFile);
}

void ValidationDistTarget::addFile(std::string&& inValue)
{
	m_files.emplace_back(std::move(inValue));
}

}
