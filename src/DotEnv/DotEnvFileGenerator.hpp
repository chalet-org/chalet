/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct DotEnvFileGenerator
{
	DotEnvFileGenerator() = default;

	static std::unique_ptr<DotEnvFileGenerator> make(const BuildState& inState);

	void set(const std::string& inKey, const std::string& inValue);
	void setRunPaths(const std::string& inValue);

	std::string get(const std::string& inKey) const;
	std::string getPath() const;
	std::string getRunPaths() const;
	std::string getLibraryPath() const;
	std::string getFrameworkPath() const;

	const Dictionary<std::string>& variables() const noexcept;

	bool save(const std::string& inFilename) const;

private:
	Dictionary<std::string> m_variables;
};
}
