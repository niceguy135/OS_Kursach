all: clean
	gcc -g -o combiner_DK ./combiner/combiner.c 

	gcc -c rangefinder_hcsr04.c
	gcc rangefinder_hcsr04.o -o rangefinder_hcsr04_IlorDash

	gcc -c play_note.c
	gcc play_note.o -o play_note_DK
clean:
	rm -f *.o
