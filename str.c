#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

#include "str.h"

int str_split_first(struct str src, struct str *before, struct str *after, char c) {
	char *delim = memchr(src.start, c, src.end - src.start);
	if (delim == NULL) {
		if (before != NULL) {
			*before = src;
		}

		if (after != NULL) {
			after->start = src.end;
			after->end = src.end;
		}

		return -1;
	} else {
		if (before != NULL) {
			before->start = src.start;
			before->end = delim;
		}

		if (after != NULL) {
			after->start = delim+1;
			after->end = src.end;
		}

		return 0;
	}
}


struct str open_file(char *path) {
	// TODO: close properly
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(-1);
	}

	off_t size = lseek(fd, 0, SEEK_END);
	
	void *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		perror("mmap");
		exit(-1);
	}

	return (struct str){.start = data, .end = data + size};
}
