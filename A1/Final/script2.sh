#!/bin/bash
echo library call
for number in {0..10}
do
time ./scat -l < test.txt > /dev/null
done
echo system call
for number in {0..10}
do
time ./scat -s < test.txt > /dev/null
done
echo sendfile
for number in {0..10}
do
time ./scat -p < test.txt > /dev/null
done
exit 0

#
