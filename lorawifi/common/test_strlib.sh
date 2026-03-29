gcc -DLINUX_TEST strlib.c test_strlib.c -o strlib && valgrind -q ./strlib
#gcc -DLINUX_TEST strlib.c test_strlib.c -o strlib &&  ./strlib
rm strlib
