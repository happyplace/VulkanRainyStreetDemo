#pragma once

#if defined(KIWI_USE_SIMPLE_STDATOMIC)
	#include "../external/simple-stdatomic-for-VS-Clang/stdatomic.h"
#else
	#if defined(__STDC_NO_ATOMICS__)
		#error this compiler does not support C11 atomics and can not run KiwiTasker
	#endif

	#include <stdatomic.h>
#endif
