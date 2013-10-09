all: clean compile

clean:
	rm -f jp2.cgi

compile:
	gcc -I/usr/local/include/openjpeg-2.0/ lib/opj_url_stream.c lib/opj_memcached_stream.c lib/opj_res.c lib/opj2png.c lib/urldecode.c jp2read.c -o jp2.cgi -fPIC -lm -lpng -lopenjp2 -lcurl -lmemcached -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700

install:
	cp jp2.cgi /usr/lib/cgi-bin/jp2

uninstall:
	rm -f /usr/lib/cgi-bin/jp2
