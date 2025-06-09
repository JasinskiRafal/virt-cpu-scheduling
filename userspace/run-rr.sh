#!/bin/bash

echo &
./test-syscall 452 7 0 A &
./test-syscall 452 5 1 B &
./test-syscall 452 3 2 C &
