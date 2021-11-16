#pragma once

struct str {
	char *start;
	// the character after the end
	char *end;
};

int str_split_first(struct str src, struct str *before, struct str *after, char c);
struct str open_file(char *path);
