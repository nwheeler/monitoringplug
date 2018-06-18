#!/bin/bash

./autogen.sh
./configure
make
make deb
