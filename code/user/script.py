import sys, os

DEST = "/sys/module/multi_flow/parameters/enabled_device"
MINORS = 128

out = ""

if len(sys.argv) == 1:
    print ("usage: ./script.py [minor1 minor2 ... minorN]")
    sys.exit(0)

for i in range(MINORS):

    if str(i) in sys.argv:
        out += "1"
    else: 
        out += "0"

    if i != MINORS-1: out += ","

os.system("echo " + out + " > " + DEST)