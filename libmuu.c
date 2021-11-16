#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


// TODO: read sample rate and buffer size from env

#include "libmuu.h"

int muu_sample_rate = 44100;

// Protects against partial reads/writes, exits if fails
void muu_wire_read(struct muu_wire wire, muu_wire_value *v) {
	if (wire.type == MUU_WIRE_CONSTANT) {
		*v = wire.value.constant;
	} else {
		// TODO: handle EINTR, partial reads
		int num_read = read(wire.value.stream.fd, v, sizeof(muu_wire_value));
		if (num_read < 0) {
			perror("muu_wire_read");
			exit(-1);
		}
	}
}

void muu_wire_write(struct muu_wire *wire, muu_wire_value v) {
	if (wire->type == MUU_WIRE_CONSTANT) {
		// you can't write to a constant wire.
		exit(-1);
	} else {
		struct muu_wire_stream *stream = &wire->value.stream;
		// TODO: buffer underruns? last buffer commit time?
		stream->write_buffer[stream->buf_len] = v;
		stream->buf_len++;

		// TODO: write buffering system
		if (stream->buf_len == stream->buf_size) {
			// TODO: handle EINTR, partial writes
			int num_written = write(stream->fd, stream->write_buffer, stream->buf_len*sizeof(muu_wire_value));
			if (num_written < 0) {
				perror("muu_wire_write");
				exit(-1);
			}
			stream->buf_len = 0;
		}
	}
}

// Returns null if not found
static struct muu_arg* muu_arg_by_name(struct muu_arg *args, char *str, size_t len) {
	for (struct muu_arg *arg=args; arg->name != NULL; arg++) {
		if (strncmp(arg->name, str, len) == 0) {
			return arg;
		}
	}

	return NULL;
}

// strtol that dies on errors
static int muu_arg_strtol(char *argname, char *str, bool is_sample) {
	char *end;
	errno = 0;
	int i = strtol(str, &end, 0);
	
	// Check if no data was read
	if (str == end) {
		fprintf(stderr, "invalid number '%s' while parsing argument '%s'\n",
				str, argname);
		exit(-1);
	} else if (errno == ERANGE ||
			(is_sample && (i > INT16_MAX || i < INT16_MIN))) {
		fprintf(stderr, "number '%s' out of range for argument '%s'\n",
				str, argname);
	}

	return i;
}

static void muu_arg_wire_fill(struct muu_arg *arg, char *val) {
	struct muu_wire *wire = arg->arg.wire;
	if (strncmp(val, "fd", 2) == 0) {
		wire->type = MUU_WIRE_STREAM;
		
		val += 2;
		wire->value.stream.fd = muu_arg_strtol(arg->name, val, false);
		// TODO: configurable buffer size
		wire->value.stream.buf_size = 128;
		wire->value.stream.buf_len = 0;
		// TODO: checked malloc
		wire->value.stream.write_buffer = malloc(wire->value.stream.buf_size * sizeof(muu_wire_value));
		
	} else {
		wire->type = MUU_WIRE_CONSTANT;
		wire->value.constant = muu_arg_strtol(arg->name, val, true);
	}
}

static void muu_arg_str_fill(struct muu_arg *arg, char *val) {
	struct muu_arg_str str_arg = arg->arg.str;

	if (str_arg.options[0] == NULL) {
		*str_arg.value = val;
	} else {
		for (char **option = str_arg.options; *option != NULL; option++) {
			if (strcmp(*option, val) == 0) {
				*str_arg.value = val;
				return;
			}
		}

		fprintf(stderr, "Invalid value '%s' for argument '%s'\n", val, arg->name);
		fprintf(stderr, "Accpetable values: ");
		for (char **option = str_arg.options; *option != NULL; option++) {
			fprintf(stderr, "%s ", *option);
		}
		fprintf(stderr, "\n");
		exit(-1);
	}
}

// TODO: muu_arg_help
void muu_arg_parse(int argc, char **argv, struct muu_arg *args) {
	// Skip binary name
	argv++;
	argc--;

	while (argc > 0) {
		char *arg = *argv;

		char *eq = strchr(arg, '=');
		char *val = eq+1;
		if (eq == NULL) {
			fprintf(stderr, "Expected '=' in '%s'\n", arg);
			exit(-1);
		}

		struct muu_arg *arg_data = muu_arg_by_name(args, arg, eq - arg);
		if (arg_data == NULL) {
			fprintf(stderr, "Invalid argument name '%.*s'\n", eq - arg, arg);
			exit(-1);
		}

		arg_data->filled = true;
		switch (arg_data->type) {
		case MUU_ARG_WIRE:
			muu_arg_wire_fill(arg_data, val);
			break;
		case MUU_ARG_STR:
			muu_arg_str_fill(arg_data, val);
			break;
		}

		argv++;
		argc--;
	}

	// Ensure all requiredarguments are filled
	for (struct muu_arg *arg=args; arg->name != NULL; arg++) {
		if (arg->required && !arg->filled) {
			fprintf(stderr, "Expected argument '%s'\n", arg->name);
			exit(-1);
		}
	}
}

void *muu_map_find(struct muu_map_entry *map, char *key) {
	for (struct muu_map_entry *entry = map; entry->key != NULL; entry++) {
		if (strcmp(key, entry->key) == 0) return entry->value;
	}

	return NULL;
}
