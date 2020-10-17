
#include <iostream> 
#include<string>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include<sys/types.h>
#include<fcntl.h>
#include<semaphore.h>
#include <stdio.h> 
using namespace std; 
sigset_t myset;
int *shmM,*shmP,*s,shmidP,shmidM,blockNumber,*startingAddress,i,bno,enteriesP,enteriesM,count=0;
char type;
pid_t pid;
key_t keyP,keyM;
sem_t *sem; //semaphore

void requestBuffer(sem_t *sem)
{
	enteriesP=*shmP;
	shmP[enteriesP*3+1]=1; //1 indicating requesting buffer for block 
	shmP[enteriesP*3+2]=pid;
	shmP[enteriesP*3+3]=blockNumber;
		*shmP=*shmP+1;
	
}
void releaseBuffer()
{
	//sem_wait(sem);
	enteriesP=*shmP;
	shmP[enteriesP*3+1]=2; //2 indicating releasing buffer for block, process has done with block
	shmP[enteriesP*3+2]=blockNumber;
	switch(type)
	{
		case 'R':shmP[enteriesP*3+3]=0;
				 break;
		case 'W':shmP[enteriesP*3+3]=1;
				 break;
	
	}
	*shmP=*shmP+1;
	//sem_post(sem);


}
bool isAllocated()
{
	/*
	It returns only when it gets to know whether the buffer for requested block is free or busy.
		returns true if buffer is allocated for requested block 
		returns false if buffer is busy or block doesn't exist in hash queue and free list is empty.
	This function continiously checks in shared memory (shmM) for a response from main_handler
	*/
	enteriesM=*shmM;
	while(*shmM)
		{
			
			count=0;
			s=shmM;
			++s;
			while(count<*shmM)
			{
				if(*s==pid && *(s+1)==1)
					return 1;
				if(*s==pid && *(s+1)==2)
					return 0;
				s+=2;
				++count;
			}
			
		}
	return 0;
}


void mySIGINT(int sig)
{
	int *s=shmM;
	cout<<"\nWoke Up";
	requestBuffer(sem);
	sleep(3);
	if(isAllocated())
		{	
			cout<<"\nBuffer Allocated\n";
			if(type=='W')
			{
			cout<<"\nWritting to block.......\n";
			sleep(10);
			}
			cout<<"Done with block\n";
			releaseBuffer();
			cout<<"Buffer released\n";
			exit(0);
		}
		cout<<"\nBuffer is busy\n";
		cout<<"Sleeping......\n";
		sigsuspend(&myset);

}
int main() 
{ 
	//sem=sem_open("/SEM_HERE",O_CREAT|O_EXCL,0644,5);

	keyP = ftok("/home/mohit/Documents/mohit",65); 
  	keyM= ftok("/home/mohit/Documents/mohit",66);
	
    // shmget returns an identifier in shmid 
   	 shmidP = shmget(keyP,1024,0666|IPC_CREAT); 
  	shmidM = shmget(keyM,1024,0666|IPC_CREAT); 
  	
  	signal(SIGINT,mySIGINT);
    // shmat to attach to shared memory 
   	 shmP = (int*)shmat(shmidP,(void*)0,0); 
	pid =getpid();
	cout<<"\nProcess Id: "<<pid<<endl;
  	cout<<"\nEnter Block number: ";
	cin>>blockNumber;
	cout<<"\nEnter type of request (R for read request, W for write request: ";
	cin>>type;
	
	cout<<"Requesting to kernel for block number: "<<blockNumber<<endl;
	requestBuffer(sem);
 	 shmM= (int*)shmat(shmidM,(void*)0,0); 

	startingAddress=shmM;
	sleep(3);
	if(isAllocated())
		{	cout<<"\nBuffer Allocated\n";
			if(type=='W')
			{
				cout<<"Writting to block.......\n";
				sleep(10);
			}
			releaseBuffer();
			cout<<"\nBuffer released";
			exit(0);
		}
	cout<<"\nSleeping.......\n";
	(void) sigemptyset(&myset);
 	sigsuspend(&myset);		
   	//detach from shared memory  
   	shmdt(shmP);
   	shmdt(shmM); 
 	return 0; 
} 

