#!/bin/bash

# Define the directory containing the ip_port.txt files
directory="."

# Define the content file
content_file="content.txt"

# Loop through each ip_port.txt file
for file in "$directory"/*_*.txt; do
    # Extract the IP address and port number from the filename
    ip_port=$(basename "$file" .txt)
    ip_port=${ip_port%.*}  # Remove the .txt extension
    ip_port=${ip_port#*_}  # Remove everything before the underscore to get ip_port

    # Run diff command and display output
    diff "$file" "$content_file"
done
