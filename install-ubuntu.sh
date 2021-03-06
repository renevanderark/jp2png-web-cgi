#/bin/bash

echo "INSTALL THE FOLLOWING PACKAGES: gcc cmake  make apache2 libpng-dev libjpeg-dev libcurl4-openssl-dev libcunit1-dev"
while true; do
	read -p "Are you sure? [y/n/s(kip)] " yn
	case $yn in
		[Yy]* ) 
			sudo apt-get install gcc cmake make apache2 libpng-dev libjpeg-dev libcurl4-openssl-dev libcunit1-dev
			sudo a2enmod cgi
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
			sudo make install
			break;;
		[Nn]* ) exit;;
		* ) echo "Please answer yes or no.";;
	esac
done


sudo mkdir /var/cache/jp2
sudo chown www-data /var/cache/jp2
echo "INSTALLATION PROCESS FINISHED"
echo "see README.md for more info"

