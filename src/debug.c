#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include "debug.h"
#include "parse_options.h"

static pthread_mutex_t debug_lock;

void mhdd_debug_init(void)
{
	pthread_mutex_init(&debug_lock, 0);
}

int mhdd_debug(int level, const char *fmt, ...)
{
	if (level<mhdd.loglevel) return 0;
	if (!mhdd.debug) return 0;


	char tstr[64];
	time_t t=time(0);
	struct tm *lt;
	lt=localtime(&t);


	strftime(tstr, 64, "%Y-%m-%d %H:%M:%S", lt);

	pthread_mutex_lock(&debug_lock);
	fprintf(mhdd.debug, "mhddfs [%s]", tstr);

	switch(level)
	{
		case MHDD_DEBUG: fprintf(mhdd.debug, " (debug): "); break;
		case MHDD_INFO:  fprintf(mhdd.debug, " (info): ");  break;
		default:         fprintf(mhdd.debug, ": ");  break;
	}

	fprintf(mhdd.debug, "[%ld] ", pthread_self());

	va_list ap;
	va_start(ap, fmt);
	int res=vfprintf(mhdd.debug, fmt, ap);
	va_end(ap);
	pthread_mutex_unlock(&debug_lock);
	return res;
}

