#!/bin/sh

#
# checkout lame
# run python scripts
# mail output
#

CVS_RSH=/home/mt/bin/sshcvs
LAME_DIR=/home/mt/mp3/lame_cvscheck
TESTCASE=/home/mt/mp3/test/castanets.wav
export OUTPUT=/tmp/cvscheck.out
export TO="lame-cvs@lists.sourceforge.net"

cd ${LAME_DIR}
#cvs -z3 -dmarkt@cvs.lame.sourceforge.net:/cvsroot/lame co -d lame_cvscheck lame
cvs update -P -d
if [ $? != 0 ]; then
    echo "Error running CVS update. cvscheck script exiting."
    exit 1
fi

if [ -f ${OUTPUT} ]; then
    mv -f ${OUTPUT} ${OUTPUT}.old
fi
rm -f frontend/lame
rm -f config.cache
./configure --enable-debug
make clean
make

if [ $? != 0 ]; then
    echo "Error compiling code..." > ${OUTPUT}
else
    test/lametest.py test/CBRABR.op ${TESTCASE} frontend/lame > ${OUTPUT}
fi

# check if there are failed tests
if grep >/dev/null 2>&1 "Number of tests which failed:  0" ${OUTPUT} ; then
    echo "No failed tests."
else
    # yes, failed tests, send output

    if diff >/dev/null 2>&1 -bBiq ${OUTPUT}.old ${OUTPUT} ; then
        export MSG='No change since last failed test(s).'
    else
        export MSG='Another change since last failed test(s)!'
        cat ${OUTPUT}; echo "${MSG}"
    fi

    ( cat ${OUTPUT}; echo "${MSG}" ) | mail -s "Automated lame test" ${TO}
fi

