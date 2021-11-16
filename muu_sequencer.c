#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libmuu.h"
#include "str.h"

#define MAX_TRACKS 32

// TODO: virtual machine?
// TODO: handle time looping?
// TODO: tokenizer?

struct muu_wire out;

int beats_per_second = 4;

struct command {
	uint8_t track;
	int16_t val;
};

struct entry {
	unsigned int time;
	int8_t len;
	struct command *commands;
};

struct sequence {
	uint8_t num_tracks;
	char *track_names[MAX_TRACKS];

	// sorted by timecode
	unsigned int num_entries;
	struct entry *entries;
};
	


// Tokenizer
enum token_type {
	TOKEN_TAB,
	TOKEN_NEWLINE,
	TOKEN_TEXT,
	TOKEN_EOF
};

struct token {
	enum token_type type;
	int line;
	// text is always present
	struct str text;
};

struct token token_pop(struct str *stream) {
	if (stream->start == stream->end) {
		return (struct token) {
			.type = TOKEN_EOF,
			.text = *stream
		};
	}
	// TODO: fill in token.line
	switch (stream->start[0]) {
	case '\n': {
		struct token tok_nl = {
			.type = TOKEN_NEWLINE, 
			.text = (struct str) {
				.start = stream->start,
				.end = stream->start + 1
			}
		};

		stream->start++;
		return tok_nl;
	}
	case '\t': {
		struct token tok_t = {
			.type = TOKEN_TAB, 
			.text = (struct str) {
				.start = stream->start,
				.end = stream->start + 1
			}
		};

		stream->start++;
		return tok_t;
	}
	}

	struct str text = {.start = stream->start};
	for (; stream->start[0] != '\t' &&
		stream->start[0] != '\n' && 
		stream->start != stream->end;
		stream->start++) {}
	text.end = stream->start;
	
	return (struct token) {
		.type = TOKEN_TEXT,
		.text = text
	};
}

struct token token_expect(struct str *stream, enum token_type type) {
	struct token tok = token_pop(stream);
	if (tok.type != type) {
		// TODO: better error message
		fprintf(stderr, "unexpected token type\n");
		exit(-1);
	}
	return tok; 
}

int strntol(struct str s) {
	// Copy string into temporary null terminated buffer
	char buf[32] = {0};
	if (s.end - s.start > 32) {
		fprintf(stderr, "numbers must be longer than 32 characters\n");
		exit(-1);
	}
	memmove(buf, s.start, s.end - s.start);
	
	errno = 0;
	char *end;
	int i = strtol(buf, &end, 0);

	if (buf == end) {
		fprintf(stderr, "invalid number '%s'\n", buf);
		exit(-1);
	// TODO: guard against out of range for s16, not just long
	} else if (errno == ERANGE) {
		fprintf(stderr, "number '%s' out of range\n", buf);
		exit(-1);
	}
	
	return i;
}

struct sequence parse_file(char *path) {
	struct str file = open_file(path);
	struct sequence seq = {0};
	
	// parse header
	struct token tok = token_expect(&file, TOKEN_TEXT);
	if (strncmp("time", tok.text.start, tok.text.end - tok.text.start) != 0) {
		fprintf(stderr, "first column in header must be 'time'\n");
		exit(-1);
	}
	token_expect(&file, TOKEN_TAB);

	int track_num = 0;
	while(1) {
		if (track_num == MAX_TRACKS) {
			fprintf(stderr, "maximum number of tracks exceeded\n");
			exit(-1);
		}

		struct str track_title = token_expect(&file, TOKEN_TEXT).text;
		seq.track_names[track_num] = malloc(track_title.end - track_title.start);
		memcpy(seq.track_names[track_num], track_title.start, track_title.end - track_title.start);
		track_num++;	

		// TODO: check if tab
		struct token delim = token_pop(&file);
		if (delim.type == TOKEN_NEWLINE) break;
	};
	seq.num_tracks = track_num;

	// parse records
	// TODO: dynamic number of records
	seq.entries = malloc(sizeof(struct entry)*64);
	seq.num_entries = 0;
	struct command cmds[MAX_TRACKS];

	while(1) {
		struct entry e = {};
		int num_cmds = 0;

		// Get timecode
		// TODO: verify timecodes are in ascending order
		struct str time = token_expect(&file, TOKEN_TEXT).text;
		e.time = strntol(time);

		// Parse track data
		for (int i=0; i<seq.num_tracks; i++) {
			token_expect(&file, TOKEN_TAB);
			struct str val_str = token_expect(&file, TOKEN_TEXT).text;
			int val_int = strntol(val_str);
			cmds[num_cmds] = (struct command){.track = i, .val = val_int};
			// TODO: overflow check
			num_cmds++;
		}
		token_expect(&file, TOKEN_NEWLINE);

		e.len = num_cmds;
		e.commands = malloc(sizeof(struct command) * num_cmds);
		memcpy(e.commands, cmds, sizeof(struct command) * num_cmds);

		seq.entries[seq.num_entries] = e;
		seq.num_entries++;
		
		if (file.start == file.end) break;
	}

	return seq;
}

void play_sequence(struct sequence seq) {
	// TODO: fixed point lib
	// TOOD: write entire sequence into memory
	int periods_per_beat = muu_sample_rate/beats_per_second;
	int entry_idx = 0;
	while (1) {
		int next_entry_idx = (entry_idx + 1) % seq.num_entries;
		int num_periods = 0;

		// if entry does not wrap around
		if (next_entry_idx > entry_idx) {
			num_periods = (seq.entries[next_entry_idx].time - seq.entries[entry_idx].time) * periods_per_beat;
		}

		// TODO: libmuu batching api
		// TODO: nonewires
		for (int i=0; i<num_periods; i++) {
			muu_wire_write(&out, (int16_t) seq.entries[entry_idx].commands[0].val);
		}
		
		entry_idx = next_entry_idx;
	}
}

int main(int argc, char **argv) {
	char *filename = NULL;
	// TODO: muu_arg_parse_unknown handler, default with err handler
	// TODO: adjustable bpm input?
	muu_arg_parse(argc, argv, MUU_ARGS(
		MUU_ARG_STR("file", &filename, true, NULL),
		MUU_ARG_WIRE("out", &out, true)
	));

	play_sequence(parse_file(filename));

	return 0;
}
