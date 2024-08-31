#!/bin/bash

if [ "$#" -eq 0 ]
then echo "No jobs in input"
exit 1
fi



for file in "$@"
do
    exec < "$file"
    while read line
    do  
        
        ./jobCommander issueJob "$line"
        sleep 0.05
    done
done
