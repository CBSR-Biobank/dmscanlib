#!/bin/bash

PROG="Linux-Debug/scanlib"
SQUARE_DEVS=(1 3 4 5 6 7 8 9 10 11 12 13 14 15)
THRESHOLDS=(5 10 15 20 25 30 35 40 45 50 55 6065 70 75 80 85 90 95 100)
GAPS=(0.06 0.065 0.07 0.075 0.08 0.085)
CORRECTIONS=(0 1 5 10 15 20)
CELLDIST=(0.32 0.325 0.33 0.335 0.34 0.345 0.35 0.355)
IMAGES=("$HOME/Desktop/problem_tube_images/5_scanned_20100409.1451.bmp")

for sqdev in "${SQUARE_DEVS[@]}"
do
    for thresh in "${THRESHOLDS[@]}"
    do
        for gap in "${GAPS[@]}"
        do
            for corr in "${CORRECTIONS[@]}"
            do
                for celldist in "${CELLDIST[@]}"
                do
                    for image in "${IMAGES[@]}"
                    do
                        img_basename=`basename $image`
                        log="${sqdev}_${thresh}_${gap}_${corr}_${celldist}_${img_basename}.log"
                        out=`$PROG --debug 9 -d -p 1 --square-dev $sqdev --threshold $thresh --gap $gap --corrections $corr --celldist $celldist -i $image | tee $log`
                        tubecnt=`cat scanlib.txt | wc -l`
                        tubecnt=`expr $tubecnt - 1`
                        echo "$sqdev,$thresh,$gap,$corr,$celldist,$image,$tubecnt,$out"
                    done
                done
            done
        done
    done
done
