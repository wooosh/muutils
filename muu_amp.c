#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "libmuu.h"

int main(int argc, char **argv) {
	// TODO: automatically detect piping out, or default value of stdout
	struct muu_wire out;
	struct muu_wire signal;
	struct muu_wire gain;
	// used for scaling to a certain range
	struct muu_wire max = MUU_WIRE_CONST(INT16_MAX);
	//muu_wire min = MUU_WIRE_CONST(0);
	
	muu_arg_parse(argc, argv, MUU_ARGS(
		MUU_ARG_WIRE("out", &out, true),
		MUU_ARG_WIRE("signal", &signal, true),
		MUU_ARG_WIRE("gain", &gain, true),
		MUU_ARG_WIRE("max", &max, false)
		//MUU_ARG_WIRE("min", &min, false)
	));

	while (1) {
		muu_wire_value sample;
		muu_wire_value gain_value;
		muu_wire_value max_value;

		muu_wire_read(signal, &sample);
		muu_wire_read(gain, &gain_value);
		muu_wire_read(max, &max_value);

		//440x = 32767
	
		// TODO: scale value down to this
		float gain_percent = gain_value/(float)INT16_MAX;
		float max_percent = max_value/(float)INT16_MAX;
		int16_t out_val = sample * gain_percent * max_percent;
		//fprintf(stderr, "%d\n", out_val);
		muu_wire_write(&out, out_val); 
	}

	return 0;
}

/*
sample: 32767
gain: 32767
max: 440
max_coefficient: 32767*coeff = 440
*/
