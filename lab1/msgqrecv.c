#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PERMS 0644
struct my_msgbuf {
   long mtype;
   int value;
};

int main(void) {
   struct my_msgbuf buf;
   int msqid;
   key_t key;

   if ((key = ftok("msgq.txt", 'B')) == -1) {
      perror("ftok");
      exit(1);
   }

   if ((msqid = msgget(key, PERMS)) == -1) { /* connect to the queue */
      perror("msgget");
      exit(1);
   }
   printf("message queue: ready to receive messages.\n");

   while(buf.value != -1){
	if(msgrcv(msqid, &buf, sizeof(buf.value), 0, 0) == -1){ //Gets the msg
	   perror("msgrcv");
	   exit(1);
	}
	if(buf.value != -1)
	   printf("recvd: \"%d\"\n",buf.value);
   }
   printf("message queue: done receiving messages.\n");
   system("rm msgq.txt");
   return 0;
}
