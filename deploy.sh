#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ opj_res.c opj2png.c urldecode.c jp2read.c -fPIC -lm -lpng -lopenjp2 -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
sudo cp a.out /usr/lib/cgi-bin/jp2
sudo chmod a+x /usr/lib/cgi-bin/jp2
sudo cp test.html /var/www/testjp2.html
sudo cp balloon.jp2 /var/www/balloon.jp2
