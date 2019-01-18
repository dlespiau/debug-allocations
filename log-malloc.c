/*
 * Building: gcc --shared -nostartfiles -fPIC  -g log-malloc.c -o log-malloc.so -lpthread -ldl
 * Usage: LD_PRELOAD=./log-malloc.so command args ...
 * Homepage: http://www.brokestream.com/wordpress/category/projects/log-malloc/
 * Version : 2007-06-01
 *
 * Ivan Tikhonov, http://www.brokestream.com, kefeer@netangels.ru
 * Damien Lespiau, https://github.com/dlespiau/debug-allocations, damien.lespiau@gmail.com
 *
 * Changes:
 *   2019-01-17: Port to 64-bit, use glibc backtrace()
 *   2007-06-01: pthread safety patch for Dan Carpenter
 *
 * Notes:
 *   If you want redirect output to file run:
 *   LD_PRELOAD=./log-malloc.so command args ... 200>filename
 */

/*
  Copyright (C) 2007 Ivan Tikhonov
  Copyright (C) 2019 Damien Lespiau

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#include <stdio.h>
#include <unistd.h>
#include <execinfo.h>
#include <pthread.h>

#define __USE_GNU
#include <dlfcn.h>

static void *(*real_malloc) (size_t size) = 0;
static void (*real_free) (void *ptr) = 0;
static void *(*real_realloc) (void *ptr, size_t size) = 0;
static void *(*real_calloc) (size_t nmemb, size_t size) = 0;
static FILE *memlog = 0;
static int memlog_fd = 200;

static pthread_mutex_t loglock;
#define LOCK (pthread_mutex_lock(&loglock));
#define UNLOCK (pthread_mutex_unlock(&loglock));

static pthread_mutex_t g_backtrace_mutex;
static pthread_mutexattr_t g_mutex_attr;

static void init_me()
{
	void *(*dummy);

	if (memlog)
		return;
	memlog = fdopen(memlog_fd, "w");
	if (!memlog) {
		memlog = stderr;
		memlog_fd = 2;
	}

	pthread_mutex_init(&loglock, 0);

	real_malloc = dlsym(RTLD_NEXT, "malloc");
	real_calloc = dlsym(RTLD_NEXT, "calloc");
	real_free = dlsym(RTLD_NEXT, "free");
	real_realloc = dlsym(RTLD_NEXT, "realloc");

	pthread_mutexattr_init(&g_mutex_attr);
	pthread_mutexattr_settype(&g_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&g_backtrace_mutex, &g_mutex_attr);
}

void
    __attribute__ ((constructor))
    _init(void)
{
	init_me();
}

static __thread int in_bt;	// thread local variable protected by loglock.

static void print_stack()
{
	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;

	in_bt = 1;

	trace_size = backtrace(trace, 16);
	messages = backtrace_symbols(trace, trace_size);
	printf("[bt] Execution path:\n");
	backtrace_symbols_fd(trace, trace_size, memlog_fd);
/*
  for (i=0; i < trace_size; ++i)
        printf("[bt] %s\n", messages[i]);
*/

	in_bt = 0;
}

void *malloc(size_t size)
{
	void *p;
	if (!real_malloc) {
		if (!real_malloc)
			return NULL;
	}

	p = real_malloc(size);

	if (in_bt) {
		return p;
	}

	if (memlog) {
		LOCK;

		fprintf(memlog, "malloc %zu %16p ", size, p);
		print_stack();
		fprintf(memlog, "\n");
		UNLOCK;
	}

	return p;
}

void *calloc(size_t nmemb, size_t size)
{
	void *p;
	if (!real_calloc) {
		if (!real_calloc)
			return NULL;
		real_calloc = dlsym(RTLD_NEXT, "calloc");
		return NULL;
	}

	p = real_calloc(nmemb, size);

	if (in_bt) {
		return p;
	}

	if (memlog) {
		LOCK;
		fprintf(memlog, "calloc %zu %zu %16p ", nmemb, size, p);
		print_stack();
		fprintf(memlog, "\n");
		UNLOCK;
	}

	return p;
}

void free(void *ptr)
{
	if (!real_free) {
		if (!real_free)
			return;
	}
	real_free(ptr);
	if (in_bt) {
		return;
	}
	if (memlog) {
		LOCK;
		fprintf(memlog, "free %16p ", ptr);
		print_stack();
		fprintf(memlog, "\n");
		UNLOCK;
	}
}

void *realloc(void *ptr, size_t size)
{
	void *p;
	if (!real_realloc) {
		if (!real_realloc)
			return NULL;
	}
	p = real_realloc(ptr, size);
	if (in_bt) {
		return p;
	}
	if (memlog) {
		LOCK;
		fprintf(memlog, "realloc %16p %zu %16p ", ptr, size, p);
		print_stack();
		fprintf(memlog, "\n");
		UNLOCK;
	}
	return p;
}
