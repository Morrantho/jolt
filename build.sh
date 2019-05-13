gcc -c -Wall -Werror -fpic src/jlnet.c -o obj/jlnet.o
gcc -shared -o lib/libjlnet.so obj/jlnet.o -ljlstd
cp lib/libjlnet.so /usr/local/lib
chmod 0755 /usr/local/lib/libjlnet.so
ldconfig
unset LD_LIBRARY_PATH
cp -r include/jlnet /usr/local/include