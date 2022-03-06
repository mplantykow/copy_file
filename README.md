## copy_file
Simple Linux cp c++ implementation, that measures data transfer speed and performs MD5 based corectness check.

## Requirements
dnf install boost-devel libpcap-devel openssl-devel

## Compilation
g++ copy_file.cpp -o copy_file -lcrypto -lboost_iostreams -lpthread

## Usage
./copy_file <src-file> <dst-file>
