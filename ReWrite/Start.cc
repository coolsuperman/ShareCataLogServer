#include"HttpServer.hpp"


//Server Server::http_serv;

void Note(){
  std::cout<<"Add IP and Port!"<<std::endl;
}

int main(int argc,char* argv[]){
  if(argc!=3){
    Note();
    exit(-1);
  }
  Server* serv = Server::GetHttpServer();
  std::string ip = argv[1];
  unsigned short port= (unsigned short)atoi(argv[2]);
  signal(SIGPIPE,SIG_IGN);
  serv->InitServer(ip,port);
  serv->Go();
  return 0;
}
