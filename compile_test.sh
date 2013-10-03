#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ opj_res.c opj2png.c opj_url_stream.c test2.c -fPIC -lm -lpng -lopenjp2 -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -lcurl -D_XOPEN_SOURCE=700
