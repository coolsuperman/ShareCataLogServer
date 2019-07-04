#include"HttpRequest.hpp"

enum _boundry_type{
  BOUNDRY_NO = 0,
  BOUNDRY_FIRST,
  BOUNDRY_MIDDLE,
  BOUNDRY_LAST,
  BOUNDRY_BAK//假设是Boundry
};

class Upload{
  private:
    int64_t content_len;
    int _file_fd;
    std::string _file_name;
    std::string _f_boundry;
    std::string _m_boundry;
    std::string _l_boundry;
  private:
    int MatchBoundry(char* buf,int blen,int* boundryPos){
      //----boundry;
      //first_boundry:------boundry;
      //middle_boundry:\r\n------boundry\r\n
      //last_boundry:\r\n------boundry--\r\n
      //从起始位置匹配first_boundry
      if(!memcmp(buf,_f_boundry.c_str(),_f_boundry.length())){
        boundryPos = 0;
        return BOUNDRY_FIRST;
      }
      for(int i = 0;i<blen;++i){
        //字符串剩余长度大于boundry的长度，则全部匹配
       // if((size_t)(blen-i)>_m_boundry.size()){
          if(!memcmp(buf+i,_m_boundry.c_str(),_m_boundry.length())){
            *boundryPos = i;
            return BOUNDRY_MIDDLE;
          }
          else if(!memcmp(buf+i,_l_boundry.c_str(),_l_boundry.length())){
            *boundryPos = i;
            return BOUNDRY_LAST;
          }
       // }
       else{
          //否则，如果剩余长度小于boundry长度，防止出现半个boundry
          //所以进行部分匹配
          int cmp_len=(blen-i);
          if(!memcmp(buf+i,_l_boundry.c_str(),cmp_len)){
            *boundryPos = i;
            return BOUNDRY_BAK;
          }
          if(!memcmp(buf+i,_m_boundry.c_str(),cmp_len)){
            *boundryPos = i;
            return BOUNDRY_BAK;
          }
        }
      }
      return BOUNDRY_NO;
    }
    bool GetFileName(char *buf,int * content_pos){
      char* ptr=NULL;
      ptr = strstr(buf,"\r\n\r\n");
      if(ptr==NULL){//获取失败
        *content_pos=0;//前面数据不能删；
        return false;
      }
      *content_pos = ptr-buf+4;
      std::string header;
      header.assign(buf,ptr-buf);
      std::string file_sep = "filename=\"";
      size_t pos = header.find(file_sep);
      if(pos==std::string::npos)
        return false;
      _file_name  = header.substr(pos+file_sep.length());
      pos = _file_name.find("\"");
      if(pos==std::string::npos)
        return false;
      _file_name.erase(pos);
      fprintf(stderr,"upload file:[%s]\n",_file_name.c_str());
      return true;
    }
    bool CreateFile(){
      std::string path = "./WWW/"+_file_name;
      _file_fd=open(path.c_str(),O_CREAT|O_WRONLY);
      if(_file_fd<0){
        fprintf(stderr,"open error:%s\n",strerror(errno));
        return false;
      }
      return true;
    }
    bool CloseFile(){
      if(_file_fd!=-1){
        close(_file_fd);
        _file_fd = -1;
      }
      return true;
    }
    bool WriteFile(char*buf,int len){
      if(_file_fd!=-1){
        write(_file_fd,buf,len);
      }
      return true;
    }
  public:
    Upload():_file_fd(-1) {}
    bool InitUpload(){//初始化boundry信息；
      std::string len=getenv("Content-Length");
      std::cerr<<"~Content-Length:"<<len<<std::endl;
      if(len.size()==0){
        fprintf(stderr,"Have No Content-Length!!\n");
        return false;
      }
      content_len = Tools::StrToDigit(len);
      std::string type=getenv("Content-Type");
      if(type.size()==0){
        fprintf(stderr,"Have No Content-Type!!\n");
        return false;
      }
     // std::cerr<<"Content---Type:"<<type<<std::endl;
      std::string boundry_sep="boundary=";
      size_t pos = type.find(boundry_sep);
      if(pos==std::string::npos){
        fprintf(stderr,"content type have no boundry!!\n");
        return false;
      }
      std::string boundry=type.substr(pos+boundry_sep.size());
      _f_boundry="--"+boundry;
      _m_boundry="\r\n"+_f_boundry+"\r\n";
      _l_boundry="\r\n"+_f_boundry+"--";
      return true;
    }
    bool ProcessUpload(){//对正文进行处理，将文件数据进行存储（处理上传）
      int64_t tlen = 0,blen=0;//已读取多少；当前buffer数据长度；
      char buf[MAX_BUFF];
      while(tlen<content_len){
        int len = read(0,buf+blen,MAX_BUFF-blen);
        blen+=len;//当前buf中数据长度
        //std::cerr<<"len:"<<len<<"blen:"<<blen<<std::endl;
        int boundry_pos,content_pos;
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_FIRST){
            std::cerr<<"first boundry match success!"<<std::endl;
          //1.匹配到firstBoundry，获取头部，从中获取文件名
          //2.若获取文件名成功，则创建文件，打开文件
          //3.将头信息从buf移除，上下数据进行下一步匹配
          if(GetFileName(buf,&content_pos)){
            CreateFile();
            std::cerr<<"OutCreatFile content_pos:"<<content_pos << " blen:" << blen<<std::endl;
            blen-=content_pos;
            memmove(buf,buf+content_pos,blen);
           // std::cerr << "buf:[" << buf << "]\n";
          }else{
            blen-=_f_boundry.size();
            memmove(buf,buf+_f_boundry.size(),blen);
          }
        }
       // std::cerr << "will middle boundary match\n";
        while(1){
         // std::cerr << "into middle boundary match\n";
          if(MatchBoundry(buf,blen,&boundry_pos)!=BOUNDRY_MIDDLE){
            std::cerr<<"middle boundry match fail!"<<std::endl;
            break;
          }
          std::cerr<<"middle boundry match success!"<<std::endl;
          //匹配middle_boundry成功
          //1.将boundry之前数据写入文件，将数据从buf中移除,关闭文件
          //2.看boundry头中是否有文件名--雷同first_boundry;
          WriteFile(buf,boundry_pos);//
          CloseFile();
          blen-=boundry_pos;//
          memmove(buf,buf+content_pos,blen);
          if(GetFileName(buf,&content_pos)){
            CreateFile();
            blen-=content_pos;
            memmove(buf,buf+content_pos,blen);
          }else{
            if(content_pos==0)
              break;
            blen-=_m_boundry.size();
            memmove(buf,buf+_m_boundry.size(),blen);
          }
        }
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_LAST){
          std::cerr<<"last boundry match success!"<<std::endl;
          //匹配Boundry_LAST成功
          //1.将boundry之前的数据写入文件
          //2.关闭文件
          //3.上传文件处理完毕，退出
          WriteFile(buf,boundry_pos);//
          CloseFile();
          return true;
        }
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_BAK){
          //1.将类似boundry位置之前的数据写入文件
          //2.移除之前的数据
          //3.剩下的数据不动，重新继续接收数据，补全后继续匹配
          WriteFile(buf,boundry_pos);//
          blen-=boundry_pos;//
          memmove(buf,buf+content_pos,blen);
        }
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_NO){
          WriteFile(buf,blen);//
          blen=0;//
        }
        tlen +=len;
      }
      return true;
    }
};

int main(){
  Upload  UPLOAD;
  std::string rsp_body;
  if(UPLOAD.InitUpload()==false) 
    return 0;
  std::cerr << "child init success\n";
  if(UPLOAD.ProcessUpload()==false)
    rsp_body="<html><body><h1>FAILED!</h1></body></html><br/>";
  else 
    rsp_body="<html><body><h1>SUCCESS!</h1></body></html><br/>";
  std::cout<<rsp_body;
  fflush(stdout);
  return 0;
}

