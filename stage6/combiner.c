#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define SUCCESS 0

typedef struct stdinCommands {
    int id;
    char *stdin_command;
    int range_min;
    int range_max;
} command;

void handler(){
	printf("Catch a SIGINT! Shutting off...");
    return -2;
}

void* readFromStdin(void *args) {
    command *arg = (command*) args;

    if( strstr(arg->stdin_command, "start") != NULL ) {

        if( strstr(arg->stdin_command, "rangefinder_1") != NULL ) {
            system("sudo ./rangefinder_hcsr04_1");

        } else if ( strstr(arg->stdin_command, "rangefinder_2") != NULL ) {
            system("sudo ./rangefinder_hcsr04_2");

        } else if ( strstr(arg->stdin_command, "play_note") != NULL ) {
            system("sudo ./play_note -q");

        } else
            printf("Cant understand what to launch!\n");

    } else if ( strstr(arg->stdin_command, "stop") != NULL ) {

        if( strstr(arg->stdin_command, "rangefinder_1") != NULL ) {
            system("sudo pkill rangefinder_hcsr04_1");

        } else if ( strstr(arg->stdin_command, "rangefinder_2") != NULL ) {
            system("sudo pkill rangefinder_hcsr04_2");

        } else if ( strstr(arg->stdin_command, "play_note") != NULL ) {
            system("sudo pkill play_note -q");

        } else
            printf("Cant understand what to kill!\n");
        
    } else if ( strstr(arg->stdin_command, "set_min") != NULL ) {

        char *_  = strtok(arg->stdin_command, " ");
        char *num  = strtok(NULL, " ");

        arg->range_min = (int)atoi(num);

    } else if ( strstr(arg->stdin_command, "set_max") != NULL ) {

        char *_  = strtok(arg->stdin_command, " ");
        char *num  = strtok(NULL, " ");

        arg->range_max = (int)atoi(num);

    } else
        printf("Cant understand!\n");

    return SUCCESS;
}

int main(int argc, char *argv[])
{
    int fd_fifo, fd_fifo_rf_1, fd_fifo_rf_2;

	int range_max=0, range_min=0;
	int volume_max=0, volume_min=0;

    short calibrating = 1;

    struct sigaction act = {};

	act.sa_handler = &handler;
	sigaction(SIGINT, &act, NULL);

	while (1) {

        char buff_but[100] = {""};
        char buff_rf_1[100] = {""};
        char buff_rf_2[100] = {""};

        struct timespec cur_time;
		clock_gettime(CLOCK_REALTIME, &cur_time); //Работаю со временем
		long int cur_secs = cur_time.tv_sec % 86400;
		int hours = (int)(cur_secs / 3600);
		int mins = (int)( (cur_secs - 3600 * hours)  / 60);
		int secs = (int)(cur_secs - 3600 * hours - 60 * mins);




        // Получение расст для ноты
        //////////////////////////////////////////////////////////////////////////////////////////
        system("sudo ./rangefinder_hcsr04_1 &");
        if((fd_fifo_rf_1 = open("range_data_1", O_RDONLY)) <= -1) {

            printf("Can't open FIFO for rangeData 1\n");
            return -1;

        } else {
                
            if(read(fd_fifo_rf_1, buff_rf_1, sizeof(buff_rf_1)) == -1) {
                printf("Can't read FIFO for rangeData 1\n");
                return -1;
            }
			close(fd_fifo_rf_1);
        }

        float range_float = atof(buff_rf_1);
		int range_cur = (int)(range_float * 1000000);
        ///////////////////////////////////////////////////////////////////////////////////////////


        // Получение расст для громкости
        ///////////////////////////////////////////////////////////////////////////////////////////
        system("sudo ./rangefinder_hcsr04_2 &");
        if((fd_fifo_rf_2 = open("range_data_2", O_RDONLY)) <= -1) {

            printf("Can't open FIFO for rangeData 2\n");
            return -1;

        } else {
                
            if(read(fd_fifo_rf_2, buff_rf_2, sizeof(buff_rf_2)) == -1) {
                printf("Can't read FIFO for rangeData 2\n");
                return -1;
            }
			close(fd_fifo_rf_2);
        }

        float range_float = atof(buff_rf_2);
		int volume_cur = (int)(range_float * 1000000);
        ///////////////////////////////////////////////////////////////////////////////////////////

		char note[3] = {""};
		char volume[4] = {""};
		strcpy(note,"");
		strcpy(volume,"");

		int diff_range = 0, diff_volume = 0;

			diff_range = (range_max - range_min)/12;
			diff_volume = (volume_max - volume_min)/3;

			if (range_cur <= (range_min + diff_range))
				strcpy(note,"C_");
			else if (range_cur <= (range_min + diff_range * 2))
				strcpy(note,"C#");
			else if (range_cur <= (range_min + diff_range * 3))
				strcpy(note,"D_");
			else if (range_cur <= (range_min + diff_range * 4))
				strcpy(note,"D#");
			else if (range_cur <= (range_min + diff_range * 5))
				strcpy(note,"E_");
			else if (range_cur <= (range_min + diff_range * 6))
				strcpy(note,"F_");
			else if (range_cur <= (range_min + diff_range * 7))
				strcpy(note,"F#");
			else if (range_cur <= (range_min + diff_range * 8))
				strcpy(note,"D_");
			else if (range_cur <= (range_min + diff_range * 9))
				strcpy(note,"D#");
			else if (range_cur <= (range_min + diff_range * 10))
				strcpy(note,"A_");
			else if (range_cur <= (range_min + diff_range * 11))
				strcpy(note,"A#");
			else if (range_cur <= range_max)
				strcpy(note,"B_");
			else
				{
					printf("Range is out of bounds!\n");
					continue;
				}

			if (volume_cur <= volume_min + diff_volume)
				strcpy(volume,"min");
			else if(volume_cur <= volume_min + diff_volume * 2)
				strcpy(volume,"cur");
			else if (volume_cur <= volume_max)
				strcpy(volume,"max");
			else
				{
					printf("Volume is out of bounds!\n");
					continue;
				}

        
		if ((note[0] == 'A') && (note[1] != '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing A#",hours, mins, secs, volume);
		}
		if (note[0] == 'B') {
			printf("Time: %d-%d-%d; Volume: %s Playing B",hours, mins, secs, volume);
		}
		if ((note[0] == 'C') && (note[1] != '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing C#",hours, mins, secs, volume);
		}
		if ((note[0] == 'D') && (note[1] != '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing D#",hours, mins, secs, volume);
		}
		if (note[0] == 'E') {
			printf("Time: %d-%d-%d; Volume: %s Playing E",hours, mins, secs, volume);
		}
		if ((note[0] == 'F') && (note[1] != '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing F#",hours, mins, secs, volume);
		}
		if ((note[0] == 'G') && (note[1] != '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing G#",hours, mins, secs, volume);
		}
		if ((note[0] == 'A') && (note[1] == '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing A#",hours, mins, secs, volume);
		}
		if ((note[0] == 'C') && (note[1] == '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing C#",hours, mins, secs, volume);
		}
		if ((note[0] == 'D') && (note[1] == '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing D#",hours, mins, secs, volume);
		}
		if ((note[0] == 'F') && (note[1] == '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing F#",hours, mins, secs, volume);
		}
		if ((note[0] == 'G') && (note[1] == '#')) {
			printf("Time: %d-%d-%d; Volume: %s Playing G#",hours, mins, secs, volume);
		}
		fflush(stdout);
	}

	return 0;
}
