/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUPPRESS_INTELLISENSE_HPP
#define CHALET_SUPPRESS_INTELLISENSE_HPP

#ifndef __INTELLISENSE__
	#define __INTELLISENSE__ 0
#endif

// This fixes "user-defined literal not found"

#if __INTELLISENSE__
	#pragma diag_suppress 2486
#endif

#endif // CHALET_SUPPRESS_INTELLISENSE_HPP
