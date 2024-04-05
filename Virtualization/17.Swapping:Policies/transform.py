#! /usr/bin/env python3

traceFile = open('./ls-trace.txt', 'r')
vpnFile = open('./vpn.txt', 'w')

# extract the VPN from the trace file
for line in traceFile:
    if (not line.startswith('=')):
        vpnFile.write(str((int("0x" + line[3:11], 16) & 0xfffff000) >> 12) + "\n")

traceFile.close()
vpnFile.close()