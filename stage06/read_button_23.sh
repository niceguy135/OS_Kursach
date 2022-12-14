#!/bin/bash

echo "23" > /sys/class/gpio/unexport
echo "23" > /sys/class/gpio/export
echo "in" > /sys/class/gpio/gpio23/direction

#mkfifo button_data_23

while true
do
	curr_date=$(date +"%T.%6N")
	curr_val=$(cat /sys/class/gpio/gpio23/value)
	echo $curr_date $curr_val > button_data_23
	sleep 1
done
