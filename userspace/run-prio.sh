#!/bin/bash

echo &
./test-syscall 453 7 0 A 2 &
./test-syscall 453 5 1 B 1 &
./test-syscall 453 3 2 C 3 &
