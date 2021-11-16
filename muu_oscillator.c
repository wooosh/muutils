#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include <stdio.h>

#include "libmuu.h"

// TODO: store waveform in memory to reduce # of write calls
// TODO: handle freq > sample rate/2

struct muu_wire out;
struct muu_wire freq;

void oscillator_tan(unsigned int freq) {
	float period = (muu_sample_rate/freq);
	while (1) {
		int16_t data;
		for (int i=0; i<period; i++) {
			data = (int16_t)(tanf(i/period * (float)M_PI) * INT16_MAX);
			muu_wire_write(&out, data);
		}
	}
}

void oscillator_sin(unsigned int freq2) {
	//float period = muu_sample_rate/freq;
	uint16_t time = 0;
	int16_t data = 0;
	while (1) {
		int16_t freq_val;
		muu_wire_read(freq, &freq_val);
		if (freq_val != 0) {
			float period = muu_sample_rate/freq_val;
		
			if (time/period * 2 * (float)M_PI > 2 * (float)M_PI) {
				time = 0;
			}
			data = sinf(time/period * 2 * (float)M_PI) * INT16_MAX;
			time++;
		}
		muu_wire_write(&out, data);
	}
}

void oscillator_triangle(unsigned int freq) {
	// Switches direction every half period
	int period = muu_sample_rate/freq;
	int16_t delta = (INT16_MAX)/(period/2);

	while (1) {
		int16_t data = INT16_MIN;
		for (int i=0; i<(period/2); i++) {
			data += delta;
			muu_wire_write(&out, data);
		}

		data = INT16_MAX;
		for (int i=0; i<(period/2); i++) {
			data -= delta;
			muu_wire_write(&out, data);
		}
	}
}

void oscillator_saw(unsigned int freq2) {
	// Uses wraparound behavior
	uint16_t data = 0;

	while (1) {
		int16_t freq_val;
		muu_wire_read(freq, &freq_val);
		if (freq_val != 0) {
			uint16_t period = muu_sample_rate/freq_val;
			data += UINT16_MAX/period;
		}
		muu_wire_write(&out, data);
	}
}

void oscillator_square(unsigned int freq) {
	int period = muu_sample_rate/freq;
	int16_t data = INT16_MIN;
	
	while (1) {
		data = (data == INT16_MIN) ? INT16_MAX : INT16_MIN;
		for (int i=0; i<(period/2); i++) {
			muu_wire_write(&out, data);
		}
	}
}

struct muu_map_entry osc_map[] = MUU_MAP(
	{"sin", oscillator_sin},
	{"triangle", oscillator_triangle},
	{"saw", oscillator_saw},
	{"square", oscillator_square},
	{"tan", oscillator_tan}
);

int main(int argc, char** argv) {
	char *osc_name = "sin";

	muu_arg_parse(argc, argv,  MUU_ARGS(
		MUU_ARG_STR("type",  &osc_name, false,
			"sin", "triangle", "saw", "square", "tan"),
		MUU_ARG_WIRE("out", &out, true),
		MUU_ARG_WIRE("freq", &freq, true)
	));
		
	void (*osc)(unsigned int) = muu_map_find(osc_map, osc_name);
	osc(440);
	return 0;
}
