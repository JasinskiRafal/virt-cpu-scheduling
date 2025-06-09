#!/bin/bash

echo &
./test-syscall 450 7 0 A &
./test-syscall 450 5 1 B &
./test-syscall 450 3 2 C &
