#!/bin/bash

SCRIPT=`basename $0`

USAGE="$SCRIPT [DIR]

Generates NFO files for DataMatrix pallet images
"
DIR=$1
INFOTOOL="../Linux-imageinfo/dmscanlib"

if [ -z "$DIR" ]; then
    echo "ERROR: directory not specified"
    exit
fi

if [ ! -e "$INFOTOOL" ]; then
    echo "ERROR: info tool not found at $INFOTOOL"
    exit
fi

BMP_FILES=()
while read -r -d $'\0'; do
    BMP_FILES+=("$REPLY")
done < <(find $DIR/ -name \*.bmp -print0)

#echo $DIR
echo $BMP_FILES

for bmpfile in "${BMP_FILES[@]}"
do
    echo "generating file: ${bmpfile/bmp/nfo}"
    if [[ "$bmpfile" == *10x10* ]]
    then
        (${INFOTOOL} --decode --palletSize=10x10 ${bmpfile} > ${bmpfile/bmp/nfo} )
    else
        (${INFOTOOL} --decode ${bmpfile} > ${bmpfile/bmp/nfo})
    fi
done
