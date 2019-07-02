#include"HttpServer.hpp"
#include<signal.h>

void Warn(){
  std::cout<<"Please add port!"<<std::endl;
} 

int main(int argc,char* argv[]){
  if(argc!=3){
    Warn();
    exit(-1);
  }
  std::string ip = argv[1];
  int port = atoi(argv[2]);
  HttpServer serv(ip,port);
  signal(SIGPIPE,SIG_IGN);
  serv.TcpServerInit();
  serv.GO();
  return 0;
}
