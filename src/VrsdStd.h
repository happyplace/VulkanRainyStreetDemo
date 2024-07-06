#ifndef VRSD_VrsdStd_h_
#define VRSD_VrsdStd_h_

#ifndef VRSD_ALIGN
	#ifdef __GNUC__
		#define VRSD_ALIGN(x) __attribute__((aligned(x)))
	#elif _WIN32
		#define VRSD_ALIGN(x) __declspec(align(x))
	#else
		#error not supported on compiler
	#endif // __GNUC__
#endif // VRSD_ALIGN

#endif // VRSD_VrsdStd_h_
