#include"Threadpool.hpp"
#include"HttpRequest.hpp"
#include"HttpResponse.hpp"

class Socket{//Socket创建模块；
  private:
    int local_socket;
    std::string ip;
    unsigned short port;
  public:
    Socket(std::string IP="",unsigned short p=0)
      :ip(IP),port(p)
    {}
    void Set(std::string IP,unsigned short p){
      ip = IP;
      port = p;
    }
    bool CreatSocket(){
      local_socket = socket(AF_INET,SOCK_STREAM,0);
      if(local_socket<0){
        std::cerr<<"CreatSocket Fial!"<<std::endl;
        return false;
      }
      int opt = 1;
      setsockopt(local_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int));//允许地址重用；
      return true;
    }
    bool Bind(){
      sockaddr_in local;
      local.sin_family = AF_INET;
      local.sin_port = htons(port);
      local.sin_addr.s_addr = inet_addr(ip.c_str());
      if(bind(local_socket,(struct sockaddr*)&local,sizeof(local))<0){
        std::cerr<<"Bind Fial!"<<std::endl;
        return false;
      }
      return true;
    }
    bool Listen(){
      if(listen(local_socket,10)<0){
        std::cerr<<"Listen Fail!"<<std::endl;
        return false;
      }
      return true;
    }
    int Accept(){
      sockaddr_in client;
      socklen_t len = sizeof(client);
      int client_socket=accept(local_socket,(struct sockaddr*)&client,&len);
      if(client_socket<0)
        return -1;
      return client_socket;
    }
    int LocalSocket(){
      return local_socket;
    }
};

class Server{//服务器模块--饿汉模式
  private:
    Socket sock;
    ThreadPool pool;
    static Server http_serv;
    Server()
    {};
    Server(const Server&)=delete;
    Server& operator=(const Server&)=delete;
  public:
    static Server* GetHttpServer(){
      return &http_serv;
    }
    bool InitServer(std::string ip,unsigned short port){
      sock.Set(ip,port);
      if(sock.CreatSocket()){
        if(sock.Bind()){
          if(sock.Listen()){
            pool.PoolInit();
            return true;
          }
        }
      }
      std::cerr<<"SeverInit Fail!"<<std::endl;
      return false;
    }
    static void Err(int socket,RequestInfo& info){
      Err_Send err(socket);
      GoWork(err,info);
      close(socket);
    }
    static bool GoWork(ResponseBasic& work,RequestInfo& info){//多态运行响应模块
      return work.Response(info);
    }
    static bool Header(int socket){
      HttpRequest request(socket);
      RequestInfo info;
      if(request.RecvHttpHeader(info)==true){
        if(request.ParseHttpHeader(info)==true){//都正确，进入响应
          if(info.RequestIsCGI()){
            CGI_Upload upload(socket);
            if(GoWork(upload,info)==false){
              Err(socket,info);
              return false;
            }
          }else{
            if(Tools::IsDir(info)){
              File_List plist(socket);
              if(GoWork(plist,info)==false){
                Err(socket,info);
                return false;
              }
            }else{
              File_Download dw(socket);
              if(GoWork(dw,info)==false){
                std::cerr<<"DW-GoWork--false"<<std::endl;
                Err(socket,info);
                return false;
              }
            }
          }
          close(socket);
          std::cerr<<"正确返回"<<std::endl;
          return true;
        }
      }
      Err(socket,info);
      return false;
    }
    void Go(){
      while(1){
        int client_socket=sock.Accept();
        if(client_socket==-1)
          continue;
        std::cerr<<"New Client Add!"<<std::endl;
        Task tt(client_socket,Header);
        pool.AddTask(tt);
      }
    }
};

Server Server::http_serv;
