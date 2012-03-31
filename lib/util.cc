// -*- c-basic-offset: 2 default-tab-width: 2 indent-tabs-mode: t -*-
// vim: autoindent tabstop=2 noexpandtab shiftwidth=2 softtabstop=2
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

static void va_echo(const char* msg, const char* fmt, va_list va) {
	uint64_t n = 3 + strlen(msg);
	char buf[n];

	snprintf(buf, n, "[%s]", msg);
	fprintf(stderr, "%-15s", buf);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
}

void do_echo(const char* msg, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	va_echo(msg, fmt, va);
	va_end(va);
}

extern void do_fatal(const char* fn, uint64_t ln, const char* fct, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	va_echo("error", fmt, va);
	fprintf(stderr, "   at %s::"WORD" (%s)\n", fn, ln, fct);
	va_end(va);
	exit(42);
}


