## UID: 505718918

## Pipe Up

Building a c script that can act as a pipe operator ( | ) where users can chain commands after ./pipe and it will pipe the input and output from each of the calls.

## Building

gcc -o pipe pipe.c
## Running

./pipe ls cat wc
OUTPUT: 4 4 39

./pipe ls cat wc wc
OUTPUT: 1 3 24

What is happening is that it is piping the output from each previous call to the current's call input until it outputs it.

## Cleaning up

make clean