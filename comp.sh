#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ lib/url2cache.c lib/opj_res.c lib/opj2png.c lib/urldecode.c test-pthreads.c -o experiment -fPIC -lm -lpng -lopenjp2 -lcurl -ljpeg  -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700
