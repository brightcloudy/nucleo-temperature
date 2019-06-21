#!/usr/bin/python
import sqlite3
import sys
if len(sys.argv) != 2:
    sys.exit(1)

conn = sqlite3.connect(sys.argv[1])

c = conn.cursor()

times = []
temps = [[], [], []]
c.execute("select timestamp from tempdata where sensorid=0")
for row in c:
    times.append(int(row[0]))

c.execute("select MAX(sensorid) from tempdata")
numsensors = c.fetchone()[0]

for sensor in range(numsensors+1):
    c.execute("select timestamp, tempvalue from tempdata where sensorid=? and tempvalue not null", (sensor,))
    for row in c:
        temps[sensor].append(row[1])

for x,i in enumerate(times):
    outputrow = "%d" % i
    for s in range(numsensors+1):
        if temps[s][x] == -1000.0:
            outputrow += " ?"
        else:
            outputrow += " %f" % temps[s][x]
    print outputrow
