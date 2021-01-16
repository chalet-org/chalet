/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILE_TOOLCHAIN_HPP
#define CHALET_ICOMPILE_TOOLCHAIN_HPP

#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
struct ICompileToolchain
{
	virtual ~ICompileToolchain() = default;

	virtual ToolchainType type() const = 0;

	virtual std::string getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile) = 0;
	virtual std::string getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) = 0;
	virtual std::string getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) = 0;
	virtual std::string getCppCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) = 0;
	virtual std::string getObjcppCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const bool treatAsC) = 0;
	virtual std::string getLinkerTargetCommand(const std::string& outputFile, const std::string& sourceObjs, const std::string& outputFileBase) = 0;
};

using CompileToolchain = std::unique_ptr<ICompileToolchain>;
}

#endif // CHALET_ICOMPILE_TOOLCHAIN_HPP
