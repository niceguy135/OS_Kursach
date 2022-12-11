#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

void help();
int main(int argc, char *argv[])
{
	int quiet = 0;
    int fd_fifo_0, fd_fifo_1, fd_fifo_rf_1, fd_fifo_rf_2;
    int exist_fifo = 0;

    int prev_butt_0 = 1, prev_butt_1 = 1;

	int range_max=0, range_min=0;
	int volume_max=0, volume_min=0;

	if (argc >= 5) {
        exist_fifo = 1;
	}

	while (1) {

        char buff0[100] = {""};
        char buff1[100] = {""};
        char buff_rf_1[100] = {""};
        char buff_rf_2[100] = {""};
		strcpy(buff0,"");
        strcpy(buff1,"");
        strcpy(buff_rf_1,"");
        strcpy(buff_rf_2,"");

        struct timespec cur_time;
		clock_gettime(CLOCK_REALTIME, &cur_time); //Работаю со временем
		long int cur_secs = cur_time.tv_sec % 86400;
		int hours = (int)(cur_secs / 3600);
		int mins = (int)( (cur_secs - 3600 * hours)  / 60);
		int secs = (int)(cur_secs - 3600 * hours - 60 * mins);
              
        char command[80] = "sudo ./button_read ";
		char number0[1];
		sprintf(number0, "%d", prev_butt_0);
		char number1[1];
        sprintf(number1, "%d", prev_butt_1);
        char *command1 = strcat(command, number0);
		char *command2 = strcat(command, " ");
		char *command3 = strcat(command, number1);
		char *command4 = strcat(command, " &");
		system(command4);

        if (exist_fifo == 1) {

			if((fd_fifo_0=open("button_data_b0", O_RDONLY)) <= -1){      //TODO: херня сломается, если не будет quiet. Доработать, елси понадобиться

				printf("Can't open FIFO for button 0");
                return -1;
			} else {
				
				if(read(fd_fifo_0, buff0, sizeof(buff0)) == -1) {
                    strcpy(buff0, "Can't read FIFO for button 0");
                    return -1;
                }
				else {		

					if( strstr(buff0, "-1") == NULL) {		//Ставлю новое состояние кнопки

						if(strstr(buff0, "Button 0") != NULL)
							prev_butt_0 = 0;                //Единичное списывание кнопки
                        else if(strstr(buff0, "-2") != NULL)
                            prev_butt_0 = 1;               //Кнопка зажаnf

					} else {
						prev_butt_0 = 1;                    //Нихуя не нажато
					}
				}

				close(fd_fifo_0);
			}

            if((fd_fifo_1=open("button_data_b1", O_RDONLY)) <= -1){      //TODO: херня сломается, если не будет quiet. Доработать, елси понадобиться

				printf("Can't open FIFO for button 1");
                return -1;
			} else { 
				if(read(fd_fifo_1, &buff1, sizeof(buff1)) == -1) {
                    strcpy(buff1, "Can't read FIFO for button 1");
                    return -1;
                }

				else {		

					if( strstr(buff1, "-1") == NULL) {		//Ставлю новое состояние кнопки

						if(strstr(buff1, "Button 1") != NULL)
							prev_butt_1 = 0;                //Единичное списывание кнопки
                        else if(strstr(buff1, "-2") != NULL)
                            prev_butt_1 = 1;               //Кнопка зажата
              
					} else {
						prev_butt_1 = 1;                    //Нихуя не нажато
					}
				}

				close(fd_fifo_1);
			}

            if((fd_fifo_rf_1=open("range_data_1", O_RDONLY)) == -1){      //TODO: херня сломается, если не будет quiet. Доработать, елси понадобиться

				printf("Can't open FIFO for RangeFinder1");
                return -1;
			} else {
				if(read(fd_fifo_rf_1, &buff_rf_1, sizeof(buff_rf_1)) == -1) {
                    strcpy(buff_rf_1, "Can't read FIFO for RangeFinder1");
                    return -1;
                } else {
					if(strstr(buff_rf_1, "SIGINT") != NULL) {
							printf("Catch SIGINT from RF1! Closing... ");
							kill(getpid(),SIGKILL);
						}
				}

				close(fd_fifo_1);
			}

            if((fd_fifo_rf_2=open("range_data_2", O_RDONLY)) == -1){      //TODO: херня сломается, если не будет quiet. Доработать, елси понадобиться

				printf("Can't open FIFO for RangeFinder2");
                return -1;
			} else {
				if(read(fd_fifo_rf_2, &buff_rf_2, sizeof(buff_rf_2)) == -1) {
                    strcpy(buff_rf_2, "Can't read FIFO for RangeFinder2");
                    return -1;
                } else {
					if(strstr(buff_rf_2, "SIGINT") != NULL) {
							printf("Catch SIGINT from RF2! Closing... ");
							kill(getpid(),SIGKILL);
						}
				}

				close(fd_fifo_rf_2);
			}
		}

		// printf("From button0: %s", buff0);
		// printf("From button1: %s", buff1);
		// printf("From rf1: %s\n", buff_rf_1);
		// printf("From rf2: %s\n", buff_rf_2);

		if (prev_butt_0 == 0) {
			printf("Start recolibrating!\n");
			range_max = 0;
			volume_max = 0;
		}

		float range_float = atof(buff_rf_1);
		int range_cur = (int)(range_float * 1000000);

		float volume_float = atof(buff_rf_2);
		int volume_cur = (int)(volume_float * 1000000);

		printf("Cur dist: %d\n", range_cur);
		printf("Cur volume: %d\n", volume_cur);
        
        if (range_max == 0 || volume_max == 0) {
			if(prev_butt_1 == 0 && range_max == 0) {

				range_max = range_cur;
				printf("Time: %d-%d-%d; Settting the max range for notes. Range is: %d\n",hours, mins, secs, range_max);

			} else
			 	printf("Calibrate range for notes!\n");

			if (prev_butt_1 == 0 && volume_max == 0) {

				volume_max = volume_cur;
				printf("Time: %d-%d-%d; Settting the max volume for notes. Range is: %d\n",hours, mins, secs, volume_max);
			} else
				printf("Calibrate volume for notes!\n");
		}

		char note[3] = {""};
		char volume[4] = {""};
		strcpy(note,"");
		strcpy(volume,"");

		int diff_range = 0, diff_volume = 0;

		if (range_max != 0 && volume_max != 0) {
			diff_range = range_max/12;
			diff_volume = volume_max/3;

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

		} else continue;
        
		if ((note[0] == 'A') && (note[1] != '#')) {
			//system("aplay ./notes/A4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing A#",hours, mins, secs, volume);
		}
		if (note[0] == 'B') {
			//system("aplay ./notes/B4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing B",hours, mins, secs, volume);
		}
		if ((note[0] == 'C') && (note[1] != '#')) {
			//system("aplay ./notes/C4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing C#",hours, mins, secs, volume);
		}
		if ((note[0] == 'D') && (note[1] != '#')) {
			//system("aplay ./notes/D4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing D#",hours, mins, secs, volume);
		}
		if (note[0] == 'E') {
			//system("aplay ./notes/E4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing E",hours, mins, secs, volume);
		}
		if ((note[0] == 'F') && (note[1] != '#')) {
			//system("aplay ./notes/F4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing F#",hours, mins, secs, volume);
		}
		if ((note[0] == 'G') && (note[1] != '#')) {
			//system("aplay ./notes/G4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing G#",hours, mins, secs, volume);
		}
		if ((note[0] == 'A') && (note[1] == '#')) {
			//system("aplay ./notes/Ad4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing A#",hours, mins, secs, volume);
		}
		if ((note[0] == 'C') && (note[1] == '#')) {
			//system("aplay ./notes/Cd4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing C#",hours, mins, secs, volume);
		}
		if ((note[0] == 'D') && (note[1] == '#')) {
			//system("aplay ./notes/Dd4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing D#",hours, mins, secs, volume);
		}
		if ((note[0] == 'F') && (note[1] == '#')) {
			//system("aplay ./notes/Fd4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing F#",hours, mins, secs, volume);
		}
		if ((note[0] == 'G') && (note[1] == '#')) {
			//system("aplay ./notes/Gd4.wav -q");
			printf("Time: %d-%d-%d; Volume: %s Playing G#",hours, mins, secs, volume);
		}
		fflush(stdout);
	}

	return 0;
}
