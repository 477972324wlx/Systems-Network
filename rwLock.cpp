#include <pthread.h>
#include <list>
#include <iostream>
#include <string>
using namespace std;

struct rwlock{
    int nActive;
    int nPendingReads;
    int nPendingWrites;
    pthread_mutex_t mutex;
    pthread_cond_t canRead , canWrite;

    rwlock(){
        nActive = nPendingReads = nPendingWrites = 0;
        pthread_mutex_init(&mutex,0);
        pthread_cond_init(&canRead,0);
        pthread_cond_init(&canWrite,0);
    }
};

void lockShared(rwlock * rw){
    pthread_mutex_lock(&rw->mutex);
    rw->nPendingReads++;
    if(rw->nPendingWrites){
        pthread_cond_wait(&rw->canRead, &rw->mutex);
    }
    while(rw->nActive < 0) 
        pthread_cond_wait(&rw->canRead, &rw->mutex);
    
    rw->nActive++;
    rw->nPendingReads--;
    pthread_mutex_unlock(&rw->mutex);
}

void unlockShared(rwlock * rw){
    pthread_mutex_lock(&rw->mutex);  
    rw->nActive--;
    if(rw->nActive == 0){
        pthread_mutex_unlock(&rw->mutex);
        pthread_cond_signal(&rw->canWrite);
    } else {
        pthread_mutex_unlock(&rw->mutex);
    } 
}

void lockExclusive(rwlock * rw){
    pthread_mutex_lock(&rw->mutex);
    rw->nPendingWrites++;
    while(rw->nActive){
        pthread_cond_wait(&rw->canWrite, &rw->mutex);
    }
    rw->nPendingWrites--;
    rw->nActive = -1;
    pthread_mutex_unlock(&rw->mutex);
}

void unlockExclusive(rwlock * rw){
 
    pthread_mutex_lock(&rw->mutex);
    rw->nActive = 0;
    int wakeReaders = rw->nPendingReads;
    pthread_mutex_unlock(&rw->mutex);

    if(wakeReaders) 
        pthread_cond_broadcast(&rw->canRead);
    else    
        pthread_cond_signal(&rw->canWrite);
}

void downgrade(rwlock * rw){
    pthread_mutex_lock(&rw->mutex);
    int wakeReaders = rw->nPendingReads;
    rw->nActive = 1;
    pthread_mutex_unlock(&rw->mutex);
    if(wakeReaders)
        pthread_cond_broadcast(&rw->canRead);
}
void upgrade(rwlock * rw){
    pthread_mutex_lock(&rw->mutex);
    if(rw->nActive == 1){
        rw->nActive = -1;
    } else {
        rw->nPendingWrites++;
        rw->nActive--;
        while(rw->nActive)
            pthread_cond_wait(&rw->canWrite, &rw->mutex);
        rw->nPendingWrites--;
        rw->nActive = -1;
    }
    pthread_mutex_unlock(&rw->mutex);
}

rwlock rw;
string str;
static int a = 0;
void * readThread(void*){
    lockShared(&rw);
    string s1 = str;
    cout << s1 << endl;
    unlockShared(&rw);
}

void * writeThread(void*){
    lockExclusive(&rw);
    char ch = str[a++];
    str = str + ch;
    unlockExclusive(&rw);
}
typedef void *(*myfunc)(void*);

int main(){
    str = "ayiyayiya";
    pthread_t tid;
    int err;
    for (int i = 0 ; i <= 300 ; ++i){
        myfunc me = readThread;
        if(i % 3 == 0){
            me = writeThread;
        }
        err = pthread_create(&tid, 0, me, NULL);
    }

}