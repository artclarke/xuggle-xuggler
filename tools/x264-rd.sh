#!/bin/sh

X264="../x264"
YUV="/usr/src/yuv/af-720x576.yuv"
OUT="/tmp/x264-$$.h264"

DAT="x264-rd.dat"

OPTS="-c"

# Init
rm -f "$DAT"
echo "#QP kb/s   PSNR Y     U     V     fps" > $DAT

for qp in `seq 1 51`
do
    LOG="/tmp/x264-$qp-$$.log"
    # clean
    rm -f "$LOG"
    # encode
    $X264 "$YUV" -o "$OUT" --qp $qp $OPTS 2> "$LOG"
    # gather stats
    cat "$LOG" |
    grep '^x264: overall' |
    sed 's/^x264: overall PSNR Y:\([[:digit:]]*\.[[:digit:]]*\) U:\([[:digit:]]*\.[[:digit:]]*\) V:\([[:digit:]]*\.[[:digit:]]*\) kb\/s:\([[:digit:]]*\.[[:digit:]]*\) fps:\([[:digit:]]*\.[[:digit:]]*\)$/\1 \2 \3 \4 \5/g' |
    awk -v QP=$qp '{ printf( "%2d %7.1f      %5.2f %5.2f %5.2f %5.3f\n", QP, $4, $1, $2, $3, $5 ); }' >> $DAT
done

# Clean
rm -f "$OUT"
rm -f "$LOG"

