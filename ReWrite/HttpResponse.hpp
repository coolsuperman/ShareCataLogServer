#include"Tools.hpp"

class ResponseBasic{//响应基类
  public:
    ResponseBasic(int sock):_cli_sock(sock)
  {}
    virtual bool InitResponse(RequestInfo& req_info)=0;//初始化模块
    virtual bool ProccessRun(RequestInfo& info)=0;//组织响应模块
    virtual bool Response(RequestInfo& info)=0;//流程运行模块
  protected:
    bool SendData(const std::string &buf){
      if(send(_cli_sock,buf.c_str(),buf.size(),0)<0)
        return false;
      return true;
    }
    bool SendCData(const std::string &buf){
      if(buf.empty())
        SendData("0\r\n\r\n");
      std::stringstream ss;
      ss<<std::hex<<buf.size()<<"\r\n";
      SendData(ss.str());
      ss.clear();
      SendData(buf);
      SendData("\r\n");
      return true;
    }
  protected:
    int _cli_sock;
};
class Err_Send : public ResponseBasic{//错误码发送衍生类
  public:
    Err_Send(int socket):ResponseBasic(socket){}
    virtual bool InitResponse(RequestInfo& req_info)override{
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info)override{

    }
    virtual bool Response(RequestInfo& info)override{
      return true;
    }
};


class File_List : public ResponseBasic{//文件目录衍生类
  public:
    File_List(int socket):ResponseBasic(socket){}
    virtual bool InitResponse(RequestInfo& req_info)override{
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info){

    }
    virtual bool Response(RequestInfo& info)override{
      return true;
    }
};

class CGI_Upload : public ResponseBasic{//上传文件衍生类
  public:
    CGI_Upload(int socket):ResponseBasic(socket){}
    virtual bool InitResponse(RequestInfo& req_info)override{
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info){

    }
    virtual bool Response(RequestInfo& info)override{
      return true;
    }
};

class File_Download : public ResponseBasic{//文件下载衍生类
  public:
    File_Download(int socket):ResponseBasic(socket){}
    virtual bool InitResponse(RequestInfo& req_info)override{
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info){

    }
    virtual bool Response(RequestInfo& info)override{
      return true;
    }
};
