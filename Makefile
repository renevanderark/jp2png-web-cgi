all: clean compile

clean:
	rm -f jp2.cgi
	rm -f jp2-cache-clean

compile:
	gcc -I/usr/local/include/openjpeg-2.0/ lib/url2cache.c lib/opj_res.c lib/opj2png.c lib/urldecode.c jp2read.c -o jp2.cgi -fPIC -lm -lpng -lopenjp2 -lcurl -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700
	gcc jp2-cache-clean.c -o jp2-cache-clean

unit:
	gcc -I/usr/local/include/openjpeg-2.0/ lib/url2cache.c lib/opj_res.c lib/opj2png.c lib/urldecode.c test/unit.c -o test/unit -fPIC -lm -lpng -lopenjp2 -lcurl -lcunit -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700
	test/unit

install:
	cp jp2.cgi /usr/lib/cgi-bin/jp2
	cp jp2-cache-clean /usr/local/bin/jp2-cache-clean
	chmod 755 /usr/local/bin/jp2-cache-clean

uninstall:
	rm -f /usr/lib/cgi-bin/jp2
	rm -f /usr/local/bin/jp2-cache-clean


