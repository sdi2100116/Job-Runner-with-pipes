#!/bin/bash


program_out1=$(./jobCommander poll queued)
program_out2=$(./jobCommander poll running)

if [ "$program_out1" = "No job waiting in line." ] && [ "$program_out2" = "No jobs running." ]; then #no jobs
    echo "No jobs to stop"
    exit 1
fi

if [ "$program_out1" = "No job waiting in line." ]; then #only jobs running
    while IFS= read -r word;
    do
        result=$(echo "$word" | cut -d',' -f1)
        result="${result:1}"
        ./jobCommander stop "$result"
        done <<< "$program_out2"
    exit 0
elif [ "$program_out2" = "No jobs running." ]; then #only jobs waiting
    while IFS= read -r word;
    do
        result=$(echo "$word" | cut -d',' -f1) #takes the string from the start to the ,
        result="${result:1}" #removes the first char (<)
        ./jobCommander stop "$result"
    done <<< "$program_out1" #takes the string program_out1 and cut it in words, based on the \n char
    exit 0
else    #jobs running and waiting

    while IFS= read -r word;
    do
        result=$(echo "$word" | cut -d',' -f1) #takes the string from the start to the ,
        result="${result:1}" #removes the first char (<)
        ./jobCommander stop "$result"
    done <<< "$program_out1" #takes the string program_out1 and cut it in words, based on the \n char


    while IFS= read -r word;
    do
        result=$(echo "$word" | cut -d',' -f1)
        result="${result:1}"
        ./jobCommander stop "$result"
    done <<< "$program_out2"
    exit 0
fi