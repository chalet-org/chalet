/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DOT_ENV_FILE_GENERATOR_HPP
#define CHALET_DOT_ENV_FILE_GENERATOR_HPP

namespace chalet
{
class BuildState;

struct DotEnvFileGenerator
{
	DotEnvFileGenerator() = default;

	static DotEnvFileGenerator make(const BuildState& inState);

	void set(const std::string& inKey, const std::string& inValue);
	void setRunPaths(const std::string& inValue);

	std::string get(const std::string& inKey) const;
	std::string getPath() const;
	std::string getRunPaths() const;
	std::string getLibraryPath() const;
	std::string getFrameworkPath() const;
	const char* getLibraryPathKey() const;
	const char* getFrameworkPathKey() const;

	bool save(const std::string& inFilename);

private:
	Dictionary<std::string> m_variables;
};
}

#endif // CHALET_DOT_ENV_FILE_GENERATOR_HPP
