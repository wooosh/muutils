#include <unistd.h>
#include <inttypes.h>

#include "libmuu.h"

int main(int argc, char **argv) {
	struct muu_wire in, out;
	// TODO: buffer size for out should be double since 2 channels
	muu_arg_parse(argc, argv, MUU_ARGS(
		MUU_ARG_WIRE("in", &in, true), 
		MUU_ARG_WIRE("out", &out, true)
	));
	
	while (1) {
		muu_wire_value sample;
		muu_wire_read(in, &sample);
		
		// TODO: write both at once
		muu_wire_write(&out, sample);
		muu_wire_write(&out, sample);
	}

	return 0;
}
