jp2png-web-cgi
==============

A jp2 decoder and PNG encoder based on CGI using openjpeg

Demo:
http://openjpeg.kbresearch.nl/jp2demo2/index.html?urn=ddd:010461485:mpeg21:p001:image

tested on [openjpeg2.0](http://code.google.com/p/openjpeg/downloads/detail?name=openjpeg-2.0.0.tar.gz&can=2&q=)


INSTALLATION
============

This project includes an installer for ubuntu distribtions: install-ubuntu.sh - NO GUARANTEES

To compile this project has dependencies on the following maintained packages:
- libcurl4-*-dev
- libpng12-dev

And on the sources of the openjpeg2.0 project hosted via google code.

The project has been tested with 64-bits distro's of Ubuntu 12.04 and 32-bit CentOS 6.4


After successful compilation (make) the two binaries are built in the project directory:
- jp2.cgi
- jp2-cache-clean

To check the compiled cgi script from bash try the following (should output a JSON response):
export QUERY_STRING=f=./balloon.jp2; ./jp2.cgi

AFTER INSTALLATION
==================

After successful installation (sudo make install) the above binaries will have been copied to:
- /usr/lib/cgi-bin/jp2.cgi
- /usr/local/bin/jp2-cache-clean

To check for success try the following url (should respond with a valid JSON encoded error: "No resource specified"):
- http://localhost/cgi-bin/jp2.cgi
 
As per your apache2 configuration it might be necessary to add this (very insecure) entry to you httpd.conf or apache2.conf or suchlike:

```xml
<Directory "/usr/lib/cgi-bin">
    Options +ExecCGI
</Directory>
```



CACHING
-------

For the script to work a caching directory with read/write access has to be available for apache2 (the www-user). This directory defaults to:
- /var/cache/jp2

To change the caching location an environment variable needs to be configured through the cgi module (/etc/apache2/mods-enabled/cgi.load):
- SetEnv JP2_CACHEDIR "/path/to/cache"

Now the following url display a PNG file decoded from the sample image hosted on github:
- http://localhost/cgi-bin/jp2.cgi?u=https%3A%2F%2Fgithub.com%2Frenevanderark%2Fjp2png-web-cgi%2Fblob%2Fmaster%2Fballoon.jp2%3Fraw%3Dtrue&t=1&r=3

Keep your cache dir clean. That's what the other binary is for. Test it out like so:
- ./jp2-cache-clean

Add an entry to your crontab (somewhat) like the following to clean the cache every minute:
- */1 * * * * /usr/local/bin/jp2-cache-clean --cache-dir /var/cache/jp2 --max-size 1024 2>> /var/log/jp2-cache-clean.log


API
---
The cgi scripts supports the following parameters (setting either 't' or 'u' shows only the header information in JSON):
- 'f': reference to a local filename of a.jp2 file
- 'u': url encoded reference to a remote filename (URL) of a.jp2 file
- 't': the tile to be decoded (now the response should become a .png file)
- 'r': resolution level (which is a reduction factor when decoding)
- 'x': x-position within the decoded tile
- 'y': y-position within the decoded tile
- 'w': width of the decoded tile (x+w should not exceed the total width of the tile)
- 'h': height of the decoded tile (y+h should not exceed the total height of the tile)
- 'n': number of compositions of the tile (max supported is shown in the jp2 header; 1 becomes grayscale in all cases)
- 'j': when added the service returns a JPEG encoded tile in stead of a PNG encoded tile
- 'c(allback)': jsonp callback for remote origin website JSON support

DEMO
----
Skip all that and symlink the demo like this:
- cd /var/www
- sudo ln -s /path/to/project/demo
- open a browser and surf to: http://localhost/demo/


LOGGING
-------
Logging of the web service is directed by default to the apache error log (/var/log/apache2/error.log)

There are two logging settings available which can be changed through Apache's environment variables (editable through /etc/apache2/mods-enabled/cgi.load). These flags are optional:
- SetEnv JP2_VERBOSE 1
- SetEnv JP2_SUPPRESS_ERRORS 1


