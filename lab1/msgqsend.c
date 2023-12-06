#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <limits.h>
#include <unistd.h>

#define PERMS 0644
struct my_msgbuf {
   long mtype;
   int value; //Int value that will be sent instead of text
};

int main(void) {
   struct my_msgbuf buf;
   int msqid;
   int len;
   key_t key;
   int i;
   system("touch msgq.txt");

   if ((key = ftok("msgq.txt", 'B')) == -1) {//Gets key value
      perror("ftok");
      exit(1);
   }

   if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {//Starts msgq
      perror("msgget");
      exit(1);
   }
   printf("Press enter to start.");
   while(getchar() != '\n'); //Waits so you have time to start recieve process before it sends msg
   printf("message queue: ready to send messages.\n");
   buf.mtype = 1; /* we don't really care in this case */

  for(i = 0; i < 50; i++){
	buf.value = ((float)rand()/RAND_MAX)*INT_MAX;
	len = sizeof(buf.value);
	if(msgsnd(msqid, &buf, len, 0) == -1)//Tries to send msg
	   perror("msgsnd");
	else
	   printf("Sending: %d\n",buf.value);
	usleep(500000);
   }
   buf.value = -1;
   len = sizeof(buf.value);
   if (msgsnd(msqid, &buf, len, 0) == -1)
      perror("msgsnd");

   if (msgctl(msqid, IPC_RMID, NULL) == -1) {
      perror("msgctl");
      exit(1);
   }
   printf("message queue: done sending messages.\n");
   return 0;
}
