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
#include <signal.h>

void help();


char note_fifo[16] = {0};

int writeFifo(char * fifoName, char * str) {
	int fifo_d = open(fifoName, O_WRONLY);
	if (fifo_d == -1) {
		printf ( "Failed to open named pipe = %s", fifoName);
		return -1;
	}

	write(fifo_d, str, strlen(str) + 1);
	close(fifo_d);
	return 0;
}

int readFifo(char * fifo_name, char * read_str, int str_len) {

	int fifo_d = open(note_fifo, O_RDONLY);

	if (fifo_d == -1) {
		printf ( "Failed to open named pipe = %s",  fifo_name);
		return -1;
	}

	read(fifo_d, read_str, str_len);
	close(fifo_d);
	return 0;
}

int sigint_recv = 0;

void sigfunc(int sig) {
	char c;
	printf("In sigfunc sig = %d\n", sig);
	if (sig == SIGINT) {
		if (writeFifo("stop_prog", "stop") < 0) {
			printf ( "Failed to open stop program pipe = stop_prog");
			return;
		}
		sigint_recv = 1;

	}
}

int main(int argc, char *argv[]) {

	signal(SIGINT, sigfunc);

	int quiet = 0;

	if (argc > 1) {
		if ((strcmp(argv[1], "-h") == 0)) {
			help();
			return 0;
		} else {
			if ((strcmp(argv[1], "-q") == 0)) {
				quiet = 1;
			}
		}
	} else {
		printf("Lack of arguments\n");
		return -1;
	}
	if (!quiet)
		printf("\nThe Notes application was started\n\n");

	int note_fifo_arg = 1;
	if (quiet) {
		note_fifo_arg++;
	}
	strcpy(note_fifo, argv[note_fifo_arg]);

	char buf[3];
	while (1) {
		readFifo(note_fifo, buf, 3);
		char playing_str[16];
		if (!quiet)
			printf("Received note %s\n", buf);

		if ((buf[0] == 'A') && (buf[1] != '#')) {
			system("aplay ./notes/A4.wav -q &");
			sprintf(playing_str, "%s", "Playing A#");
			//printf("Playing A#");
		}
		if (buf[0] == 'B') {
			system("aplay ./notes/B4.wav -q &");
			sprintf(playing_str, "%s", "Playing B");
			//printf("Playing B");
		}
		if ((buf[0] == 'C') && (buf[1] != '#')) {
			system("aplay ./notes/C4.wav -q &");
			sprintf(playing_str, "%s", "Playing C#");
			//printf("Playing C#");
		}
		if ((buf[0] == 'D') && (buf[1] != '#')) {
			system("aplay ./notes/D4.wav -q &");
			sprintf(playing_str, "%s", "Playing D#");
			//printf("Playing D#");
		}
		if (buf[0] == 'E') {
			system("aplay ./notes/E4.wav -q &");
			sprintf(playing_str, "%s", "Playing E");
			//printf("Playing E");
		}
		if ((buf[0] == 'F') && (buf[1] != '#')) {
			system("aplay ./notes/F4.wav -q &");
			sprintf(playing_str, "%s", "Playing F#");
			//printf("Playing F#");
		}
		if ((buf[0] == 'G') && (buf[1] != '#')) {
			system("aplay ./notes/G4.wav -q &");
			sprintf(playing_str, "%s", "Playing G#");
			//printf("Playing G#");
		}
		if ((buf[0] == 'A') && (buf[1] == '#')) {
			system("aplay ./notes/Ad4.wav -q &");
			sprintf(playing_str, "%s", "Playing A#");
			//printf("Playing A#");
		}
		if ((buf[0] == 'C') && (buf[1] == '#')) {
			system("aplay ./notes/Cd4.wav -q &");
			sprintf(playing_str, "%s", "Playing C#");
			//printf("Playing C#");
		}
		if ((buf[0] == 'D') && (buf[1] == '#')) {
			system("aplay ./notes/Dd4.wav -q &");
			sprintf(playing_str, "%s", "Playing D#");
			//printf("Playing D#");
		}
		if ((buf[0] == 'F') && (buf[1] == '#')) {
			system("aplay ./notes/Fd4.wav -q &");
			sprintf(playing_str, "%s", "Playing F#");
			//printf("Playing F#");
		}
		if ((buf[0] == 'G') && (buf[1] == '#')) {
			system("aplay ./notes/Gd4.wav -q &");
			sprintf(playing_str, "%s", "Playing G#");
			//printf("Playing G#");
		}
		if (!quiet) {
			printf("%s", playing_str);
			printf("\n");
		}
		fflush(stdout);
		if (sigint_recv) {
			sleep(1);
			exit(0);
		}
	}

	return 0;
}

void help() {
	printf("    Use this application for plaing notes\n");
	printf("    execute format: ./notes NOTE_FIFO_NAME\n");
	printf("    -h - help\n");
	printf("    -q - quiet\n");
	printf("    NOTE_FIFO_NAME - name of fifo with notes in scientific pitch notation\n");
	printf("    Example:\n");
	printf("    ./notes note_fifo\n");
	printf("    playing A#\n");
}
