#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern int muu_sample_rate;

typedef int16_t muu_wire_value;

enum muu_wire_type {
	MUU_WIRE_CONSTANT,
	MUU_WIRE_STREAM
};

struct muu_wire_stream {
	int fd;
	muu_wire_value *write_buffer;
	size_t buf_size;
	size_t buf_len;
};

struct muu_wire {
	enum muu_wire_type type;
	union {
		struct muu_wire_stream stream;
		muu_wire_value constant;
	} value;
};

#define MUU_WIRE_CONST(val)\
	(struct muu_wire) {\
		.type = MUU_WIRE_CONSTANT,\
		.value.constant = (val)\
	}

// Protects against partial reads/writes, exits if fails
void muu_wire_read(struct muu_wire, muu_wire_value*);
void muu_wire_write(struct muu_wire*, muu_wire_value);

// Arg parsing
struct muu_arg_str {
	char **value;

	// NULL ptr terminated, NULL, if unused
	char **options;
};

enum muu_arg_type {
	MUU_ARG_WIRE,
	MUU_ARG_STR
};

struct muu_arg {
	char *name;
	bool filled;
	bool required;

	enum muu_arg_type type;
	union {
		struct muu_arg_str str;
		struct muu_wire *wire;
	} arg;
};

#define MUU_ARGS(...)\
	(struct muu_arg[]){__VA_ARGS__, (struct muu_arg){.name = NULL}}

#define MUU_ARG_WIRE(argname, wirevar, is_required)\
	(struct muu_arg) {\
		.name = (argname),\
		.filled = false,\
		.required = (is_required),\
		.type = MUU_ARG_WIRE,\
		.arg.wire = (wirevar)\
	}

#define MUU_ARG_STR(argname, strvar, is_required, ...)\
	(struct muu_arg) {\
		.name = (argname),\
		.filled = false,\
		.required = (is_required),\
		.type = MUU_ARG_STR,\
		.arg.str = (struct muu_arg_str) {\
			.value = (strvar),\
			.options = (char*[]){__VA_ARGS__, NULL}\
		}\
	}

void muu_arg_parse(int argc, char **argv, struct muu_arg *args);

// Simple map for help with parsing arguments, not performant at all
struct muu_map_entry {
	char *key;
	void *value;
};

#define MUU_MAP(...)\
	(struct muu_map_entry[]) {__VA_ARGS__, {.key = NULL}}

// Returns null if not found
void *muu_map_find(struct muu_map_entry*, char*);
