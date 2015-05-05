#include <sys/statvfs.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc<2)
	{
		printf("Usage: %s directory\n", argv[0]);
		_exit(-1);
	}

	struct statvfs stv;
	int ret=statvfs(argv[1], &stv);

	printf(
	    "             Path:%s\n"
	    " statvfs returned:%d\n\n"
	    "          f_bsize=%12llu /* file system block size */\n"
	    "         f_frsize=%12llu /* fragment size */\n"
	    "         f_blocks=%12llu /* size of fs in f_frsize units */\n"
	    "          f_bfree=%12llu /* # free blocks */\n"
	    "         f_bavail=%12llu /* # free blocks for non-root */\n"
	    "          f_files=%12llu /* # inodes */\n"
	    "          f_ffree=%12llu /* # free inodes */\n"
	    "         f_favail=%12llu /* # free inodes for non-root */\n"
	    "           f_fsid=%12llu /* file system ID */\n"
	    "           f_flag=%12llu /* mount flags */\n"
	    "        f_namemax=%12llu /* maximum filename length */\n",
	argv[1], ret,
	(unsigned long long)stv.f_bsize,
	(unsigned long long)stv.f_frsize,
	(unsigned long long)stv.f_blocks,
	(unsigned long long)stv.f_bfree,
	(unsigned long long)stv.f_bavail,
	(unsigned long long)stv.f_files,
	(unsigned long long)stv.f_ffree,
	(unsigned long long)stv.f_favail,
	(unsigned long long)stv.f_fsid,
	(unsigned long long)stv.f_flag,
	(unsigned long long)stv.f_namemax);
	return 0;
}
