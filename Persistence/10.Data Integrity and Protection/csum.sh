#!/bin/bash

FILENAME="./test-csum.txt"
CHECKSUM="./checksum.txt"
rm $FILENAME $CHECKSUM

echo "Test csum" >> $FILENAME

make create-csum

./create-csum $FILENAME $CHECKSUM
./check-csum  $FILENAME $CHECKSUM   

echo ""
echo "manually corrupt the file"
echo ""
echo "add a line" >> $FILENAME

./check-csum  $FILENAME $CHECKSUM