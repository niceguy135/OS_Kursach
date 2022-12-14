#!/bin/bash

echo "22" > /sys/class/gpio/unexport
echo "22" > /sys/class/gpio/export
echo "in" > /sys/class/gpio/gpio22/direction

#mkfifo button_data_22

while true
do
	curr_date=$(date +"%T.%6N")
	curr_val=$(cat /sys/class/gpio/gpio22/value)
	echo $curr_date $curr_val > button_data_22
	sleep 1
done
