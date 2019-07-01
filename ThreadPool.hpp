#include"Tools.hpp"
#include<pthread.h>

typedef bool(*Handler)(int sock);

class HttpTask{
  private:
    int sock;
    Handler TaskHandler;
  public:
    HttpTask(int sock_=0,Handler handler=NULL):sock(sock_),TaskHandler(handler)
  { }
    void SetTask(int sock_,Handler handler){
      sock = sock_;
      TaskHandler = handler;
    }
    bool Run(){
      return TaskHandler(sock);
    }
};

class ThreadPool{
  private:
    size_t Max_pthread;
    size_t Now_pthread;
    std::queue<HttpTask> task_queue;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool quit=false;
  private: 
    void QueueLock(){
      pthread_mutex_lock(&lock);
    }
    void QueueUnlock(){
      pthread_mutex_unlock(&lock);
    }
    void SleepThread(){
      if(quit==true){
        Now_pthread--;
        QueueUnlock();
        pthread_exit(NULL);
      }
      pthread_cond_wait(&cond,&lock);
    }
    void WakeUpThread(int i = 1){
      if(i==1){
        pthread_cond_signal(&cond);
      }else{
        pthread_cond_broadcast(&cond);
      }
    }
    bool IsEmpty(){
      return !(task_queue.size());
    }
    bool IsFull(){
      return Now_pthread==Max_pthread;
    }
    void TaskGet(HttpTask& t){
      t = task_queue.front();
      task_queue.pop();
    }
  public:
    ThreadPool(int max):Max_pthread(max),Now_pthread(0){
      pthread_cond_init(&cond,NULL);
      pthread_mutex_init(&lock,NULL);
    }
    ~ThreadPool(){
      pthread_cond_destroy(&cond);
      pthread_mutex_destroy(&lock);
    }
    void PoolInit(){
      pthread_t tpid[Max_pthread];
      for(size_t i =0;i<Max_pthread;i++){
        pthread_create(tpid+i,NULL,Work,(void*)this);
        Now_pthread++;
      }
    }
    static void* Work(void* arg){
      pthread_detach(pthread_self());
      ThreadPool* that = (ThreadPool*)arg;
      while(1){
       that->QueueLock();
       while(that->IsEmpty()){
         that->SleepThread();
       }
       HttpTask t;
       that->TaskGet(t);
       that->QueueUnlock();
       t.Run();
      }
      return NULL;
    }
    void AddTask(const HttpTask &t){
      QueueLock();
      task_queue.push(t);
      WakeUpThread();
      QueueUnlock();
    }
    bool PoolStop(){
      quit = true;
      while(!Now_pthread){
        WakeUpThread(0);
      }
      return true;
    }
};
