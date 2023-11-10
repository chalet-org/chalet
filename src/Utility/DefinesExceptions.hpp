/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND))
	#ifndef CHALET_EXCEPTIONS
		#define CHALET_EXCEPTIONS
	#endif

	#define CHALET_THROW(exception) throw exception
	#define CHALET_TRY try
	#define CHALET_CATCH(exception) catch (exception)
	#define CHALET_EXCEPT_ERROR(what) Diagnostic::fatalErrorFromException(what);
#else
	#include <cstdlib>
	#define CHALET_THROW(exception) std::abort()
	#define CHALET_TRY if (true)
	#define CHALET_CATCH(exception) if (false)
	#define CHALET_EXCEPT_ERROR(what)
#endif
