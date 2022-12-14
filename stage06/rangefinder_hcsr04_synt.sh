while true
do
	new_range=$(( $RANDOM % 1000000 ))
	echo "0.0"$new_range > rangefinder_val
	sleep 1
done
