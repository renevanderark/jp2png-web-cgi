#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ lib/opj_url_stream.c lib/opj_memcached_stream.c lib/opj_res.c lib/opj2png.c  test_memcached.c -fPIC -lm -lmemcached -lpng -lopenjp2 -lcurl -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
