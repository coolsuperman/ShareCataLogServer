#include"Tools.hpp"

#define MAX_HTTPHD 4096
#define FLODER "WWW"
#define MAX_PATH 256
#define MAX_BUFF 4096


class HttpRequest{
  private:
    int cli_sock;
    RequestInfo req_info;
    std::string header;
  public:
    HttpRequest(int sock):cli_sock(sock)
  {}
    bool RecvHttpHeader(RequestInfo& info){//接收http请求头
      char tmp[MAX_HTTPHD] = {0};
      while(1){
        int ret = recv(cli_sock,tmp,MAX_HTTPHD,MSG_PEEK);//返回值为0表示对端关闭链接，send触发SIGPIPE，对端关闭链接；
        if(ret<=0){
          if(errno==EINTR||errno==EAGAIN)//此次读取被信号打断，当前缓冲区没有就绪数据；
            continue;
          info.SetError("500");
          return false;
        }
        char* ptr = strstr(tmp,"\r\n\r\n");
        if(ptr==NULL&&ret==MAX_HTTPHD){
          info.SetError("413");//头部太大，返回413
          return false;
        }
        else if(ptr==NULL&&ret<MAX_HTTPHD){
          usleep(500);
          continue;
        }
        int hdr_len = ptr-(char*)tmp;
        header.assign(tmp,hdr_len);
        recv(cli_sock,tmp,hdr_len+4,0);//真正取掉；
        break;
      }
      return true;
    }
    bool PathIsLeagal (std::string& path,RequestInfo& info){
      std::string file = FLODER+path;
      std::cout<<file<<std::endl;
      if(stat(file.c_str(),&(info._st))<0){//判断文件是否存在
        std::cout<<" Path is illegal !"<<std::endl;
        info.SetError("404");
        return false;
      }
      //存在，转化为绝对路径；
      char ret[MAX_PATH] = {0};
      realpath(file.c_str(),ret);
      info._path_phys = ret;
      if(info._path_phys.find(FLODER)==std::string::npos){//判断转化的绝对路径里有根目录没？
        info.SetError("403");
        return false;
      }
      return true;
    }
    bool ParseHttpFirstLine(std::string line,RequestInfo& info){
      std::vector<std::string> line_list;
      if(Tools::Split(line," ",line_list)!=3){
        info.SetError("400");
        return false;
      }
      std::string URL;
      info._method = line_list[0];
      URL = line_list[1];
      info._version = line_list[2];
      if(info._method!="GET"&&info._method!="POST"&&info._method!="HEAD"){
        info.SetError("405");
        return false;
      }
      if(info._version!="HTTP/0.9"&&info._version!="HTTP/1.0"&&info._version!="HTTP/1.1"){
        info.SetError("400");
        return false;
      }
      size_t pos = URL.find('?');
      if(pos==std::string::npos)
        info._path_info = URL;
      else {
        info._path_info = URL.substr(0,pos);
        info._query_string = URL.substr(pos+1);
      }
      return PathIsLeagal(info._path_info,info);
    }
    bool ParseHttpHeader(RequestInfo& info){
      std::vector<std::string> pars_list;
      Tools::Split(header,"\r\n",pars_list);
      if(ParseHttpFirstLine(pars_list[0],info)==false)
        return false;
      for(size_t i = 1;i<pars_list.size();++i){
        size_t pos = pars_list[i].find(':');
        info.hd_list[pars_list[i].substr(0,pos)] = pars_list[i].substr(pos+2);
      }
      return true;
    }
};
