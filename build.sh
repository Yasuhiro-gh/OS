mkdir _build
cd _build
g++ -c -Wall -g ../myfunc.cpp
g++ -c -Wall -g ../myprog.cpp
g++ -g -o myprog.exe myprog.o myfunc.o
cd ..