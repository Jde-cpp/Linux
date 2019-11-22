#pragma once

#ifdef JDE_LINUX_EXPORTS
	#define JDE_LINUX_EXPORT __attribute__((visibility("default")))
#else 
	#define JDE_LINUX_EXPORT
#endif