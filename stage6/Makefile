all : rangefinder_hcsr04_1.o rangefinder_hcsr04_2.o combiner.o
	gcc -o rangefinder_hcsr04_1 rangefinder_hcsr04_1.o
	gcc -o rangefinder_hcsr04_2 rangefinder_hcsr04_2.o
	gcc -o combiner combiner.o	
clean :
	rm -rf rangefinder_hcsr04_1 rangefinder_hcsr04_2 combiner *.o
rangefinder_hcsr04_1.o : rangefinder_hcsr04_1.c
	gcc -c -o rangefinder_hcsr04_1.o rangefinder_hcsr04_1.c
rangefinder_hcsr04_2.o : rangefinder_hcsr04_2.c
	gcc -c -o rangefinder_hcsr04_2.o rangefinder_hcsr04_2.c
combiner.o : combiner.c
	gcc -c -o combiner.o combiner.c