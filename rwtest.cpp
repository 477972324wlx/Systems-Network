#include <pthread.h>
#include <list>
#include <iostream>
#include <string>

struct rwlock{
  int nActive;
  int nPendingReads;
  int nPendingWrites;

  pthread_cond_t canRead;
  pthread_cond_t canWrite;
  pthread_mutex_t mutex;  
};

void lockShared(rwlock * rw){
    pthread_mutex_lock(&rw->mutex);
    rw->nPendingReads++;
    while(rw->nPendingWrites)
        pthread_cond_wait(&rw->canRead, &rw->mutex);
    if(rw->nActive < 0)
        pthread_cond_wait(&rw->canRead, &rw->mutex);
    rw->nPendingReads--;
    rw->nActive++;
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
    while(rw->nActive > 0)
        pthread_cond_wait(&rw->canWrite, &rw->mutex);
    rw->nActive = -1;
    rw->nPendingWrites--;
    pthread_mutex_unlock(&rw->mutex);
}

void unlockExclusive(rwlock * rw){
    int awakeReaders;
    pthread_mutex_lock(&rw->mutex);
    awakeReaders = rw->nPendingReads;
    rw->nActive++;
    pthread_mutex_unlock(&rw->mutex);
    if(awakeReaders > 0)
        pthread_cond_broadcast(&rw->canRead);
    else    
        pthread_cond_signal(&rw->canWrite);
    
}