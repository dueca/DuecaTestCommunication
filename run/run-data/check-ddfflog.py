import os
import sys
from pyddff import DDFFTagged

df = DDFFTagged('datalog.ddff')

for t in df.tags():
    print(t)

for k in df.keys():
    try:
        t, s, d = df[k].get_data()
        print("Data", k, "times", t[0], t[-1], s[0], d.keys())
        for k in d.keys():
            print(d[k])
    except TypeError:
        print("Cannot convert", k)
        try:
            for t, v in df[k].items():
                print(t, v)
        except KeyError:
            print("Cannot iterate", k)
