#!/bin/bash

# Define the paths to the user programs
user_program1="/path/to/user_program1"
user_program2="/path/to/user_program2"

# Initialize IP and starting port
ip_address="127.0.0.1"  # Default to localhost
starting_port=10000

# Loop to run the programs 12 times
for (( i=1; i<=12; i++ )); do
    echo "Running pair $i"

    # Calculate the ports for the current pair
    port1=$((starting_port + (i - 1) * 2))
    port2=$((port1 + 1))

    # Start user_program1 with delay
    ($user_program1 $ip_address $port1 $ip_address $port2 &)

    # Start user_program2 with delay
    sleep 1  # Adjust delay time as needed
    ($user_program2 $ip_address $port2 $ip_address $port1 &)
    sleep 1 # Delay 
done

echo "All pairs executed"
