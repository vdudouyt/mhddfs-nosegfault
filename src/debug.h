#ifndef __MHDD_DEBUG_H__
#define __MHDD_DEBUG_H__

#define MHDD_DEFAULT_DEBUG_LEVEL 2

#define MHDD_DEBUG  0
#define MHDD_INFO   1
#define MHDD_MSG    2


int mhdd_debug(int level, const char *fmt, ...);
void mhdd_debug_init(void);

#endif
