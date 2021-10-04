/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LIST_PRINTER_HPP
#define CHALET_LIST_PRINTER_HPP

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;

struct ListPrinter
{
	ListPrinter(const CommandLineInputs& inInputs, const StatePrototype& inPrototype);

	bool printListOfRequestedType();

private:
	StringList getArchitectures() const;

	const CommandLineInputs& m_inputs;
	const StatePrototype& m_prototype;
};
}

#endif // CHALET_LIST_PRINTER_HPP
