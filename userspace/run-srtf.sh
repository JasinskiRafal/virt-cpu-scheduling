#!/bin/bash

echo &
./test-syscall 451 7 0 A &
./test-syscall 451 5 1 B &
./test-syscall 451 3 2 C &
