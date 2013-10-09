#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ lib/opj_url_stream.c lib/opj_res.c lib/opj2png.c lib/opj_memcached_stream.c test_memcached.c -fPIC -lm -lpng -lopenjp2 -lcurl -lmemcached -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
