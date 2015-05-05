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
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE 1
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#ifndef WITHOUT_XATTR
#include <attr/xattr.h>
#endif

#include "parse_options.h"
#include "tools.h"

#include "debug.h"

#include <uthash.h>

void save_backtrace(int sig)
{
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d\n", sig);
  FILE *log = fopen("/tmp/mhddfs_backtrace.log", "w");
  backtrace_symbols_fd(array, size, fileno(log));
  fclose(log);
  exit(1);
}

// getattr
static int mhdd_stat(const char *file_name, struct stat *buf)
{
	mhdd_debug(MHDD_MSG, "mhdd_stat: %s\n", file_name);
	char *path = find_path(file_name);
	if (path) {
		int ret = lstat(path, buf);
		free(path);
		if (ret == -1) return -errno;
		return 0;
	}
	errno = ENOENT;
	return -errno;
}

//statvfs
static int mhdd_statfs(const char *path, struct statvfs *buf)
{
	int i, j;
	struct statvfs * stats;
	struct stat st;
	dev_t * devices;

	mhdd_debug(MHDD_MSG, "mhdd_statfs: %s\n", path);

	stats = calloc(mhdd.cdirs, sizeof(struct statvfs));
	devices = calloc(mhdd.cdirs, sizeof(dev_t));

	for (i = 0; i < mhdd.cdirs; i++) {
		int ret = statvfs(mhdd.dirs[i], stats+i);
		if (ret != 0) {
			free(stats);
			free(devices);
			return -errno;
		}

		ret = stat(mhdd.dirs[i], &st);
		if (ret != 0) {
			free(stats);
			free(devices);
			return -errno;
		}
		devices[i] = st.st_dev;
	}

	unsigned long
		min_block = stats[0].f_bsize,
		min_frame = stats[0].f_frsize;

	for (i = 1; i<mhdd.cdirs; i++) {
		if (min_block>stats[i].f_bsize) min_block = stats[i].f_bsize;
		if (min_frame>stats[i].f_frsize) min_frame = stats[i].f_frsize;
	}

	if (!min_block)
		min_block = 512;
	if (!min_frame)
		min_frame = 512;

	for (i = 0; i < mhdd.cdirs; i++) {
		if (stats[i].f_bsize>min_block) {
			stats[i].f_bfree    *=  stats[i].f_bsize/min_block;
			stats[i].f_bavail   *=  stats[i].f_bsize/min_block;
			stats[i].f_bsize    =   min_block;
		}
		if (stats[i].f_frsize>min_frame) {
			stats[i].f_blocks   *=  stats[i].f_frsize/min_frame;
			stats[i].f_frsize   =   min_frame;
		}
	}

	memcpy(buf, stats, sizeof(struct statvfs));

	for (i = 1; i<mhdd.cdirs; i++) {

		/* if the device already processed, skip it */
		if (devices[i]) {
			int dup_found = 0;
			for (j = 0; j < i; j++) {
				if (devices[j] == devices[i]) {
					dup_found = 1;
					break;
				}
			}

			if (dup_found)
				continue;
		}

		if (buf->f_namemax<stats[i].f_namemax) {
			buf->f_namemax = stats[i].f_namemax;
		}
		buf->f_ffree  +=  stats[i].f_ffree;
		buf->f_files  +=  stats[i].f_files;
		buf->f_favail +=  stats[i].f_favail;
		buf->f_bavail +=  stats[i].f_bavail;
		buf->f_bfree  +=  stats[i].f_bfree;
		buf->f_blocks +=  stats[i].f_blocks;
	}

	free(stats);
	free(devices);
	return 0;
}

static int mhdd_readdir(
		const char *dirname,
		void *buf,
		fuse_fill_dir_t filler,
		off_t offset,
		struct fuse_file_info * fi)
{
	int i, j, found;

	mhdd_debug(MHDD_MSG, "mhdd_readdir: %s\n", dirname);
	char **dirs = (char **) calloc(mhdd.cdirs+1, sizeof(char *));

	typedef struct dir_item {
		char            *name;
		struct stat     *st;
		UT_hash_handle   hh;
	} dir_item;

	dir_item * items_ht = NULL;


	struct stat st;

	// find all dirs
	for(i = j = found = 0; i<mhdd.cdirs; i++) {
		char *path = create_path(mhdd.dirs[i], dirname);
		if (stat(path, &st) == 0) {
			found++;
			if (S_ISDIR(st.st_mode)) {
				dirs[j] = path;
				j++;
				continue;
			}
		}
		free(path);
	}

	// dirs not found
	if (dirs[0] == 0) {
		errno = ENOENT;
		if (found) errno = ENOTDIR;
		free(dirs);
		return -errno;
	}

	// read directories
	for (i = 0; dirs[i]; i++) {
		struct dirent *de;
		DIR * dh = opendir(dirs[i]);
		if (!dh)
			continue;

		while((de = readdir(dh))) {
			// find dups
			
			struct dir_item *prev;

			HASH_FIND_STR(items_ht, de->d_name, prev);

			if (prev) {
				continue;
			}

			// add item
			char *object_name = create_path(dirs[i], de->d_name);
			struct dir_item *new_item =
				calloc(1, sizeof(struct dir_item));

			new_item->name = strdup(de->d_name);
			new_item->st = calloc(1, sizeof(struct stat));
			lstat(object_name, new_item->st);

			HASH_ADD_KEYPTR(
				hh,
				items_ht,
				new_item->name,
				strlen(new_item->name),
				new_item
			);
			free(object_name);
		}

		closedir(dh);
	}

	dir_item *item, *tmp;

	// fill list
	HASH_ITER(hh, items_ht, item, tmp) {
		if (filler(buf, item->name, item->st, 0))
			break;
	}

	// free memory
	HASH_ITER(hh, items_ht, item, tmp) {
		free(item->name);
		free(item->st);
		free(item);
	}
	HASH_CLEAR(hh, items_ht);

	for (i = 0; dirs[i]; i++)
		free(dirs[i]);
	free(dirs);
	return 0;
}

// readlink
static int mhdd_readlink(const char *path, char *buf, size_t size)
{
	mhdd_debug(MHDD_MSG, "mhdd_readlink: %s, size = %d\n", path, size);

	char *link = find_path(path);
	if (link) {
		memset(buf, 0, size);
		int res = readlink(link, buf, size);
		free(link);
		if (res >= 0)
			return 0;
	}
	return -1;
}

#define CREATE_FUNCTION 0
#define OPEN_FUNCION    1
// create or open
static int mhdd_internal_open(const char *file,
		mode_t mode, struct fuse_file_info *fi, int what)
{
	mhdd_debug(MHDD_INFO, "mhdd_internal_open: %s, flags = 0x%X\n",
		file, fi->flags);
	int dir_id, fd;

	char *path = find_path(file);

	if (path) {
		if (what == CREATE_FUNCTION)
			fd = open(path, fi->flags, mode);
		else
			fd = open(path, fi->flags);
		if (fd == -1) {
			free(path);
			return -errno;
		}
		struct flist *add = flist_create(file, path, fi->flags, fd);
		fi->fh = add->id;
		flist_unlock();
		free(path);
		return 0;
	}

	mhdd_debug(MHDD_INFO, "mhdd_internal_open: new file %s\n", file);

	if ((dir_id = get_free_dir()) < 0) {
		errno = ENOSPC;
		return -errno;
	}

	create_parent_dirs(dir_id, file);
	path = create_path(mhdd.dirs[dir_id], file);

	if (what == CREATE_FUNCTION)
		fd = open(path, fi->flags, mode);
	else
		fd = open(path, fi->flags);

	if (fd == -1) {
		free(path);
		return -errno;
	}

	if (getuid() == 0) {
		struct stat st;
		gid_t gid = fuse_get_context()->gid;
		if (fstat(fd, &st) == 0) {
			/* parent directory is SGID'ed */
			if (st.st_gid != getgid()) gid = st.st_gid;
		}
		fchown(fd, fuse_get_context()->uid, gid);
	}
	struct flist *add = flist_create(file, path, fi->flags, fd);
	fi->fh = add->id;
	flist_unlock();
	free(path);
	return 0;
}

// create
static int mhdd_create(const char *file,
		mode_t mode, struct fuse_file_info *fi)
{
	mhdd_debug(MHDD_MSG, "mhdd_create: %s, mode = %X\n", file, fi->flags);
	int res = mhdd_internal_open(file, mode, fi, CREATE_FUNCTION);
	if (res != 0)
		mhdd_debug(MHDD_INFO,
			"mhdd_create: error: %s\n", strerror(-res));
	return res;
}

// open
static int mhdd_fileopen(const char *file, struct fuse_file_info *fi)
{
	mhdd_debug(MHDD_MSG,
		"mhdd_fileopen: %s, flags = %04X\n", file, fi->flags);
	int res = mhdd_internal_open(file, 0, fi, OPEN_FUNCION);
	if (res != 0)
		mhdd_debug(MHDD_INFO,
			"mhdd_fileopen: error: %s\n", strerror(-res));
	return res;
}

// close
static int mhdd_release(const char *path, struct fuse_file_info *fi)
{
	struct flist *del;
	int fh;

	mhdd_debug(MHDD_MSG, "mhdd_release: %s, handle = %lld\n", path, fi->fh);
	del = flist_item_by_id_wrlock(fi->fh);
	if (!del) {
		mhdd_debug(MHDD_INFO,
			"mhdd_release: unknown file number: %llu\n", fi->fh);
		errno = EBADF;
		return -errno;
	}

	fh = del->fh;
	flist_delete_wrlocked(del);
	close(fh);
	return 0;
}

// read
static int mhdd_read(const char *path, char *buf, size_t count, off_t offset,
		struct fuse_file_info *fi)
{
	ssize_t res;
	struct flist * info;
	mhdd_debug(MHDD_INFO, "mhdd_read: %s, offset = %lld, count = %lld\n",
			path,
			(long long)offset,
			(long long)count
		  );
	info = flist_item_by_id(fi->fh);
	if (!info) {
		errno = EBADF;
		return -errno;
	}
	res = pread(info->fh, buf, count, offset);
	flist_unlock();
	if (res == -1)
		return -errno;
	return res;
}

// write
static int mhdd_write(const char *path, const char *buf, size_t count,
		off_t offset, struct fuse_file_info *fi)
{
	ssize_t res;
	struct flist *info;
	mhdd_debug(MHDD_INFO, "mhdd_write: %s, handle = %lld\n", path, fi->fh);
	info = flist_item_by_id(fi->fh);

	if (!info) {
		errno = EBADF;
		return -errno;
	}

	res = pwrite(info->fh, buf, count, offset);
	if ((res == count) || (res == -1 && errno != ENOSPC)) {
		flist_unlock();
		if (res == -1) {
			mhdd_debug(MHDD_DEBUG,
				"mhdd_write: error write %s: %s\n",
				info->real_name, strerror(errno));
			return -errno;
		}
		return res;
	}

	// end free space
	if (move_file(info, offset + count) == 0) {
		res = pwrite(info->fh, buf, count, offset);
		flist_unlock();
		if (res == -1) {
			mhdd_debug(MHDD_DEBUG,
				"mhdd_write: error restart write: %s\n",
				strerror(errno));
			return -errno;
		}
		if (res < count) {
			mhdd_debug(MHDD_DEBUG,
				"mhdd_write: error (re)write file %s %s\n",
				info->real_name,
				strerror(ENOSPC));
		}
		return res;
	}
	errno = ENOSPC;
	flist_unlock();
	return -errno;
}

// truncate
static int mhdd_truncate(const char *path, off_t size)
{
	char *file = find_path(path);
	mhdd_debug(MHDD_MSG, "mhdd_truncate: %s\n", path);
	if (file) {
		int res = truncate(file, size);
		free(file);
		if (res == -1)
			return -errno;
		return 0;
	}
	errno = ENOENT;
	return -errno;
}

// ftrucate
static int mhdd_ftruncate(const char *path, off_t size,
		struct fuse_file_info *fi)
{
	int res;
	struct flist *info;
	mhdd_debug(MHDD_MSG,
		"mhdd_ftruncate: %s, handle = %lld\n", path, fi->fh);
	info = flist_item_by_id(fi->fh);

	if (!info) {
		errno = EBADF;
		return -errno;
	}

	int fh = info->fh;
	res = ftruncate(fh, size);
	flist_unlock();
	if (res == -1)
		return -errno;
	return 0;
}

// access
static int mhdd_access(const char *path, int mask)
{
	mhdd_debug(MHDD_MSG, "mhdd_access: %s mode = %04X\n", path, mask);
	char *file = find_path(path);
	if (file) {
		int res = access(file, mask);
		free(file);
		if (res == -1)
			return -errno;
		return 0;
	}

	errno = ENOENT;
	return -errno;
}

// mkdir
static int mhdd_mkdir(const char * path, mode_t mode)
{
	mhdd_debug(MHDD_MSG, "mhdd_mkdir: %s mode = %04X\n", path, mode);

	if (find_path_id(path) != -1) {
		errno = EEXIST;
		return -errno;
	}

	char *parent = get_parent_path(path);
	if (!parent) {
		errno = EFAULT;
		return -errno;
	}

	if (find_path_id(parent) == -1) {
		free(parent);
		errno = EFAULT;
		return -errno;
	}
	free(parent);

	int dir_id = get_free_dir();
	if (dir_id<0) {
		errno = ENOSPC;
		return -errno;
	}

	create_parent_dirs(dir_id, path);
	char *name = create_path(mhdd.dirs[dir_id], path);
	if (mkdir(name, mode) == 0) {
		if (getuid() == 0) {
			struct stat st;
			gid_t gid = fuse_get_context()->gid;
			if (lstat(name, &st) == 0) {
				/* parent directory is SGID'ed */
				if (st.st_gid != getgid())
					gid = st.st_gid;
			}
			chown(name, fuse_get_context()->uid, gid);
		}
		free(name);
		return 0;
	}
	free(name);
	return -errno;
}

// rmdir
static int mhdd_rmdir(const char * path)
{
	mhdd_debug(MHDD_MSG, "mhdd_rmdir: %s\n", path);
	char *dir;
	while((dir = find_path(path))) {
		int res = rmdir(dir);
		free(dir);
		if (res == -1) return -errno;
	}
	return 0;
}

// unlink
static int mhdd_unlink(const char *path)
{
	mhdd_debug(MHDD_MSG, "mhdd_unlink: %s\n", path);
	char *file = find_path(path);
	if (!file) {
		errno = ENOENT;
		return -errno;
	}
	int res = unlink(file);
	free(file);
	if (res == -1) return -errno;
	return 0;
}

// rename
static int mhdd_rename(const char *from, const char *to)
{
	mhdd_debug(MHDD_MSG, "mhdd_rename: from = %s to = %s\n", from, to);

	int i, res;
	struct stat sto, sfrom;
	char *obj_from, *obj_to;
	int from_is_dir = 0, to_is_dir = 0, from_is_file = 0, to_is_file = 0;
	int to_dir_is_empty = 1;

	if (strcmp(from, to) == 0)
		return 0;

	/* seek for possible errors */
	for (i = 0; i < mhdd.cdirs; i++) {
		obj_to   = create_path(mhdd.dirs[i], to);
		obj_from = create_path(mhdd.dirs[i], from);
		if (stat(obj_to, &sto) == 0) {
			if (S_ISDIR(sto.st_mode)) {
				to_is_dir++;
				if (!dir_is_empty(obj_to))
					to_dir_is_empty = 0;
			}
			else
				to_is_file++;
		}
		if (stat(obj_from, &sfrom) == 0) {
			if (S_ISDIR (sfrom.st_mode))
				from_is_dir++;
			else
				from_is_file++;
		}
		free(obj_from);
		free(obj_to);

		if (to_is_file && from_is_dir)
			return -ENOTDIR;
		if (to_is_file && to_is_dir)
			return -ENOTEMPTY;
		if (from_is_dir && !to_dir_is_empty)
			return -ENOTEMPTY;
	}

	/* parent 'to' path doesn't exists */
	char *pto = get_parent_path (to);
	if (find_path_id(pto) == -1) {
		free (pto);
		return -ENOENT;
	}
	free (pto);

	/* rename cycle */
	for (i = 0; i < mhdd.cdirs; i++) {
		obj_to   = create_path(mhdd.dirs[i], to);
		obj_from = create_path(mhdd.dirs[i], from);
		if (stat(obj_from, &sfrom) == 0) {
			/* if from is dir and at the same time file,
			   we only rename dir */
			if (from_is_dir && from_is_file) {
				if (!S_ISDIR(sfrom.st_mode)) {
					free(obj_from);
					free(obj_to);
					continue;
				}
			}

			create_parent_dirs(i, to);

			mhdd_debug(MHDD_MSG, "mhdd_rename: rename %s -> %s\n",
				obj_from, obj_to);
			res = rename(obj_from, obj_to);
			if (res == -1) {
				free(obj_from);
				free(obj_to);
				return -errno;
			}
		} else {
			/* from and to are files, so we must remove to files */
			if (from_is_file && to_is_file && !from_is_dir) {
				if (stat(obj_to, &sto) == 0) {
					mhdd_debug(MHDD_MSG,
						"mhdd_rename: unlink %s\n",
						obj_to);
					if (unlink(obj_to) == -1) {
						free(obj_from);
						free(obj_to);
						return -errno;
					}
				}
			}
		}

		free(obj_from);
		free(obj_to);
	}

	return 0;
}

// .utimens
static int mhdd_utimens(const char *path, const struct timespec ts[2])
{
	mhdd_debug(MHDD_MSG, "mhdd_utimens: %s\n", path);
	int i, res, flag_found;

	for (i = flag_found = 0; i<mhdd.cdirs; i++) {
		char *object = create_path(mhdd.dirs[i], path);
		struct stat st;
		if (lstat(object, &st) != 0) {
			free(object);
			continue;
		}

		flag_found = 1;
		struct timeval tv[2];

		tv[0].tv_sec = ts[0].tv_sec;
		tv[0].tv_usec = ts[0].tv_nsec / 1000;
		tv[1].tv_sec = ts[1].tv_sec;
		tv[1].tv_usec = ts[1].tv_nsec / 1000;

		res = lutimes(object, tv);
		free(object);
		if (res == -1)
			return -errno;
	}
	if (flag_found)
		return 0;
	errno = ENOENT;
	return -errno;
}

// .chmod
static int mhdd_chmod(const char *path, mode_t mode)
{
	mhdd_debug(MHDD_MSG, "mhdd_chmod: mode = 0x%03X %s\n", mode, path);
	int i, res, flag_found;

	for (i = flag_found = 0; i<mhdd.cdirs; i++) {
		char *object = create_path(mhdd.dirs[i], path);
		struct stat st;
		if (lstat(object, &st) != 0) {
			free(object);
			continue;
		}

		flag_found = 1;
		res = chmod(object, mode);
		free(object);
		if (res == -1)
			return -errno;
	}
	if (flag_found)
		return 0;
	errno = ENOENT;
	return -errno;
}

// chown
static int mhdd_chown(const char *path, uid_t uid, gid_t gid)
{
	mhdd_debug(MHDD_MSG,
		"mhdd_chown: pid = 0x%03X gid = %03X %s\n", uid, gid, path);
	int i, res, flag_found;

	for (i = flag_found = 0; i < mhdd.cdirs; i++) {
		char *object = create_path(mhdd.dirs[i], path);
		struct stat st;
		if (lstat(object, &st) != 0) {
			free(object);
			continue;
		}

		flag_found = 1;
		res = lchown(object, uid, gid);
		free(object);
		if (res == -1)
			return -errno;
	}
	if (flag_found)
		return 0;
	errno = ENOENT;
	return -errno;
}

// symlink
static int mhdd_symlink(const char *from, const char *to)
{
	mhdd_debug(MHDD_MSG, "mhdd_symlink: from = %s to = %s\n", from, to);
	int i, res;
	char *parent = get_parent_path(to);
	if (!parent) {
		errno = ENOENT;
		return -errno;
	}

	int dir_id = find_path_id(parent);
	free(parent);

	if (dir_id == -1) {
		errno = ENOENT;
		return -errno;
	}

	for (i = 0; i < 2; i++) {
		if (i) {
			if ((dir_id = get_free_dir()) < 0) {
				errno = ENOSPC;
				return -errno;
			}

			create_parent_dirs(dir_id, to);
		}

		char *path_to = create_path(mhdd.dirs[dir_id], to);

		res = symlink(from, path_to);
		free(path_to);
		if (res == 0)
			return 0;
		if (errno != ENOSPC)
			return -errno;
	}
	return -errno;
}

// link
static int mhdd_link(const char *from, const char *to)
{
	mhdd_debug(MHDD_MSG, "mhdd_link: from = %s to = %s\n", from, to);

	int dir_id = find_path_id(from);

	if (dir_id == -1) {
		errno = ENOENT;
		return -errno;
	}

	int res = create_parent_dirs(dir_id, to);
	if (res != 0) {
		return res;
	}

	char *path_from = create_path(mhdd.dirs[dir_id], from);
	char *path_to = create_path(mhdd.dirs[dir_id], to);

	res = link(path_from, path_to);
	free(path_from);
	free(path_to);

	if (res == 0)
		return 0;
	return -errno;
}

// mknod
static int mhdd_mknod(const char *path, mode_t mode, dev_t rdev)
{
	mhdd_debug(MHDD_MSG, "mhdd_mknod: path = %s mode = %X\n", path, mode);
	int res, i;
	char *nod;

	char *parent = get_parent_path(path);
	if (!parent) {
		errno = ENOENT;
		return -errno;
	}

	int dir_id = find_path_id(parent);
	free(parent);

	if (dir_id == -1) {
		errno = ENOENT;
		return -errno;
	}

	for (i = 0; i < 2; i++) {
		if (i) {
			if ((dir_id = get_free_dir())<0) {
				errno = ENOSPC;
				return -errno;
			}
			create_parent_dirs(dir_id, path);
		}
		nod = create_path(mhdd.dirs[dir_id], path);

		if (S_ISREG(mode)) {
			res = open(nod, O_CREAT | O_EXCL | O_WRONLY, mode);
			if (res >= 0)
				res = close(res);
		} else if (S_ISFIFO(mode))
			res = mkfifo(nod, mode);
		else
			res = mknod(nod, mode, rdev);

		if (res != -1) {
			if (getuid() == 0) {
				struct fuse_context * fcontext =
					fuse_get_context();
				chown(nod, fcontext->uid, fcontext->gid);
			}
			free(nod);
			return 0;
		}
		free(nod);
		if (errno != ENOSPC)
			return -errno;
	}
	return -errno;
}

#if _POSIX_SYNCHRONIZED_IO + 0 > 0 || defined(__FreeBSD__)
#undef HAVE_FDATASYNC
#else
#define HAVE_FDATASYNC 1
#endif

//fsync
static int mhdd_fsync(const char *path, int isdatasync,
		struct fuse_file_info *fi)
{
	struct flist *info;
	mhdd_debug(MHDD_MSG,
		"mhdd_fsync: path = %s handle = %llu\n", path, fi->fh);
	info = flist_item_by_id(fi->fh);
	int res;
	if (!info) {
		errno = EBADF;
		return -errno;
	}

	int fh = info->fh;

#ifdef HAVE_FDATASYNC
	if (isdatasync)
		res = fdatasync(fh);
	else
#endif
		res = fsync(fh);

	flist_unlock();
	if (res == -1)
		return -errno;
	return 0;
}

// Define extended attribute support

#ifndef WITHOUT_XATTR
static int mhdd_setxattr(const char *path, const char *attrname,
                const char *attrval, size_t attrvalsize, int flags)
{
	char * real_path = find_path(path);
	if (!real_path)
		return -ENOENT;

	mhdd_debug(MHDD_MSG,
		"mhdd_setxattr: path = %s name = %s value = %s size = %d\n",
                real_path, attrname, attrval, attrvalsize);
        int res = setxattr(real_path, attrname, attrval, attrvalsize, flags);
        free(real_path);
        if (res == -1) return -errno;
        return 0;
}
#endif

#ifndef WITHOUT_XATTR
static int mhdd_getxattr(const char *path, const char *attrname, char *buf, size_t count)
{
        int size = 0;
	char * real_path = find_path(path);
	if (!real_path)
		return -ENOENT;

	mhdd_debug(MHDD_MSG,
		"mhdd_getxattr: path = %s name = %s bufsize = %d\n",
                real_path, attrname, count);
        size = getxattr(real_path, attrname, buf, count);
        free(real_path);
        if (size == -1) return -errno;
        return size;
}
#endif

#ifndef WITHOUT_XATTR
static int mhdd_listxattr(const char *path, char *buf, size_t count)
{
        int ret = 0;
	char * real_path = find_path(path);
	if (!real_path)
		return -ENOENT;

	mhdd_debug(MHDD_MSG,
		"mhdd_listxattr: path = %s bufsize = %d\n",
                real_path, count);

        ret=listxattr(real_path, buf, count);
        free(real_path);
        if (ret == -1) return -errno;
        return ret;
}
#endif

#ifndef WITHOUT_XATTR
static int mhdd_removexattr(const char *path, const char *attrname)
{
	char * real_path = find_path(path);
	if (!real_path)
		return -ENOENT;

	mhdd_debug(MHDD_MSG,
		"mhdd_removexattr: path = %s name = %s\n",
                real_path, attrname);

        int res = removexattr(real_path, attrname);
        free(real_path);
        if (res == -1) return -errno;
        return 0;
}
#endif

// functions links
static struct fuse_operations mhdd_oper = {
	.getattr    	= mhdd_stat,
	.statfs     	= mhdd_statfs,
	.readdir    	= mhdd_readdir,
	.readlink   	= mhdd_readlink,
	.open       	= mhdd_fileopen,
	.release    	= mhdd_release,
	.read       	= mhdd_read,
	.write      	= mhdd_write,
	.create     	= mhdd_create,
	.truncate   	= mhdd_truncate,
	.ftruncate  	= mhdd_ftruncate,
	.access     	= mhdd_access,
	.mkdir      	= mhdd_mkdir,
	.rmdir      	= mhdd_rmdir,
	.unlink     	= mhdd_unlink,
	.rename     	= mhdd_rename,
	.utimens    	= mhdd_utimens,
	.chmod      	= mhdd_chmod,
	.chown      	= mhdd_chown,
	.symlink    	= mhdd_symlink,
	.mknod      	= mhdd_mknod,
	.fsync      	= mhdd_fsync,
	.link		= mhdd_link,
#ifndef WITHOUT_XATTR
        .setxattr   	= mhdd_setxattr,
        .getxattr   	= mhdd_getxattr,
        .listxattr  	= mhdd_listxattr,
        .removexattr	= mhdd_removexattr,
#endif
};


// start
int main(int argc, char *argv[])
{
	mhdd_debug_init();
	struct fuse_args *args = parse_options(argc, argv);
	flist_init();
	signal(SIGSEGV, save_backtrace);
	return fuse_main(args->argc, args->argv, &mhdd_oper, 0);
}
