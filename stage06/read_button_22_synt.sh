#!/bin/bash

while true
do
	curr_date=$(date +"%T.%6N")
	#curr_val=$(($RANDOM%2))
	curr_val=1
	echo $curr_date $curr_val > button_data_22
	sleep 1
done
