/*******************************************************************************
 * Copyright (c) 2022 Sergey Balabaev (sergei.a.balabaev@gmail.com)                      *
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
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define VAR7_1

#ifdef VAR2
int TRIG = 8; // GPIO PIN TRIG
int ECHO = 11; // GPIO PIN ECHO
#endif

#ifdef VAR7_1
int TRIG = 26; // GPIO PIN TRIG
int ECHO = 27; // GPIO PIN ECHO
#endif

#ifdef VAR7_2
int TRIG = 8; // GPIO PIN TRIG
int ECHO = 11; // GPIO PIN ECHO
#endif

#ifdef VAR9
int TRIG = 8; // GPIO PIN TRIG
int ECHO = 11; // GPIO PIN ECHO
#endif

void Exiting(int);

int read_pins_file(char *file) {
	FILE *f = fopen(file, "r");
	if (f == 0) {
		fprintf(stderr, "ERROR: can't open %s file\n", file);
		return -1;
	}

	char str[32];
	while (!feof(f)) {
		if (fscanf(f, "%s\n", str))
			printf("%s\n", str);
		fflush(stdout);
		sleep(1);
	}
	fclose(f);

	return 0;
}

static int GPIOExport(int pin) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		Exiting(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

static int GPIOUnexport(int pin) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		Exiting(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";

#define DIRECTION_MAX 35
	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		Exiting(-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3],
	                IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		Exiting(-1);
	}

	close(fd);
	return (0);
}

static int GPIORead(int pin) {
#define VALUE_MAX 30
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		Exiting(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		Exiting(-1);
	}

	close(fd);

	return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		Exiting(-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		Exiting(-1);
	}

	close(fd);
	return (0);
}

void Exiting(int parameter) {
	GPIOUnexport(TRIG);
	GPIOUnexport(ECHO);
	exit(parameter);
}

void Exiting_sig() {
	GPIOUnexport(TRIG);
	GPIOUnexport(ECHO);
	exit(0);
}

void help() {
	printf("    Use this application for reading from rangefinder\n");
	printf("    execute format: ./range TIME FIFO_NAME \n");
	printf("    return: length in cm\n");
	printf("    TIME - pause between writing in ms\n");
	printf("    FIFO_NAME - path to created fifo for reading length from rangefinder\n");
	printf("    -h - help\n");
	printf("    -q - quiet\n");
}


char  named_pipe[16] = {0};

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

#define TIMEOUT_SEC 2
#define QUIT_HELP_OPT_POS 1

int main(int argc, char *argv[]) {

	signal(SIGINT, sigfunc);

	int quiet = 0;

	bool named_pipe_exist = false;
	if (argc > QUIT_HELP_OPT_POS) {
		if ((strcmp(argv[QUIT_HELP_OPT_POS], "-h") == 0)) {
			help();
			return 0;
		} else {
			if ((strcmp(argv[QUIT_HELP_OPT_POS], "-q") == 0)) {
				quiet = 1;
			}
		}
	}

	int sl;

	int sl_argument = 1;
	if (quiet) {
		sl_argument++;
	}
	sl = atoi(argv[sl_argument]);
	int fifo_name_argument = sl_argument + 1;
	strcpy(named_pipe, argv[fifo_name_argument]);

	if (!quiet)
		printf("named_pipe = %s\n", named_pipe);



	//if ((quiet && argc != 3) || (!quiet && argc != 2)) {
	//	help();
	//	return 0;
	//}


	if (!quiet)
		printf("\nThe rangefinder application was started\n\n");

	double search_time = 0;
	signal(SIGINT, Exiting_sig);
	GPIOExport(TRIG);
	GPIOExport(ECHO);
	sleep(0.05);
	GPIODirection(TRIG, OUT);
	GPIODirection(ECHO, IN);

	while (1) {
		GPIOWrite(TRIG, 1);
		usleep(10);
		GPIOWrite(TRIG, 0);
		while (!GPIORead(ECHO)) {
		}
		double start_time = clock();
		int flag = 0;
		while (GPIORead(ECHO)) {
			if (clock() - start_time >
			    TIMEOUT_SEC * CLOCKS_PER_SEC) {
				flag = 1;
				break;
			}
		}
		if (flag) {
			printf("Timeout reached, sleeping for one second\n");
			sleep(1);
			return 0;
		}
		double end_time = clock();
		search_time = (end_time - start_time) / CLOCKS_PER_SEC;

		time_t rawtime;
		struct tm * timeinfo;

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

		char range_str[32] = {0};
		sprintf(range_str, "%0.6f\n", search_time);

		if (writeFifo(named_pipe, range_str) < 0) {
			return -1;
		}

		if (!quiet)
			printf("signal_delay: %lf\nTime = %s", search_time, asctime(timeinfo));
		else
			printf("%lf\n", search_time);

		fflush(stdout);

		if ((sl > 0) && (sl < 60000))
			usleep(sl * 1000);
		else
			sleep(1);

		if (sl == 0) {
			//close(fifo_d);
			return 0;
		}
		if (sigint_recv) {
			exit(0);
		}
	}

	//close(fifo_d);
	return 0;
}
