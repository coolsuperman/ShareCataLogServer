#include"Tools.hpp"
#include<time.h>
#include<sstream>
#include<dirent.h>
#include<fcntl.h>

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
    static void TimeToGMT(time_t t,std::string& gmt){ 
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
    std::string _method;//请求方法
    std::string _version;//协议版本
    std::string _path_info;//资源路径
    std::string _path_phys;//资源实际路径
    std::string _query_string;//查询字符串
    std::unordered_map<std::string,std::string>_hdr_list;//存储头信息中的所有键值对；
    struct stat _st;//获取其他文件信息；大小，inode。之类；
  public:
    std::string _err_code;//错误码；
    void SetError(const std::string &code){
      _err_code = code;
    }
  public:
    bool RequestIsCGI(){
      if((_method=="GET"&&(_query_string.size()!=0))||(_method=="POST")){
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
       std::cout<<file<<std::endl;
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
     /* for(auto it:info._hdr_list){//看看头部内容；
      //  std::cout<<"["<<it.first<<":"<<it.second<<"]"<<std::endl;
      //}
      //std::cerr<<std::endl<<"Now Display info!"<<std::endl;
       info.display();*/
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
    std::string _fsize;//文件大小；
    std::string _mime;//获取文件类型
    std::string _cont_len;
  public:
    HttpResponse(int sock):_cli_sock(sock)
  {}
    bool InitResponse(RequestInfo& req_info){//初始化一些请求的相应信息；

      Tools::TimeToGMT(req_info._st.st_mtime,_mtime);//Last-Modified
      Tools::MakeETag(req_info._st.st_size,req_info._st.st_ino,req_info._st.st_mtime,_etag);//ETag;
      time_t t= time(NULL);//Date
      Tools::TimeToGMT(t,_date);
      Tools::DigitToStr(req_info._st.st_size,_fsize);
      Tools::GetMime(req_info._path_phys,_mime);
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
      std::cerr<<"NOW in ProcessFile!"<<std::endl;
      std::string rsp_header;
      rsp_header = info._version+" 200 OK\r\n";
      //rsp_header+="Content-Type: text/html;charset=UTF-8\r\n";//这样返回的数据浏览器会按照文本模式打开；
      rsp_header+="Content-Type: "+_mime+"\r\n";//这样返回的数据浏览器会按照文本模式打开；
      rsp_header+="Connection: close\r\n";
      rsp_header+="Content-Length: "+_fsize+"\r\n";//知道大小不用chuncked；
      rsp_header+="ETage: "+_etag+"\r\n";
      rsp_header+="Last-Modified: "+_mtime+"\r\n";
      rsp_header+="Date: "+_date+"\r\n\r\n";
      SendData(rsp_header);
      int fd=open(info._path_phys.c_str(),O_RDONLY);
      if(fd<0){
        info._err_code="400";
        ErrHandler(info);
        return false;
      }
        int rlen = 0;
        char tmp[MAX_BUFF];
        while((rlen=read(fd,tmp,MAX_BUFF))>0){
          //tmp[rlen]='\0';
          //SendData(tmp);
          send(_cli_sock,tmp,rlen,0);
        }
      close(fd);
      return true;
    }
    static int filt(const struct dirent* dir){//scandir的过滤策略；
      if(strcmp(dir->d_name,".")==0)
        return 0;//标识不添加到列表；
      return 1;//添加；
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
      rsp_body += "<title>Home/Catalog</title>";
      rsp_body +="<meta charset = 'utf-8'>";
      rsp_body +="</head><body>";
      rsp_body +="<h1>[Path]:"+info._path_info+"</h1>";
      rsp_body +="<form action='/upload' method='POST' enctype='multipart/form-data'>";
      rsp_body +="<input type='file' name='FileUpLoad' />";
      rsp_body +="<input type='submit' value='上传' / >";
      rsp_body +="</form>";
      rsp_body +="<hr /><ol>";
      //获取目录下每一个文件，组织处html信息，chunked传输;
      SendCData(rsp_body);

      std::string file_html;
      struct dirent **p_dirent = NULL;
      int num = scandir(info._path_phys.c_str(),&p_dirent,filt,alphasort);//过滤规则：不做过滤，默认字母排序
      for(int i = 0;i<num;i++){
        file_html+= "<li>";//有序列表元素
        std::string path = info._path_phys;
        std::string file = path+"/"+p_dirent[i]->d_name;
        struct stat st;
        if(stat(file.c_str(),&st)<0)//获取文件信息；
          continue;
        std::string mtime;
        std::string mime;
        std::string fsize;
       //std::cout<<"st_mtime:"<<st.st_mtime<<"-"<<"st_mime:"<<p_dirent[i]->d_name<<"-"<<"st_fsize:"<<st.st_size<<std::endl;
        Tools::TimeToGMT(st.st_mtime,mtime);
        Tools::GetMime(p_dirent[i]->d_name,mime);
        Tools::DigitToStr(st.st_size,fsize);
       //std::cout<<"mtime:"<<mtime<<"-"<<"mime:"<<mime<<"-"<<"fsize:"<<fsize<<std::endl;
        file_html+="<strong><a href='"+ info._path_info;
        file_html+=p_dirent[i]->d_name;
        file_html+="'>";
        file_html+=p_dirent[i]->d_name;
        file_html+= "</a></strong>";
        file_html+="<br/ ><small>";
        file_html+="Modf: "+mtime+"<br />";
        file_html+=mime+"  Size:"+fsize+"B";
        file_html+="<br /><br /></small></li>";
      }
      SendCData(file_html);
      rsp_body = "</ol><hr /></body></html>";
      SendCData(rsp_body);
      SendData("");
      return true;
    }
    bool ProcessCGI(RequestInfo& info){//cgi请求处理;；
      //使用外部程序完成CGI请求处理---文件上传
      //将http头信息和正文全都交给子进程处理
      //使用环境变量传递头信息
      //使用管道传递正文数据
      //使用管道接收CGI程序处理结果
      //流程：创建管道，创建子进程，设置子进程环境变量；
      int data[2];//用于传输数据；
      int back[2];//返回处理结果;
      if(pipe(data)||pipe(back)){
        info._err_code="500";
        ErrHandler(info);
        return false;
      }
      int pid =fork();
      if(pid<0){
        info._err_code = "500";
        ErrHandler(info);
        return false;
      }else if(pid==0){
        setenv("METHOD",info._method.c_str(),1);
        setenv("VERSION",info._version.c_str(),1);
        setenv("PATH_INFO",info._path_info.c_str(),1);
        setenv("QUERY_STRING",info._query_string.c_str(),1);
        for(auto it=info._hdr_list.begin();it !=info._hdr_list.end();it++){
          setenv(it->first.c_str(),it->second.c_str(),1);
        }
        close(data[1]);
        close(back[0]);
        dup2(data[0],0);//子进程直接从标准输入读取正文数据
        dup2(back[1],1);//子进程直接打印处理结果传递给父进程
        execl(info._path_phys.c_str(),info._path_phys.c_str(),NULL);
        exit(0);
      }
      //下面是父进程，传数据，收结果,相应客户端；
      close(data[0]);
      close(back[1]);
      //1.通过data管道将正文数据传递给子进程；
      auto it = info._hdr_list.find("Content-Length");//没找着不用提交数据给子进程
      if(it!=info._hdr_list.end()){
        char buf[MAX_BUFF]={0};
        long content_len = Tools::StrToDigit(it->second);
        int rlen=recv(_cli_sock,buf,MAX_BUFF,0);
        if(rlen<0){
          return false;
        }
        if(write(data[1],buf,rlen)<0)
          return false;
      }
      //2.通过out管道读取子进程的处理结果直到返回0；
      //3.将处理结果组织http数据，响应给客户端；
      std::string rsp_header;
      rsp_header = info._version+" 200 OK\r\n";
      rsp_header+="Content-Type: text/html\r\n";//这样返回的数据浏览器会按照文本模式打开；
      rsp_header+="Connection: close\r\n";
      rsp_header+="ETage: "+_etag+"\r\n";
      rsp_header+="Last-Modified: "+_mtime+"\r\n";
      rsp_header+="Date: "+_date+"\r\n\r\n";
      SendData(rsp_header);
      while(1){
        char buf[MAX_BUFF]={0};
        int rlen=read(back[0],buf,MAX_BUFF);
        if(rlen==0){
          break;
        }
        send(_cli_sock,buf,rlen,0);
      }

      close(data[1]);
      close(back[0]);
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
      if(IsDir(info)){//判断请求文件是否是目录
        std::cerr<<"Now Go ProcessList!"<<std::endl;
        if(ProcessList(info))//执行文件列表展示响应
          std::cout<<"List Print Finish!"<<std::endl;
      }else{
        std::cerr<<"Now Go ProcessFile!"<<std::endl;
        if(ProcessFile(info)){;//执行文件下载相应
        std::cerr<<"Finish Send Filrdata"<<std::endl;
        }
      }
      return true;
    }
};

class UpLoad{
  //CGI外部程序中的文件上传功能处理接口；
};
