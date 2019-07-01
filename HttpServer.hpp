#include"ThreadPool.hpp"
#include"HttpRequest.hpp"

class ServSocket{
  private:
    std::string ip;
    unsigned short port;
    int local_socked;
  public:
    ServSocket(std::string ip_,unsigned short port_):ip(ip_),port(port_){}
    void CreatSocket(){
      local_socked =socket(AF_INET,SOCK_STREAM,0);
      if(local_socked<0){
        std::cerr<<"Socket filed!"<<std::endl;
        exit(-1);
      }
      int opt = 1;
      setsockopt(local_socked,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int));
    }
    void Bind(){
      sockaddr_in local;
      local.sin_family = AF_INET;
      local.sin_port = htons(port);
      local.sin_addr.s_addr = inet_addr(ip.c_str());
      if(bind(local_socked,(struct sockaddr*)&local,sizeof(local))<0){
        std::cerr<<"Bind Error!"<<std::endl;
        exit(-2);
      }
    }
    void Listen(){
      if(listen(local_socked,10)<0){       
        std::cerr<<"Listen Error!"<<std::endl;
        exit(-3);
      }
    }
    int Accept(){
      sockaddr_in client;
      socklen_t len = sizeof(client);
      int client_socket= accept(local_socked,(struct sockaddr*)&client,&len);
      if(client_socket<0)
        return -1;
      return client_socket;
    }
    int GetLocal(){
      return local_socked;
    }
};
class HttpServer{
  private:
    ThreadPool pool;//最好写成单例模式
    ServSocket socket;
  private:
    static bool HttpHandler (int sock){
      std::cerr<<"Now HttpHandler"<<std::endl;
      HttpRequest request(sock);
      HttpResponse response(sock);
      RequestInfo info;
      if(request.RecvHttpHeader(info)==false){//接收头部；
        goto out;
      }if(request.ParseHttpHeader(info)==false){//解析头部
        goto out;
      }
     // if(info.RequestIsCGI()){//判断是否是CGI请求
        //如果是，执行CGI响应；
       // response.CGIHandler(info);
     // }else{//不是则执行目录列表/文件下载响应
      std::cerr<<"Go FileHandler！"<<std::endl;
        response.FileHandler(info);
    // }
      close(sock);
      return true;
out:
      std::cerr<<"Try Send Err！"<<std::endl;
      response.ErrHandler(info);
      close(sock);
      return false;
    }
  public:
    HttpServer(std::string ip_,unsigned short port_,int capaciaty=10):pool(capaciaty),socket(ip_,port_)
  {}
    void TcpServerInit(){
      socket.CreatSocket();
      socket.Bind();
      socket.Listen();
      pool.PoolInit();
    }
    void GO(){
      while(1){
        int client_socket = socket.Accept();
        if(client_socket==-1)
          continue;
        std::cerr << "new connect\n";
        HttpTask tt(client_socket,HttpHandler);
        pool.AddTask(tt);
      }
    }
    ~HttpServer(){}
};

