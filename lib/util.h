// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2
#ifndef __UTIL_H
#define __UTIL_H

# define WORD	"%" PRIu64
# define XWORD	"%" PRIx64

#include "vmkit/Locks.h"
#include "vmkit/System.h"
#include "VmkitGC.h"
#include "vmkit/Allocator.h"
#include "config.h"

#define GC_VAR(__type, __var)										\
  __type __var = NULL;													\
  llvm_gcroot(__var, 0)

#define GC_PARAM(__var)													\
  llvm_gcroot(__var, 0)

#define GC_SELF(T)															\
  T* self = this;																\
  llvm_gcroot(self, 0)

#if defined(DO_DEBUG)
# define ddo(...) __VA_ARGS__
#else
# define ddo(...)
#endif

extern void do_echo(const char* msg, const char* fmt, ...);
extern void do_fatal(const char* fn, uint64_t ln, const char* fct, const char* fmt, ...) __attribute__((noreturn));

#define warning(...)														\
  do_echo("warning", __VA_ARGS__)

#define fatal(...)																								\
  do_fatal(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

#define error_on(__test, ...)																\
  if(__test)																								\
		fatal("assertion failed '" #__test "':\t" __VA_ARGS__)

#define debug_printf(...)														\
  ddo(do_echo("info",    __VA_ARGS__))
#define Assert(__test, ...)											\
  ddo(error_on(!(__test), __VA_ARGS__))

#define nyi_fatal()															\
  fatal("not yet implemented")

#define nyi()																														\
  ({ static int z_nyi=0; if(!z_nyi) { z_nyi=1; fprintf(stderr, "Not yet implemented at %s::%d (%s)\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); }}) 

#endif
