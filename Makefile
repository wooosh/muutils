.PHONY: all clean

all: libmuu.o muuoscillator muuamp muumux muusequencer muuenvelope muus16tou8

clean:
	rm -f libmuu.o muuoscillator muuamp muumux muusequencer muuenvelope muus16tou8

str.o: str.c str.h
	$(CC) -c -o str.o str.c

libmuu.o: libmuu.c libmuu.h
	$(CC) -c -o libmuu.o libmuu.c

muuoscillator: muu_oscillator.c libmuu.o
	$(CC) -o muuoscillator muu_oscillator.c -lm libmuu.o

muuamp: muu_amp.c libmuu.o
	$(CC) -o muuamp muu_amp.c libmuu.o

muuenvelope: muu_envelope.c libmuu.o
	$(CC) -o muuenvelope muu_envelope.c libmuu.o

muumux: muu_mux.c libmuu.o
	$(CC) -o muumux muu_mux.c libmuu.o

muus16tou8: muu_s16_to_u8.c libmuu.o
	$(CC) -o muus16tou8 muu_s16_to_u8.c libmuu.o

muusequencer: muu_sequencer.c libmuu.h libmuu.o str.o
	$(CC) -o muusequencer muu_sequencer.c libmuu.o str.o

