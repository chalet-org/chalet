/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_INITIALIZER_HPP
#define CHALET_PROJECT_INITIALIZER_HPP

#include "Core/CommandLineInputs.hpp"
#include "Init/BuildJsonProps.hpp"

namespace chalet
{
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

	const CommandLineInputs& m_inputs;

	std::string m_rootPath;
};
}

#endif // CHALET_PROJECT_INITIALIZER_HPP
