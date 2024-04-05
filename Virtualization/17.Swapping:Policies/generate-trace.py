#! /usr/bin/env python3
import random
from operator import itemgetter
import os

random.seed(0)
numaddrs = 10
maxpage = 10
count = {}
addrList = []
python_file_path = os.path.abspath(__file__)
file = open(os.path.join(os.path.dirname(python_file_path), "vpn.txt"), "w")
while(1):
    for i in range(numaddrs):
        n = int(maxpage * random.random())
        addrList.append(n)
        if n in count:
            count[n] += 1
        else:
            count[n] = 1

    sortedCount = sorted(count.items(), key=itemgetter(1), reverse=True)
    countSum = 0
    for k in range(int(0.2 * numaddrs)):
        countSum += sortedCount[k][1]

    if countSum / numaddrs >= 0.8:
        for i in range(numaddrs):
            file.write(str(addrList[i]) + "\n")
        break
    else:
        count = {}
        addrList = []

print(addrList)
file.close()