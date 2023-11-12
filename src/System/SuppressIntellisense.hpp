/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#ifndef __INTELLISENSE__
	#define __INTELLISENSE__ 0
#endif

// This fixes "user-defined literal not found"

#if __INTELLISENSE__
	#pragma diag_suppress 2486
#endif
