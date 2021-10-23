/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_INITIALIZER_HPP
#define CHALET_PROJECT_INITIALIZER_HPP

namespace chalet
{
struct BuildJsonProps;
struct CommandLineInputs;

class ProjectInitializer
{
public:
	explicit ProjectInitializer(const CommandLineInputs& inInputs);

	bool run();

private:
	bool doRun(const BuildJsonProps& inProps);

	bool makeBuildJson(const BuildJsonProps& inProps);
	bool makeMainCpp(const BuildJsonProps& inProps);
	bool makePch(const BuildJsonProps& inProps);
	bool makeGitIgnore();
	bool makeReadme();
	bool makeDotEnv();

	std::string getBannerV1() const;
	std::string getBannerV2() const;

	const CommandLineInputs& m_inputs;

	std::string m_rootPath;
};
}

#endif // CHALET_PROJECT_INITIALIZER_HPP
