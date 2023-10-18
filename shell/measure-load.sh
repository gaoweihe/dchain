#!/bin/bash

LOGFILE="/home/gaowh/load.log"

while true; do
    # Capture the date and time
    echo "Timestamp: $(date)" >> $LOGFILE

    # Log CPU stats
    echo "CPU Stats:" >> $LOGFILE
    vmstat 1 2 | tail -n 1 >> $LOGFILE

    # Log Network stats for interface eth0 (change accordingly)
    echo "Network Stats (for eth0):" >> $LOGFILE
    nload -o csv -t 1000 -i 1024 -u K eth0 | tail -n 1 >> $LOGFILE

    echo "------------------------------------" >> $LOGFILE

    # Sleep for 5 minutes
    sleep 300
done
