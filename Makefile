all:main upload
main:HttpGo.cpp
	g++  $^ -o $@ -std=c++11  -lpthread
upload:upload.cpp HttpRequest.hpp
	g++  $^ -o $@ -std=c++11 
.PHONY:
clean:
	rm ./main ./upload
