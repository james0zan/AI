#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "ai-api.hpp"

int data1=0, data2=0, data3=0;

void *child1(void *v) {
  AI_INS_THIS_FUNC
  data1 = 1;
  data2 = 2;
  data3 = 3;
  return NULL;
}

void *child2(void *v) {
  data1 = 1;
  do {
    AI_INS_THIS_BB
    data2 = 2;
  } while (0);
  data3 = 3;
  return NULL;
}

int main() {
  AI_INS_THIS_ADDR(&data1);
  pthread_t tid1 ,tid2;
  pthread_create(&tid1, NULL, child1, NULL);
  pthread_create(&tid2, NULL, child2, NULL);
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
}