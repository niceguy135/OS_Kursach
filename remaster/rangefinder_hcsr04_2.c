#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

int TRIG = 8; // GPIO PIN TRIG
int ECHO = 11; // GPIO PIN ECHO

void Exiting(int);

int read_pins_file(char *file)
{
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

static int GPIOExport(int pin)
{
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

static int GPIOUnexport(int pin)
{
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

static int GPIODirection(int pin, int dir)
{
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

static int GPIORead(int pin)
{
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

static int GPIOWrite(int pin, int value)
{
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

void Exiting(int parameter)
{
	GPIOUnexport(TRIG);
	GPIOUnexport(ECHO);
	exit(parameter);
}

void Exiting_sig()
{
	GPIOUnexport(TRIG);
	GPIOUnexport(ECHO);
	exit(0);
}

#define TIMEOUT_SEC 2

int main(int argc, char *argv[])
{

	int fd_fifo_0 = 0;		 					//дескриптор пайпа

	double search_time = 0;
	signal(SIGINT, Exiting_sig);
	GPIOExport(TRIG);
	GPIOExport(ECHO);
	sleep(0.05);
	GPIODirection(TRIG, OUT);
	GPIODirection(ECHO, IN);

	while (1) {
		char buff0[100] = {""};
		strcpy(buff0,"");

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
			continue;
		}
		double end_time = clock();
		search_time = (end_time - start_time)/CLOCKS_PER_SEC;

		char stringNum[50];
		sprintf(stringNum, "%f", search_time);

		strcpy(buff0, stringNum);

		
		if((fd_fifo_0=open("range_data_2", O_WRONLY)) == -1){ 

			printf("Can't open FIFO for write distance RF2");
            return -1;
		} else {

			write(fd_fifo_0, &buff0, strlen(buff0)+1);
			close(fd_fifo_0);
		}
	}
	return 0;
}
