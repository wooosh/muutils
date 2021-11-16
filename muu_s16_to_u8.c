#include "libmuu.h"

int main(int argc, char **argv) {
	struct muu_wire in, out;

	muu_arg_parse(argc, argv, MUU_ARGS(
		MUU_ARG_WIRE("in", &in, true),
		MUU_ARG_WIRE("out", &out, true)
	));

	while (1) {
		muu_wire_value sample;
		muu_wire_read(in, &sample);
		sample = sample/2 + INT16_MAX/2;
		muu_wire_write(&out, sample);
	}

	return 0;
}
