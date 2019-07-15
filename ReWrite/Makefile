all:main upload 
main:Start.cc
	g++ $^ -o $@ -std=c++11 -lpthread `mysql_config --cflags --libs`
upload:upload.cc 
	g++  $^ -o $@ -std=c++11 `mysql_config --cflags --libs` 
	mv upload ./Web/
SQL:MySQL.hpp SQL.cc
	g++ $^ -o $@ -std=c++11 `mysql_config --cflags --libs` 
.PHONY:
clean:
	rm ./main ./Web/upload
