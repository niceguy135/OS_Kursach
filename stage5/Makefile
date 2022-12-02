
all : rangefinder_hcsr04_1.o play_note.o
	gcc -o rangefinder_hcsr04_1 rangefinder_hcsr04_1.o
	gcc -o play_note play_note.o	
clean :
	rm -rf rangefinder_hcsr04_1 play_note *.o
rangefinder_hcsr04_1.o : rangefinder_hcsr04_1.c
	gcc -c -o rangefinder_hcsr04_1.o rangefinder_hcsr04_1.c
play_note.o : play_note.c
	gcc -c -o play_note.o play_note.c