#!/usr/bin/env python
from string import *
import os, commands, getopt, sys


def Usage(mesg):
    print mesg + os.linesep

    print "Usage: " + os.linesep

    print "Mode 1. Compare output of 'lame1' and 'lame2':"
    print "./lametest.py  [options] options_file  input.wav lame1 lame2" + os.linesep

    print "Mode 2. Compare output of lame1 with reference solutions:"
    print "./lametest.py [options] options_file input.wav lame1" + os.linesep

    print "Mode 3. Generate reference solutions using lame1:"
    print "./lametest.py -m options_file input.wav lame1" + os.linesep

    print "options: "	
    print "   -w   convert mp3's to wav's before comparison"

    sys.exit(0)



##################################################################
# compare two files, return # bytes which differ and the
# number of bytes in larger file 
##################################################################
def fdiff(name1,name2):
    cmd = "cmp -l "+name1 + " " + name2 + " | wc -l"
    # XXX either
    #    use a combination of os.popen + read
    # or
    #    write owen cmp+wc function
    # to replace commands.getoutput
    out = commands.getoutput(cmd)
    out = split(out,"\n")
    out=out[-1]
    if (-1==find(out,"No")):
        diff = atof(out)

	status = os.access(name1, os.R_OK)
	if 1 != status:
	    size1=0
	else:
            size1 = os.path.getsize(name1)
        size2 = os.path.getsize(name2)
        diff = diff + abs(size1-size2)
        size = max(size1,size2) 
    else:
        diff = -1
        size = 0
    return (diff,size)

################################################################## 
# compare two files, return # bytes which differ and the
# number of bytes in larger file 
##################################################################
def compare(name1,name2,decode):
    if (decode):
        print "converting mp3 to wav for comparison..."
        # XXX shouldn't we use lame1 instead of a hardcoded lame?
        os.system("lame --quiet --mp3input --decode " + name1)
        os.system("lame --quiet --mp3input --decode " + name2)
        name1 = name1 + ".wav"
        name2 = name2 + ".wav"

    rcode = 0
    diff,size=fdiff(name1,name2)
    if (diff==0):
        print "output identical:  diff=%i  total=%i" % (diff,size)
        rcode = 1
    elif (diff>0):
	print "output different: diff=%i  total=%i  %2.0f%%" % \
        (diff,size,100*float(diff)/size)
    else:
	print "Error comparing files:"
        print "File 1: " + name1
        print "File 2: " + name2

    return rcode






################################################################## 
# main program
##################################################################
try:
    optlist, args = getopt.getopt(sys.argv[1:], 'wm')
except getopt.error, val:
    Usage('ERROR: ' + val)

decode=0
lame2="none"
for opt in optlist:
    if opt[0] == '-w':
        decode=1
    elif opt[0] == '-m':
        lame2="makeref"
        print os.linesep + "Generating reference output"


if len(args) < 3:
    Usage("Not enough arguments.")
if len(args) > 4:
    Usage("Too many arguments.")

if (lame2=="makeref"):
    if len(args) != 3:
    	Usage("Too many arguments for -r/-m mode.")
else:
    if len(args) ==3:	
        lame2="ref"

# populate variables from args and normalize & expand path
if len(args) >=3:
    options_file = os.path.normpath(os.path.expanduser(args[0]))
    input_file = os.path.normpath(os.path.expanduser(args[1]))
    lame1 = os.path.normpath(os.path.expanduser(args[2]))
if len(args) >=4:
    lame2 = os.path.normpath(os.path.expanduser(args[3]))

# check readability of options_file
status = os.access(options_file, os.R_OK)
if 1 != status:
    Usage(options_file + " not readable")

# check readability of input_file
status = os.access(input_file, os.R_OK)
if 1 != status:
    Usage(input_file + " not readable")


# generate searchlist of directories
path = split(os.environ['PATH'], os.pathsep)
path.append(os.curdir)

# init indicator vars
lame1_ok = 0
lame2_ok = 0

# check for executable lame1
for x in path:
    status = os.access(os.path.join(x, lame1), os.X_OK)
    if 1 == status:
        lame1_ok = 1
        break
if 1 != lame1_ok:
    Usage(lame1 + " is not executable")

if not (lame2=="ref" or lame2=="makeref"):
    # check for executable lame2
    for x in path:
        status = os.access(os.path.join(x, lame2), os.X_OK)
        if 1 == status:
            lame2_ok = 1 
            break
    if 1 != lame2_ok:
        Usage(lame2 + " is not executable")


tmp = split(options_file, os.sep)
tmp = tmp[-1]
basename = replace(input_file,".wav","")
basename = basename + "." + tmp


num_ok=0
n=0
foptions=open(options_file)
line = rstrip(foptions.readline())
while line:
    n = n+1
    name1 = basename + "." + str(n) + ".mp3"
    name2 = basename + "." + str(n) + "ref.mp3"

    print      # empty line

    if (lame2=='ref'):
        cmd = "rm -f "+name1
        os.system(cmd)
        cmd = lame1 + " --quiet " + line + " " + input_file + " " + name1
        print "executable:      ",lame1
        print "options:         ",line
        print "input:           ",input_file 
        print "reference output:",name2
        print cmd
        os.system(cmd)
        num_ok = num_ok+compare(name1,name2,decode)
    elif (lame2=='makeref'):
        cmd = "rm -f "+name2
        os.system(cmd)
        print "executable: ",lame1
        print "options:    ",line
        print "input:      ",input_file 
        print "output:     ",name2
        cmd = lame1 + " --quiet " + line + " " + input_file + " " + name2
        os.system(cmd)
    else:
        cmd = "rm -f "+name1
        os.system(cmd)
        cmd = "rm -f "+name2
        os.system(cmd)
        print "executable:  ",lame1
        print "executable2: ",lame2
        print "options:     ",line
        print "input:       ",input_file
        cmd1 = lame1 + " --quiet " + line + " " + input_file + " " + name1
        cmd2 = lame2 + " --quiet " + line + " " + input_file + " " + name2
        os.system(cmd1)
        os.system(cmd2)
	num_ok = num_ok + compare(name1,name2,decode)

    line = rstrip(foptions.readline())

foptions.close()

if lame2 != 'makeref':
    print os.linesep + "Number of tests which passed: ",num_ok
    print "Number of tests which failed: ",n-num_ok
