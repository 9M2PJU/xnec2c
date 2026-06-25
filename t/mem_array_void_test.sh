#!/bin/sh
# Compile-fail guard for the mem_array_* void element type assertion.
#
# The pass fixture (void * element, ie void **vec) must compile; the fail
# fixture (void element, ie void *buf) must not, and its diagnostic must
# name the void element type. The silent sizeof(void) == 1 path leaves no
# runtime artifact, so the guard is proven only at compile time, with the
# configured compiler exported through AM_TESTS_ENVIRONMENT.

CC="${CC:-cc}"
inc="-I${srcdir}/../src"
pass="${srcdir}/src/mem_array_void_pass.c"
fail="${srcdir}/src/mem_array_void_fail.c"

if ! "$CC" -std=gnu11 $inc -fsyntax-only "$pass"; then
	echo "FAIL: pass fixture (void ** element) did not compile"
	exit 1
fi

err=$("$CC" -std=gnu11 $inc -fsyntax-only "$fail" 2>&1)
if [ $? -eq 0 ]; then
	echo "FAIL: fail fixture (void element) compiled but the guard must reject it"
	exit 1
fi

case "$err" in
	*"void element type"*)
		echo "PASS: void element type rejected at compile time"
		exit 0
		;;
	*)
		echo "FAIL: fail fixture rejected without the void element diagnostic:"
		echo "$err"
		exit 1
		;;
esac
