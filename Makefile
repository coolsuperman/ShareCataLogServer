all:main
main:HttpGo.cpp
	g++  $^ -o $@ -std=c++11  -lpthread
.PHONY:
clean:
	rm ./main
