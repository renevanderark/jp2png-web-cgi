CGI_BIN = /usr/lib/cgi-bin

all: clean compile

test: unit integration

clean:
	rm -f jp2.cgi
	rm -f jp2-cache-clean

compile:
	gcc -I/usr/local/include/openjpeg-2.0/ lib/log.c lib/url2cache.c lib/opj_res.c lib/opj2png.c lib/urldecode.c jp2read.c -o jp2.cgi -fPIC -lm -lpng -lopenjp2 -lcurl -ljpeg  -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700
	gcc jp2-cache-clean.c -o jp2-cache-clean

unit:
	gcc -I/usr/local/include/openjpeg-2.0/ lib/log.c lib/url2cache.c lib/opj_res.c lib/opj2png.c lib/urldecode.c test/unit.c -o test/unit -fPIC -lm -lpng -ljpeg -lopenjp2 -lcurl -lcunit -std=c99 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -D_XOPEN_SOURCE=700
	test/unit

integration:
	cd test; ./jp2read-http.py; cd .. 

stress:
	test/parallel_stress.py --save-response

install:
	cp jp2.cgi $(CGI_BIN)/jp2.cgi
	cp jp2-cache-clean /usr/local/bin/jp2-cache-clean
	chmod 755 /usr/local/bin/jp2-cache-clean

uninstall:
	rm -f $(CGI_BIN)/jp2.cgi
	rm -f /usr/local/bin/jp2-cache-clean

        
