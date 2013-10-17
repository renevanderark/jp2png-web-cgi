#/bin/bash

echo "INSTALL THE FOLLOWING PACKAGES: gcc make cmake httpd libpng-devel libcurl-devel"
while true; do
	read -p "Are you sure? [y/n/s(kip)] " yn
	case $yn in
		[Yy]* ) 
			sudo yum install gcc make cmake httpd libpng-devel libcurl-devel
			break;;
		[Ss]* ) break;;
		[Nn]* ) exit;;
		* ) echo "Please answer yes or no or skip.";;
	esac
done

echo "DOWNLOAD OPENJPEG2.0 SOURCES"
while true; do
	read -p "Are you sure? [y/n/s(kip)] " yn
	case $yn in
		[Yy]* )
			rm -rf openjpeg-2.0.0
			rm -f openjpeg-2.0.0.tar.gz
			wget 'http://openjpeg.googlecode.com/files/openjpeg-2.0.0.tar.gz'
			tar -xzvf openjpeg-2.0.0.tar.gz
			mv openjpeg-2.0.0/CMakeLists.txt openjpeg-2.0.0/CMakeLists.txt.1
			sed 's/cmake_minimum_required(VERSION 2.8.2)/cmake_minimum_required(VERSION 2.6.4)/g' openjpeg-2.0.0/CMakeLists.txt.1 > openjpeg-2.0.0/CMakeLists.txt
			break;;
		[Nn]* ) exit;;
		[Ss]* ) break;;
		* ) echo "Please answer yes or no.";;
	esac
done




echo "APPLY PATCH TO prevent OpenJPEG v2 issues as described in 'http://bugs.ghostscript.com/show_bug.cgi?id=693540#c2'"
while true; do
	read -p "Apply Patch? [y/s(kip)] " yn
	case $yn in
		[Yy]* ) cp lib/patch/j2k.c openjpeg-2.0.0/src/lib/openjp2/j2k.c; break;;
		[Ss]* ) break;;
		* ) echo "Please answer yes or no.";;
	esac
done

echo "INSTALLING OPENJPEG2.0 FROM SOURCE"
while true; do
	read -p "Are you sure? [y/n] " yn
	case $yn in
		[Yy]* ) 
			cd openjpeg-2.0.0
			cmake .
			make
			sudo make install
			sudo ldconfig
			sudo ln -s /usr/local/lib/libopenjp2.so.6 /usr/lib/libopenjp2.so.6
			cd ..
			break;;
		[Ss]* ) break;;
		[Nn]* ) exit;;
		* ) echo "Please answer yes or no.";;
	esac
done


echo "CONTINUE TO INSTALL THE WEBSERVICE TO YOUR cgi-bin"
while true; do
	read -p "Are you sure? [y/n] " yn
	case $yn in
		[Yy]* )
			make
			sudo make install CGI_BIN=/var/www/cgi-bin
			break;;
		[Nn]* ) exit;;
		* ) echo "Please answer yes or no.";;
	esac
done

sudo setsebool allow_httpd_anon_write on
sudo setsebool httpd_can_network_connect on
sudo apachectl start

sudo mkdir /var/cache/jp2
sudo chown apache /var/cache/jp2
sudo chmod 777 /var/cache/jp2
sudo chcon -R system_u:object_r:httpd_sys_content_t:s0 /var/cache/jp2

echo "INSTALLATION PROCESS FINISHED"
echo "see README.md for more info"

