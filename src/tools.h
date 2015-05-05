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

  Modified by Glenn Washburn <gwashburn@Crossroads.com>
     (added support for extended attributes.)
*/
#ifndef __TOOLS__H__
#define __TOOLS__H__

#include <stdint.h>
#include <pthread.h>

#include "flist.h"

int get_free_dir(void);
char * create_path(const char *dir, const char * file);
char * find_path(const char *file);
int find_path_id(const char *file);

int create_parent_dirs(int dir_id, const char *path);
int copy_xattrs(const char *from, const char *to);


// true if success
int move_file(struct flist * file, off_t size);


// paths
char * get_parent_path(const char *path);
char * get_base_name(const char *path);


// others
int dir_is_empty(const char *path);

#define MOVE_BLOCK_SIZE     32768

#endif
