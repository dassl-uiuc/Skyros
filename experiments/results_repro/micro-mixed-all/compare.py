#!/usr/bin/env python
from collections import defaultdict

thrpt_dict= defaultdict(dict)
for system in ['orig', 'rtop']:
    with open(system,'r') as f:
        for line in f:
            write_per = int(line.split(' ')[0].split('w')[1].split('.')[1]) *10
            thrpt = float(line.split(' ')[1].strip())
            thrpt_dict[system][write_per] = thrpt

for write_per in [10,30,50,70,90]:
    print write_per, thrpt_dict['orig'][write_per], thrpt_dict['rtop'][write_per]
