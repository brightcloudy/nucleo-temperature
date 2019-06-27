#!/bin/bash
while [ true ];do
  DBFILE=`ls -1t data/ | head -n 1`
  for HRS in 1 6 24 144 720;do 
    ./gnuplot-data.py $[HRS*720] "data/$DBFILE" | (cat > /dev/shm/gnudata && trap 'rm /dev/shm/gnudata' EXIT && gnuplot -c plotcmds)
    mv plot.png `printf "plot-%03d.png" $HRS`
  done
  echo "Updated plots at $(date)"
  sleep 300
done
