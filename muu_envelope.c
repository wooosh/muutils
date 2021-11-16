#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "libmuu.h"

/* Envelope generator
 *
 * Inputs
 *	time is given # of periods
 *
 * 	attack  - time it takes to reach full amplitude
 * 	decay   - time it takes to go from full amplitude to sustained level
 * 	sustain - consistent amplitude level
 * 	release - time it takes for the from sustained level to zero
 *
 *	retrig  - when changed from 0, resets envelope if gate is already held
 *	gate    - when >0, continues sustain
 *
 * 	signal  - source that the envelope is applied to
 *
 * Outputs
 * 	out - output
 */

enum envelope_state {
	ENVELOPE_NONE,
	ENVELOPE_ATTACK,
	ENVELOPE_DECAY,
	ENVELOPE_SUSTAIN,
	ENVELOPE_RELEASE
};

int main(int argc, char **argv) {
	struct muu_wire attack, decay, sustain, release,
			retrig, gate,
			signal,
			out;
	
	muu_arg_parse(argc, argv, MUU_ARGS(
		MUU_ARG_WIRE("attack", &attack, true),
		MUU_ARG_WIRE("decay", &decay, true),
		MUU_ARG_WIRE("sustain", &sustain, true),
		MUU_ARG_WIRE("release", &release, true),
		
		MUU_ARG_WIRE("retrig", &retrig, true),
		MUU_ARG_WIRE("gate", &gate, true),
		
		MUU_ARG_WIRE("signal", &signal, true),
		
		MUU_ARG_WIRE("out", &out, true)
	));

	enum envelope_state state = ENVELOPE_NONE;
	unsigned int state_time = 0;
	int16_t amplitude = 0;
	
	int16_t last_retrig_val = 0;
	int16_t last_gate_val = 0;
	while (1) {
		// TODO: table format with auto read and args?
		// TODO: update_wires(), with wire.value
		muu_wire_value attack_v, decay_v, sustain_v, release_v,
			       retrig_v, gate_v,
			       signal_v;

		muu_wire_read(attack, &attack_v);
		muu_wire_read(decay, &decay_v);
		muu_wire_read(sustain, &sustain_v);
		muu_wire_read(release, &release_v);

		muu_wire_read(retrig, &retrig_v);
		muu_wire_read(gate, &gate_v);

		muu_wire_read(signal, &signal_v);

		if (last_retrig_val == 0 && retrig_v > 0) {
			state = ENVELOPE_ATTACK;
			state_time = 0;
		}
		last_retrig_val = retrig_v;

		last_gate_val = gate_v;

		switch (state) {
			case ENVELOPE_NONE:
				amplitude = 0;
				break;
			case ENVELOPE_ATTACK: {
				if (state_time >= attack_v) {
					state = ENVELOPE_DECAY;
					state_time = 0;
				}
				int16_t remaining_time = attack_v - state_time;
				amplitude += (INT16_MAX - amplitude)/remaining_time;
				break;
			}
			case ENVELOPE_DECAY: {
				if (state_time >= decay_v) {
					state = ENVELOPE_SUSTAIN;
					state_time = 0;
				}
				int16_t remaining_time = decay_v - state_time;
				amplitude += (sustain_v - amplitude)/remaining_time;
				break;
			}
			case ENVELOPE_SUSTAIN:
				amplitude = sustain_v;
				if (gate_v == 0) {
					state = ENVELOPE_RELEASE;
					state_time = 0;
				}
				break;
			case ENVELOPE_RELEASE: {
				if (state_time >= release_v) {
					state = ENVELOPE_NONE;
					state_time = 0;
				}
				int16_t remaining_time = release_v - state_time;
				amplitude += (0 - amplitude)/remaining_time;
				break;
			}
		}

		state_time++;
		float gain_percent = amplitude/(float)INT16_MAX;
		int16_t out_v = gain_percent * signal_v;
		muu_wire_write(&out, out_v); 
	}

	return 0;
}
