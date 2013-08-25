#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ opj_res.c opj2png.c jp2read.c -fPIC -lm -lpng -lopenjp2
sudo cp a.out /usr/lib/cgi-bin/jp2
sudo chmod a+x /usr/lib/cgi-bin/jp2
sudo cp test.html /var/www/testjp2.html

