/*
	mhddfs - Multi HDD [FUSE] File System
	Copyright (C) 2008 Dmitry E. Oboukhov <dimka@avanto.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef __PARSE__OPTIONS__H__
#define __PARSE__OPTIONS__H__

#include <stdio.h>
#include <sys/types.h>
#include <fuse.h>

struct mhdd_config
{
	char *  mount;    // mount point
	char ** dirs;     // dir list

	int  cdirs;       // count dirs in dirs

	off_t move_limit; // no limits

	FILE  *debug;
	char  *debug_file;

	char  *mlimit_str;  // mlimit string

	int   loglevel;
};

extern struct mhdd_config mhdd;

struct fuse_args * parse_options(int argc, char *argv[]);

#endif
