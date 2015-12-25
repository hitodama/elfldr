#ifndef Debug_H
#define Debug_H

#if defined(DEBUG) || defined(Debug)
	#define debugPrint fprintf
#else
	#define debugPrint(...)
#endif

#endif
