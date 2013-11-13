#include "ai-common/ai_defines.h"
#include "ai-common/ai_structs.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


void ERROR(const char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

void print_help() {
  ERROR("[Usage] id-server [Output: .AccessesInfo File]");
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    print_help();
  }

  socklen_t sin_len;

  int socket_descriptor;
  struct sockaddr_in sin;

  bzero(&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(AI_NUM_SERVER_PORT);
  sin_len = sizeof(sin);

  socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
  assert(bind(socket_descriptor, (struct sockaddr *)&sin, sizeof(sin)) == 0);

  FILE * LogFile = fopen(argv[1], "w");
  AI_ACCESS_ID CNT = 1;
  char message[AI_NUM_SERVER_MESSAGE_MAX_SIZE];
  while(1) {
    recvfrom(socket_descriptor, message, sizeof(message), 0, (struct sockaddr *)&sin, &sin_len);
    fprintf(LogFile, "%lu\t|\t%s", CNT, message);
    fflush(LogFile);
    sendto(socket_descriptor, &CNT, sizeof(CNT), 0, (struct sockaddr *)&sin, sin_len);
    CNT++;
  }

  return 0;
}