#!/bin/bash
DBFILE=`ls -1t data/ | head -n 1`
./gnuplot-data.py "data/$DBFILE" | (cat > /dev/shm/gnudata && trap 'rm /dev/shm/gnudata' EXIT && gnuplot -c plotcmds)
