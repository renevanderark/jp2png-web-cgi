#!/bin/bash

gcc -I/usr/local/include/openjpeg-2.0/ lib/log.c lib/url2cache.c lib/opj_res.c lib/opj2png.c lib/urldecode.c test-pthreads.c -o experiment -fPIC -lm -lpng -lopenjp2 -lcurl -ljpeg -lpthread -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700


if [ $? -eq 0 ]
then
	sudo cp experiment /usr/lib/cgi-bin/experiment.cgi
	sudo tail -f /var/log/apache2/error.log
fi
