#include"Tools.hpp"
#include<time.h>
#include<sstream>
#include<dirent.h>

std::unordered_map<std::string,std::string> err_desc={
  {"200","OK"},
  {"400","Bad Request"},
  {"403","Forbidden"},
  {"404","Not Found"},
  {"405","Method Nod Allowed"},
  {"413","Request Entity Too Large"},
  {"500","Internal Server Error"},
};

std::unordered_map<std::string,std::string> type ={
  {"txt","text/plain"},
  {"html","text/html"},
  {"htm","text/html"},
  {"jpg","image/jpeg"},
  {"zip","application/zip"},
  {"mp3","audio/mpeg"},
  {"unkonw","application/octet-stream"},
};

class Tools{
  public:
    static int Split(std::string& src,const std::string&search,std::vector<std::string>& list){//将头的每一行放入一个vector中
      int num = 0;
      size_t start = 0;
      size_t end = 0;
      while(start<src.size()){
        end = src.find(search,start);
        if(end==std::string::npos)
          break;
        list.push_back(src.substr(start,end-start));
        num++;
        start = end+search.size();
      }
      if(start<src.size()){
        list.push_back(src.substr(start));
        num++;
      }
      return num;
    }
    static const std::string GetErro_desc(std::string code){
      //>
      auto pos = err_desc.find(code);
      if(pos==err_desc.end())
        return "Unknow Error";
      return pos->second;
    }
    static void TimeToGMT(time_t t,std::string gmt){ 
      struct tm* mt = gmtime(&t);//将普通时间转换为格林威治时间；
      char tmp[128]={0};
      int len = strftime(tmp,127,"%a, %d %b %Y %H:%M:%S GMT",mt);
      gmt.assign(tmp,len);
    }
    static void DigitToStr(int64_t num,std::string& str){
      std::stringstream ss;
      ss<<num;
      str=ss.str();
    }
    static int64_t StrToDigit(std::string &str){
      int64_t num;
      std::stringstream ss;
      ss<<str;
      ss>>num;
      return num;
    }
    static void MakeETag(int64_t size,int64_t ino,int64_t mtime,std::string& etag){
      std::stringstream ss;
      //"ino-size-mtime"
      ss<<"\""<<std::hex<<ino<<"-"<<std::hex<<size<<"-"<<std::hex<<mtime<<"\"";
      etag = ss.str();
    }
    static void GetMime(const std::string &file,std::string &mime){//通过文件名获取文件类型；
      size_t pos;
      pos = file.find_last_of(".");
      if(pos==std::string::npos){
        mime=type["unknow"];
        return;
      }
      std::string suffix = file.substr(pos+1);
      auto it = type.find(suffix);//检查这个类型有没；
      if(it==type.end()){
        mime=type["unkonw"];
      }else{
        mime = it->second;
      }
    }
};


class RequestInfo{
  //包含HttpRequest解析出的请求信息；
  public:
    std::string _method="NULLL";//请求方法
    std::string _version="NULL";//协议版本
    std::string _path_info="NULL";//资源路径
    std::string _path_phys="NULL";//资源实际路径
    std::string _query_string="NULL";//查询字符串
    std::unordered_map<std::string,std::string>_hdr_list;//存储头信息中的所有键值对；
    struct stat _st;//获取其他文件信息；大小，inode。之类；
  public:
    std::string _err_code;//错误码；
    void SetError(const std::string &code){
      _err_code = code;
    }
  public:
    bool RequestIsCGI(){
      if((_method=="GET"&&!_query_string.empty())||(_method=="POST")){
        return true;
      }
      return false;
    }
    void display(){
      std::cout<<"~_version: "<<_version<<std::endl<<"~_path_info: "<<_path_info<<std::endl<<"~_path_phys: "<<_path_phys<<std::endl<<"~_query_string: "<<_query_string<<std::endl;
    }
};
# define MAX_HTTPHDR 4096
#define WWWROOT "WWW"
#define MAX_PATH 256
#define MAX_BUFF 4096

class HttpRequest{
  private:
    int _cli_sock;
    RequestInfo _req_info;
    std::string header;
  public:
    HttpRequest(int sock):_cli_sock(sock)
  {}
    bool RecvHttpHeader(RequestInfo& info){//接收http请求头
      std::cerr<<"Now 接收http请求头"<<std::endl;
      char tmp[MAX_HTTPHDR]={0};
      while(1){
        std::cerr<<"In RecvHeader Loop"<<std::endl;
        int ret = recv(_cli_sock,tmp,MAX_HTTPHDR,MSG_PEEK);//返回值为0表示对端关闭链接，send触发SIGPIPE信号，对端关闭链接；
        if(ret<=0){
          if(errno==EINTR|| errno==EAGAIN){//此次读取被信号打断、当前缓冲区没有就绪数据；
            continue;
          }
          info.SetError("500");
          return false;
        }
        char* ptr = strstr(tmp,"\r\n\r\n");
        if(ptr==NULL&&ret==MAX_HTTPHDR){
          info.SetError("413");//头部实体太大，返回413；
          return false;
        }else if(ptr==NULL&&ret<MAX_HTTPHDR){//这不应该是得到了吗？不一定每个都一样大啊
          usleep(1000);
          continue;
        }
        //>
        int hdr_len = ptr-(char*)tmp;
        header.assign(tmp,hdr_len);
        recv(_cli_sock,tmp,hdr_len+4,0);//真正取了，把后面的四个字符也取了
        std::cout<<"HHHHH:"<<header<<std::endl;
        break;
      }
      return true;
    }
    bool ParseHttpFirstLine(std::string& line,RequestInfo& info ){
      //std::cout<<line<<std::endl;
      std::vector<std::string> line_list;
      if(Tools::Split(line," ",line_list)!=3)
      {
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
      //std::cout<<info._version<<std::endl;
      if(info._version!="HTTP/0.9"&&info._version!="HTTP/1.0"&&info._version!="HTTP/1.1"){
        info.SetError("400");
        return false;
      }
      //解析URL：/upload?key=val&val=val...
      size_t pos=0;
      pos = URL.find('?');//没有？就说明没有参数，是个访问请求；
      if(pos == std::string::npos){
        info._path_info = URL;
      }else{
        info._path_info = URL.substr(0,pos);
        info._query_string = URL.substr(pos+1);//把请求参数写进这个变量里；
      }
      return PathIsLeagal(info._path_info,info);
    }
    bool PathIsLeagal(std::string& path,RequestInfo& info){
      std::string file = WWWROOT+path;
     // std::cout<<file<<std::endl;
      if(stat(file.c_str(),&(info._st))<0){//判断该文件存在否？
        std::cout<<"248Path Is Legal 404"<<std::endl;
        info.SetError("404");
        return false;
      }
      //文件存在的话，就会将相对路径转化为绝对路径；
      char tmp[MAX_PATH] = {0};
      realpath(file.c_str(),tmp);
      info._path_phys = tmp;
      //判断相对路径，防止出现就是将相对路径改为绝对路径的时候，这个绝对路径中没有根目录了，此时就进不去了
      //访问权限不够
      if(info._path_phys.find(WWWROOT)==std::string::npos){//为啥要加WWW
        info.SetError("403");
        return false;
      }
      return true;
    }
    bool ParseHttpHeader(RequestInfo& info){//解析http请求头；
      std::cerr<<"解析http请求头"<<std::endl;
      std::vector<std::string> hdr_list;
      Tools::Split(header,"\r\n",hdr_list);
      if(ParseHttpFirstLine(hdr_list[0],info)==false)
        return false;
      for(size_t i = 1;i<hdr_list.size();i++){
        size_t pos = hdr_list[i].find(':');
        info._hdr_list[hdr_list[i].substr(0,pos)]=hdr_list[i].substr(pos+2);
      }
      for(auto it:info._hdr_list){//看看头部内容；
        std::cout<<"["<<it.first<<":"<<it.second<<"]"<<std::endl;
      }
      std::cerr<<std::endl<<"Now Display info!"<<std::endl;
      info.display();
      return true;
    }
    //向外提供解析结果；
    RequestInfo& GetRequestInfo();
};

class HttpResponse{
  private://文件请求（完成文件下载/类表功能）接口//CGI请求接口
    int _cli_sock;
    std::string _etag;//文件是否被修改过，是否是源文件
    std::string _mtime;//最后一次修改时间；
    std::string _date;//当前系统响应时间；
    std::string _cont_len;
  public:
    HttpResponse(int sock):_cli_sock(sock)
  {}
    bool InitResponse(RequestInfo& req_info){//初始化一些请求的相应信息；

      Tools::TimeToGMT(req_info._st.st_mtime,_mtime);//Last-Modified
      Tools::MakeETag(req_info._st.st_size,req_info._st.st_ino,req_info._st.st_mtime,_etag);//ETag;
      time_t t= time(NULL);//Date
      Tools::TimeToGMT(t,_date);
      return true;
    }
    bool SendData(const std::string &buf){
      if(send(_cli_sock,buf.c_str(),buf.size(),0)<0)
        return false;
      return true;
    }
    bool SendCData(std::string &buf){//分块传输，每一块需要先发送该块大小
      if(buf.empty()){
        SendData("0\r\n\r\n");
      }
      std::stringstream ss;
      ss<<std::hex<<buf.size()<<"\r\n";
      SendData(ss.str());
      ss.clear();
      SendData(buf);
      SendData("\r\n");
      return true;
    }

    bool ProcessFile(RequestInfo& info){;//文件下载
      return true;
    }
    bool ProcessList(RequestInfo& info){;//文件列表功能
      std::cerr<<"Now in ProcessFile"<<std::endl;
      //组织头部：首行；
      //content-Type:text/html\r\n
      //ETage:\r\n
      //Date:\r\n
      //Transfer-Encoding：chunked\r\n//分块传输，只告诉一部分一部分的
      //Connrction:close\r\n\r\n
      //正文：
      //每一个目录下的文件都要组织一个html标签信息
      std::string rsp_header;
      rsp_header = info._version+" 200 OK\r\n";
      rsp_header+="Content-Type: text/html;charset=UTF-8\r\n";
      rsp_header+="Connection: close\r\n";
      if(info._version=="HTTP/1.1")
        rsp_header+="Transfer-Encoding: chunked\r\n";
      rsp_header+="ETage: "+_etag+"\r\n";
      rsp_header+="Last-Modified: "+_mtime+"\r\n";
      rsp_header+="Date: "+_date+"\r\n\r\n";
      SendData(rsp_header);
      std::string rsp_body;
      rsp_body = "<html><head>";//前置说明；
      rsp_body += "<title>Catalog:</title>";
      rsp_body +="<meta charset = 'utf-8'>";
      rsp_body +="</head><body>";
      rsp_body +="<h1>"+info._path_info+"</h1><hr /><ol>";
      //获取目录下每一个文件，组织处html信息，chunked传输;
      std::string file_html;
      struct dirent **p_dirent = NULL;
      file_html = "<li>";//有序列表元素
      int num = scandir(info._path_phys.c_str(),&p_dirent,0,alphasort);//过滤规则：不做过滤，默认字母排序
      for(int i = 0;i<num;i++){
        std::string path = info._path_phys;
        std::string file = path+p_dirent[i]->d_name;
        struct stat st;
        if(stat(file.c_str(),&st)<0)//获取文件信息；
          continue;
        std::string mtime;
        std::string mime;
        std::string fsize;
        Tools::GetMime(p_dirent[i]->d_name,mime);
        Tools::DigitToStr(st.st_size/1024,fsize);
        Tools::TimeToGMT(st.st_mtime,mtime);
        file_html+="<strong><a href='"+ info._path_info;
        file_html+=p_dirent[i]->d_name;
        file_html+="'>";
        file_html+=p_dirent[i]->d_name;
        file_html+= "</a></strong>";
        file_html+="<br /><small>";
        file_html+="modified: "+mtime+"<br />";
        file_html+=mime+" - "+fsize+"kbytes";
        file_html+="<br /><br /></small></li>";
        SendCData(file_html);
      }
      rsp_body = "</ol><hr /></body></html>";
      SendCData(rsp_body);
      SendData("");
      return true;
    }
    bool ProcessCGI(RequestInfo& info){//cgi请求处理;；
      return true;
    }
    bool ErrHandler(RequestInfo &info){
      std::string rsp_header;//首行，头部（content-Length Date）,空行，正文；
      rsp_header = info._version+" "+info._err_code+" "+Tools::GetErro_desc(info._err_code)+"\r\n";
      time_t Time =time(NULL);
      std::string gmt;
      Tools::TimeToGMT(Time,gmt);
      rsp_header+="Date: "+gmt+"\r\n";
      std::string rsp_body;
      rsp_body = "<html><body><h1>"+info._err_code+"<h1></body></html>";
      std::string Length;
      Tools::DigitToStr(rsp_body.size(),Length);
      rsp_header +="Content-Length: "+Length+"\r\n\r\n";
      send(_cli_sock,rsp_header.c_str(),rsp_header.size(),0);
      send(_cli_sock,rsp_body.c_str(),rsp_body.size(),0);
      return true;
    }
    bool CGIHandler(RequestInfo& info){
      InitResponse(info);
      ProcessCGI(info);
      return true;
    }
    bool IsDir(RequestInfo& info){
      if(info._st.st_mode & S_IFDIR){
        if(info._path_info.back()!='/'){
          info._path_info.push_back('/');
        }
        if(info._path_phys.back()!='/'){
          info._path_phys.push_back('/');
        }
        return true;
      }
      return false;
    }
    bool FileHandler(RequestInfo& info){
      InitResponse(info);//初始化文件响应信息
      if(IsDir(info)||1){//判断请求文件是否是目录
        std::cerr<<"Now Go ProcessList!"<<std::endl;
        if(ProcessList(info))//执行文件列表展示响应
          std::cout<<"List Print Finish!"<<std::endl;
      }else{
        ProcessFile(info);//执行文件下载相应
      }
      return true;
    }
};

class UpLoad{
  //CGI外部程序中的文件上传功能处理接口；
};
