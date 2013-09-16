jp2png-web-cgi
==============

A jp2 decoder and PNG encoder based on CGI using openjpeg

Demo:
http://openjpeg.kbresearch.nl/jp2demo/index.php?localfile=ddd_010459732_mpeg21_p001_image.jp2


tested on [openjpeg2.0](http://code.google.com/p/openjpeg/downloads/detail?name=openjpeg-2.0.0.tar.gz&can=2&q=)

Build and install openjpeg
-----

1. cd /your/path/to/openjpeg2.0

2. sudo apt get install cmake make

3. cmake .

4. make

5. sudo make install

6. sudo ldconfig (alternatively set LD_LIBRARY_PATH to location of libopenjp2.so)

Deps:
-----
libpng-dev
