all:main upload 
main:Start.cc
	g++ $^ -o $@ -std=c++11 -lpthread
upload:upload.cc 
	g++  $^ -o $@ -std=c++11 
	mv upload ./Web/
.PHONY:
clean:
	rm ./main ./Web/upload
