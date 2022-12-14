#!/bin/bash

#mkfifo range_fifo
#mkfifo note_fifo

#mkfifo button_data_22
#mkfifo button_data_23

sudo ./read_button_22.sh &
sudo ./read_button_23.sh &

sudo ./rangefinder_hcsr04_IlorDash -q 1000 range_fifo &
./play_note_IlorDash -q note_fifo &

./combiner_IlorDash range_fifo note_fifo button_data_22 button_data_23
