autoheader && aclocal && autoconf && libtoolize --force && automake -a
echo "#################################"
echo "run ./configure --with-apr=/home/httpd-test/httpd"
