/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Convert/BuildFileConverter.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
BuildFileConverter::BuildFileConverter(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool BuildFileConverter::convertFromInputs()
{
	auto& inputFile = m_inputs.inputFile();
	auto& format = m_inputs.settingsKey();

	return convert(format, inputFile);
}

/*****************************************************************************/
bool BuildFileConverter::convert(const std::string& format, const std::string& inputFile)
{
	auto formats = m_inputs.getConvertFormatPresets();
	if (!String::equals(formats, format))
	{
		Diagnostic::error("Unsupported project format requested: {}", format);
		return false;
	}

	if (!Files::pathExists(inputFile))
	{
		Diagnostic::error("Project file does not exist: {}", inputFile);
		return false;
	}

	if (String::endsWith(fmt::format(".{}", format), inputFile))
	{
		Diagnostic::error("Project file '{}' already has the format: {}", inputFile, format);
		return false;
	}

	// Right now, this loads all formats and treats them as JSON
	JsonFile file(inputFile);
	if (!file.load())
	{
		Diagnostic::error("There was a problem loading the file: {}", inputFile);
		return false;
	}

	if (!file.validate(ChaletJsonSchema::get(m_inputs)))
	{
		Diagnostic::error("There was a problem validating the file for the new format: {}", inputFile);
		return false;
	}

	auto outputFile = fmt::format("{}.{}", String::getPathFolderBaseName(inputFile), format);
	if (!file.saveAs(outputFile))
	{
		Diagnostic::error("There was a problem saving the new format: {}", outputFile);
		return false;
	}

	const auto color = Output::getAnsiStyle(Output::theme().build);
	const auto flair = Output::getAnsiStyle(Output::theme().flair);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	auto output = fmt::format("   {} {}\u2192 {}{}{}\n", inputFile, flair, color, outputFile, reset);
	std::cout.write(output.data(), output.size());
	std::cout.flush();

	return true;
}
}
