#!/usr/bin/env python
from collections import defaultdict

thrpt_dict= defaultdict(dict)
for system in ['orig', 'rtop']:
    with open(system,'r') as f:
        for line in f:
            write_per = int(line.split(' ')[0].split('ee')[1])
            thrpt = float(line.split(' ')[1].strip())
            thrpt_dict[system][write_per] = thrpt

for write_per in [1,2,5,10,20,40,100]:
    print write_per, thrpt_dict['orig'][write_per], thrpt_dict['rtop'][write_per]
