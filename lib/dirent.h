#ifndef __DIRENT_H
#define __DIRENT_H

#include "sys/types.h"

typedef struct __DIR DIR; /* opaque */

struct dirent
{
	ino_t d_ino;
	off_t d_off;
#ifdef __GOT_SHORT_LONG
	unsigned short int d_reclen;
#else
#  warning TODO: short+long
	unsigned char d_reclen[2];
#endif
	unsigned char d_type;
	char d_name[256];
};

int dirfd(DIR *dirp);

DIR *opendir(const char *name);
DIR *fdopendir(int fd);
int closedir(DIR *dirp);

struct dirent *readdir(DIR *dirp);

void rewinddir(DIR *dirp);
#ifdef __GOT_SHORT_LONG
void seekdir(DIR *dirp, long pos);
long telldir(DIR *dirp);
#endif

int scandir(const char *restrict dir,
						struct dirent ***restrict namelist,
						int (*selector)(const struct dirent *),
						int (*cmp)(const struct dirent **, const struct dirent **));

#endif
