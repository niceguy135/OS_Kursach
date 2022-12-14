/*******************************************************************************
 * Copyright (c) 2022 Sergey Balabaev (sergei.a.balabaev@gmail.com)                    *
 *                                                                             *
 * The MIT License (MIT):                                                      *
 * Permission is hereby granted, free of charge, to any person obtaining a     *
 * copy of this software and associated documentation files (the "Software"),  *
 * to deal in the Software without restriction, including without limitation   *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell   *
 * copies of the Software, and to permit persons to whom the Software is       *
 * furnished to do so, subject to the following conditions:                    *
 * The above copyright notice and this permission notice shall be included     *
 * in all copies or substantial portions of the Software.                      *
 *                                                                             *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,             *
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR       *
 * OTHER DEALINGS IN THE SOFTWARE.                                             *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>

#define ARG_CNT 4
#define QUIT_HELP_OPT_POS 1

#define READ_BUTTON_LEN 17 //hh:mm:ss.ssssss_n

void help();

int writeFifo(char * fifoName, char * str) {

	//printf("In write fifo = %s\n", fifoName);

	int fifo_d = open(fifoName, O_WRONLY);

	if (fifo_d == -1) {
		printf ( "Failed to open named pipe = %s", fifoName);
		return -1;
	}

	write(fifo_d, str, strlen(str) + 1);
	close(fifo_d);
	return 0;
}

int readFifo(char * fifo_name, char* read_str, int str_len) {
	int fifo_d = open(fifo_name, O_RDONLY);

	if (fifo_d == -1) {
		printf ( "Failed to open named pipe = %s",  fifo_name);
		return -1;
	}

	int read_num = read(fifo_d, read_str, str_len);
	close(fifo_d);

	//printf("65) In read fifo = %s, Read = %s\n", fifo_name, read_str);

	return read_num;
}

int getButtonState(char * fifo_name) {
	char button_str[100] = {0};
	//printf("72) init button_str = %s\n", button_str);
	int button_state_pos = readFifo(fifo_name, button_str, 100);

	if (button_state_pos < 0) {
		printf ( "Failed to open get Button state pipe = %s",  fifo_name);
		return -1;
	}

	char* last_new_line = strrchr(button_str, '\n');
	//printf("button_str = %s", button_str);
	//printf("button_str last = %d\n", last_new_line[-1] - '0');
	return last_new_line[-1] - '0';
}

float getRange(char * fifo_name) {
	char range_str[32] = {0};

	if (readFifo(fifo_name, range_str, 32) < 0) {
		printf ( "Failed to open get Range pipe = %s",  fifo_name);
		return -1;
	}

	float range = 0.0f;
	sscanf(range_str, "%f", &range);
	return range;
}

int setNote(char * fifo_name, char * note) {
	if (writeFifo(fifo_name, note) < 0) {
		printf ( "Failed to open set Note pipe = %s",  fifo_name);
		return -1;
	}

	return 0;
}





int stop_prog_recv = 0;
pthread_mutex_t mutex_stop = PTHREAD_MUTEX_INITIALIZER;

char* stop_prog_fifo_name = "stop_prog";

void *readStopProgFifoThread() {
	char stop_prog_str[16] = {'\0'};
	pthread_mutex_lock(&mutex_stop);

	while (1) {
		int readFifoNum = readFifo(stop_prog_fifo_name, stop_prog_str, 16);
		if (readFifoNum < 0) {
			pthread_exit(0);
		}
		if (!strncmp(stop_prog_str, "stop", strlen("stop"))) {
			stop_prog_recv = 1;
			pthread_mutex_unlock(&mutex_stop);
			pthread_exit(0);
		}
	}
}

float range = 0.0f;
pthread_mutex_t mutex_range = PTHREAD_MUTEX_INITIALIZER;

void *readRangeFifoThread(void * args) {
	while (1) {
		pthread_mutex_lock(&mutex_range);
		range = getRange((char*)args);
		pthread_mutex_unlock(&mutex_range);
		sleep(1);
	}
}

char note_str[3] = {0};
int write_note_res = -1;
pthread_mutex_t mutex_note = PTHREAD_MUTEX_INITIALIZER;

void *writeNoteFifoThread(void * args) {
	while (1) {
		if (!strlen(note_str)) {
			continue;
		}
		if (!pthread_mutex_trylock(&mutex_note)) {
			write_note_res = setNote((char*)args, note_str);
			pthread_mutex_unlock(&mutex_note);
		}
		note_str[0] = '\0';
	}
}


int button_0_state;
int button_1_state;
pthread_mutex_t mutex_button = PTHREAD_MUTEX_INITIALIZER;
typedef struct  {
	char button_0_fifo[16];
	char button_1_fifo[16];
} button_thread_args_t;
void *readButtonsFifoThread(void * args) {
	while (1) {
		pthread_mutex_lock(&mutex_button);

		button_thread_args_t *button_fifo = (button_thread_args_t *) args;

		button_0_state = getButtonState(button_fifo->button_0_fifo);
		button_1_state = getButtonState(button_fifo->button_1_fifo);
		pthread_mutex_unlock(&mutex_button);
		sleep(1);
	}

}


typedef struct {
	int notes_num;
	float max_range;
	float min_range;
	float range_step;
	pthread_mutex_t mutex_note_params;
} note_params_t;

note_params_t note_params = {.notes_num = 12,
                             .max_range = 0.1f,
                             .min_range = 0.0005f,
                             .range_step = (0.1f - 0.0005f) / 12,
                             .mutex_note_params = PTHREAD_MUTEX_INITIALIZER
                            };

char cmd_recv[64];
pthread_mutex_t mutex_cmd = PTHREAD_MUTEX_INITIALIZER;

char * cmd_stop = "stop";
char * cmd_start = "start";
char * cmd_set_min = "set_min";
char * cmd_set_max = "set_max";
char * rangefinder_prog_name = "rangefinder_hcsr04_IlorDash";
char * playnote_prog_name = "play_note__IlorDash";
char * all_prog_name = "all";

char *rangefinder_prog_args[] = {"-q", "1000", "range_fifo"};
char *playnote_prog_args[] = {"-q", "note_fifo"};

int start_prog = 0;

void *readCmdsFifoThread() {
	char cmd[16];
	char arg[32];
	while (1) {
		pthread_mutex_lock(&mutex_cmd);
		scanf("%16s %16s", cmd, arg);

		if (!strncmp(cmd, cmd_stop, strlen(cmd_stop))) {
			if (writeFifo(stop_prog_fifo_name, arg) < 0) {
				printf("Failed write stop_prog for %s\n", arg );
			}

		} else if (strncmp(cmd, cmd_start, strlen(cmd_start))) {
			if (!strncmp(arg, rangefinder_prog_name, strlen(rangefinder_prog_name))) {
				start_prog = 1;
			} else if (!strncmp(arg, playnote_prog_name, strlen(playnote_prog_name))) {
				start_prog = 2;
			} else if (!strncmp(arg, all_prog_name, strlen(all_prog_name))) {
				start_prog = 3;
			}

		} else if (!strncmp(cmd, cmd_set_min, strlen(cmd_set_min))) {
			pthread_mutex_lock(&note_params.mutex_note_params);
			note_params.min_range = atoi(arg);
			if ((note_params.min_range < 0) || (note_params.min_range >= note_params.max_range)) {
				printf("Failed get min range\n");
				continue;
			}
			note_params.range_step = (note_params.max_range - note_params.min_range) / note_params.notes_num;
			pthread_mutex_unlock(&note_params.mutex_note_params);

		} else if (!strncmp(cmd, cmd_set_max, strlen(cmd_set_max))) {
			pthread_mutex_lock(&note_params.mutex_note_params);
			note_params.max_range = atoi(arg);
			if ((note_params.max_range < 0) || (note_params.max_range <= note_params.min_range)) {
				printf("Failed get max range\n");
				continue;
			}
			note_params.range_step = (note_params.max_range - note_params.min_range) / note_params.notes_num;
			pthread_mutex_unlock(&note_params.mutex_note_params);
		}
		pthread_mutex_unlock(&mutex_cmd);
	}

}


int main(int argc, char *argv[]) {

	int volume = 1;

	char range_fifo[16] = {0};
	char note_fifo[16] = {0};
	button_thread_args_t button_thread_args;

	int quiet = 0;

	if (argc > 1) {
		if ((strcmp(argv[QUIT_HELP_OPT_POS], "-h") == 0)) {
			help();
			return 0;
		}
	}

	if (argc > ARG_CNT) {
		if ((strcmp(argv[QUIT_HELP_OPT_POS], "-q") == 0)) {
			quiet = 1;
		}

	} else {
		printf("Lack of arguments\n");
		return -1;
	}

	int range_fifo_arg = 1;

	if (quiet) {
		range_fifo_arg++;
	}

	strcpy(range_fifo, argv[range_fifo_arg]);

	int note_fifo_arg = range_fifo_arg + 1;
	strcpy(note_fifo, argv[note_fifo_arg]);

	int button_0_fifo_arg = note_fifo_arg + 1;
	strcpy(button_thread_args.button_0_fifo, argv[button_0_fifo_arg]);

	int button_1_fifo_arg = button_0_fifo_arg + 1;
	strcpy(button_thread_args.button_1_fifo, argv[button_1_fifo_arg]);

	if (!quiet) {
		printf("Received range_fifo = %s\nplay_note_fifo = %s\n", range_fifo, note_fifo);
	}

	pthread_t thread_stop;

	if (pthread_create(&thread_stop, NULL, readStopProgFifoThread, NULL) != 0) {
		fprintf(stderr, "error: thread_stop was failed\n");
		exit(-1);
	}

	pthread_t thread_range;

	if (pthread_create(&thread_range, NULL, readRangeFifoThread, (void*)range_fifo) != 0) {
		fprintf(stderr, "error: thread_range was failed\n");
		exit(-1);
	}

	pthread_t thread_note;

	if (pthread_create(&thread_note, NULL, writeNoteFifoThread, (void*) note_fifo) != 0) {
		fprintf(stderr, "error: thread_note was failed\n");
		exit(-1);
	}

	pthread_t thread_button;

	if (pthread_create(&thread_button, NULL, readButtonsFifoThread, & button_thread_args) != 0) {
		fprintf(stderr, "error: thread_button was failed\n");
		exit(-1);
	}

	pthread_t thread_cmd;

	if (pthread_create(&thread_cmd, NULL, readCmdsFifoThread, NULL) != 0) {
		fprintf(stderr, "error: thread_cmd was failed\n");
		exit(-1);
	}

	while (1) {

		if (!pthread_mutex_trylock(&mutex_cmd)) {
			if (start_prog > 0) {
				pid_t pid = fork();
				if (!pid) {
					printf("Child process\n");

					switch (start_prog) {
						case 1: {
							if (execvp(rangefinder_prog_name, rangefinder_prog_args) == -1) {
								printf("cannot run execvp %s\n", rangefinder_prog_name);
								exit(-1);
							}
							break;
						}

						case 2: {
							if (execvp(playnote_prog_name, playnote_prog_args) == -1) {
								printf("cannot run execvp %s\n", playnote_prog_name);
								exit(-1);
							}

							break;
						}

						case 3: {
							if (execvp(rangefinder_prog_name, rangefinder_prog_args) == -1) {
								printf("cannot run execvp %s\n", rangefinder_prog_name);
								exit(-1);
							}
							if (execvp(playnote_prog_name, playnote_prog_args) == -1) {
								printf("cannot run execvp %s\n", playnote_prog_name);
								exit(-1);
							}
							break;
						}

						default:
					}
				}

				pthread_mutex_unlock(&mutex_cmd);
			}



			if (!pthread_mutex_trylock(&mutex_stop)) {
				if (stop_prog_recv) {
					printf("Received stop prog\n");
					exit(0);
				}

				pthread_mutex_unlock(&mutex_stop);
			}

			int button_0_state_local;
			int button_1_state_local;

			pthread_mutex_lock(&mutex_button);
			button_0_state_local = button_0_state;
			button_1_state_local = button_1_state;
			pthread_mutex_unlock(&mutex_button);


			printf("button_0_state = %d, button_1_state = %d\n", button_0_state_local, button_1_state_local);

			if ((button_0_state_local < 0) || (button_1_state_local < 0)) {
				printf("Failed to get button states\n");
				exit(-1);
			}

			if (!button_0_state_local) {
				int min_range_tmp;
				pthread_mutex_lock(&mutex_range);
				min_range_tmp = range;
				pthread_mutex_unlock(&mutex_range);

				if (min_range_tmp < 0) {
					printf("Failed get min range\n");
					exit(-1);
				}

				pthread_mutex_lock(&note_params.mutex_note_params);
				note_params.min_range = min_range_tmp;
				note_params.range_step = (note_params.max_range - note_params.min_range) / note_params.notes_num;
				pthread_mutex_unlock(&note_params.mutex_note_params);

				if (!quiet) {
					time_t rawtime;
					struct tm * timeinfo;
					time ( &rawtime );
					timeinfo = localtime ( &rawtime );
					printf ( "Time %s, set min range %0.6f\n", asctime (timeinfo), note_params.min_range);
				}

			} else if (!button_1_state_local) {
				int max_range_tmp;

				pthread_mutex_lock(&mutex_range);
				max_range_tmp = range;
				pthread_mutex_unlock(&mutex_range);

				if (max_range_tmp < 0) {
					printf("Failed get max range\n");
					exit(-1);
				}

				pthread_mutex_lock(&note_params.mutex_note_params);
				note_params.max_range = max_range_tmp;
				note_params.range_step = (note_params.max_range - note_params.min_range) / note_params.notes_num;
				pthread_mutex_unlock(&note_params.mutex_note_params);

				if (!quiet) {
					time_t rawtime;
					struct tm * timeinfo;
					time ( &rawtime );
					timeinfo = localtime ( &rawtime );
					printf ( "Time %s, set max range %0.6f\n", asctime (timeinfo), note_params.max_range);
				}

			} else {
				int range_local;
				pthread_mutex_lock(&mutex_range);
				range_local = range;
				pthread_mutex_unlock(&mutex_range);

				if (range_local < 0) {
					printf("Failed get range\n");
					exit(-1);
				}

				pthread_mutex_lock(&mutex_note);
				pthread_mutex_lock(&note_params.mutex_note_params);

				if (range_local >= note_params.max_range) {
					sprintf(note_str, "A#");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - note_params.range_step)) {
					sprintf(note_str, "A");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - (2 * note_params.range_step))) {
					sprintf(note_str, "B");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - (3 * note_params.range_step))) {
					sprintf(note_str, "C#");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - (4 * note_params.range_step))) {
					sprintf(note_str, "C");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - (5 * note_params.range_step))) {
					sprintf(note_str, "D#");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - (6 * note_params.range_step))) {
					sprintf(note_str, "D");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if ((range_local >= (note_params.max_range - (7 * note_params.range_step)))) {
					sprintf(note_str, "E");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else if (range_local >= (note_params.max_range - (8 * note_params.range_step))) {
					sprintf(note_str, "F#");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else	if (range_local >= (note_params.max_range - (9 * note_params.range_step))) {
					sprintf(note_str, "F");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else	if (range_local >= (note_params.max_range - (10 * note_params.range_step))) {
					sprintf(note_str, "G#");
					//write(note_fd, note_str, strlen(note_str) + 1);

				} else	if (range_local < (note_params.max_range - (10 * note_params.range_step))) {
					sprintf(note_str, "G");
					//write(note_fd, note_str, strlen(note_str) + 1);
				}

				pthread_mutex_unlock(&note_params.mutex_note_params);
				pthread_mutex_unlock(&mutex_note);

				while (write_note_res < 0) {}

				pthread_mutex_lock(&mutex_note);
				if (write_note_res < 0) {
					printf("Failed set note\n");
					exit(-1);
				}
				pthread_mutex_unlock(&mutex_note);

				if (!quiet) {
					time_t rawtime;
					struct tm * timeinfo;
					time ( &rawtime );
					timeinfo = localtime ( &rawtime );
					printf ( "Time %s, note %s, volume %d\n", asctime (timeinfo), note_str, volume);
				}

				sleep(1);	// duration of note .wav
			}
		}

		return 0;
	}
}

void help() {
	printf("    Use this application for playing termenwoks\n");
	printf("    execute format: ./combiner RANGE_FIFO_NAME PLAY_NOTE_FIFO_NAME BUTTON_0_FIFO BUTTON_1_FIFO\n");
	printf("    -h - help\n");
	printf("    -q - quiet\n");
	printf("    RANGE_FIFO_NAME - name of created fifo rangefinder output\n");
	printf("    PLAY_NOTE_FIFO_NAME - name of created fifo play_note input\n");
	printf("    BUTTON_0_FIFO/BUTTON_1_FIFO - name of created fifos for buttons output\n");
	printf("\n    Available commands:\n");
	printf("    <stop [program name]> - stops program - rangefinder_hcsr04_IlorDash, play_note__IlorDash, all\n");
	printf("    <start [program name]> - starts program - rangefinder_hcsr04_IlorDash, play_note__IlorDash, all\n");
	printf("    <set_min [value]> - sets min range\n");
	printf("    <set_max [value]> - sets max range\n");
}
