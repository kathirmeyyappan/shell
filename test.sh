#!/bin/bash


testname=$1
BATCHPATH=./batch-files
OUTPUTPATH=./expected-outputs

mkdir out
./shell $BATCHPATH/$testname > out/$testname.stdout

echo "Testing $testname stdout"
diff out/$testname.stdout $OUTPUTPATH/$testname.stdout

rm -rf out

exit