//Implementation of getblock Algorithm
#include <iostream> 
#include<bits/stdc++.h>
#include<vector>
#include<queue>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include <sys/ipc.h> 
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/shm.h> 
#include<time.h>
#include<stdlib.h>
#include<fcntl.h>
using namespace std;
#define MAX_BLOCK_NUMBER 100
pid_t pidChild; //pid for process that is processing asynchronous write
int processId,requestedBlockNo,*shmP,*shmM,shmidP,shmidM,enteriesM,enteriesP=0;  //shmP,shmM-shared variables
int *s,i,randomIndex,freedBlockNo,Count=1,current=1,Count1;
bool delayedWrite;  //if the type of request is write then it is set to indicate delayed write buufer
vector<vector <int> >  busyBuffer(MAX_BLOCK_NUMBER);  //for storing sleeping processes for blocked buffer
vector<int> emptyFreeList;		//for storing sleeping processes for any freed buffer
void wakeUp();
class node
{
	node *left; 	//points to left buffer in hash queue
	node *right;	//points to right buffer in hash queue
	node *fLeft;	//points to left free buffer
	node *fRight;	//points to right free buffer
	int blockNumber;
	string status;
	bool delayedWrite;
	friend class bufferCache;
	public:
	
	node(int bno,node *l=NULL,node *r=NULL)
	{
		blockNumber=bno;
		status="Free";
		delayedWrite=0;
		left=l;
		right=r;
		fLeft=NULL;
		fRight=NULL;
	}
	
};
vector <pair<int,node *> > aWrite; //to store which child is handling asnchronous write of which block
node *delayedBuffer;
class bufferCache
{

	node* hHead[4];   //heads of four hash queues
	node* hTail[4];	  //tails of four hash queues
	node* fHead;	  //head of free list
	node* fTail;	  //tail of free list
	public:
	bufferCache()
	{
	    int i;
		node *temp;
		hHead[0]=hHead[1]=hHead[2]=hHead[3]=hTail[0]=hTail[1]=hTail[2]=hTail[3]=fHead=fTail=NULL;
		for(i=1;i<=2;i++)
		{
			temp=addToHashQueue(i);
			addToFreeList(temp);
		}
	}
	string getBlock(int);
	node* updateStatusToFree(int);
	node* addToHashQueue(int);
	void deleteFromHashQueue(int);
	void removeFromFreeList(node*);
	node* removeFromFrontFreeList();
	void addToFreeList(node*);
	void addToFrontFreeList(node*);
	void markDelayedWrite(node *);
	void display();
	static void myHandler(int, siginfo_t *,void *);

}b;
node* search(pid_t pid)
{
	int i;
	node *n=NULL;
	for(i=0;i<aWrite.size();i++)
	{
		if(aWrite[i].first==pid)
		{
			n=aWrite[i].second;
			aWrite.erase(aWrite.begin()+i);
			return n;
		}
	}
}
void bufferCache::myHandler(int sig, siginfo_t *siginfo, void *context)
	{
			cout<<"\nAsnchronous Writter process--------------------------\n";
			node *temp;
			pidChild=siginfo->si_pid; //process id of process that has completed asynchronous write for a block, that will be used to fetch corresponding block on which the child is processing asynchronous write
			
			temp=search(pidChild);
			cout<<"Assynchronous write for block number: "<<temp->blockNumber<<" is completed\n";
			temp->delayedWrite=0;
			temp->status="FREE";
			//delayedBuffer->right=NULL;
			//delayedBuffer->left=NULL;
			temp->fRight=NULL;
			temp->fLeft=NULL;
			b.addToFrontFreeList(temp);
			cout<<"Suceessfully added buffer to the front of free list\n";
			freedBlockNo=delayedBuffer->blockNumber;  //making freedBlockNo is equal to block number of delayed marked buffer whose asynchronous write has completed so that wake up function correctly wakes up sleeping processess waiting for that particular block number 
			//aWrite.erase(pidChild);
			wakeUp();
			b.display();
			
	}



node* bufferCache::updateStatusToFree(int bno)
{
	
	int hQNumber=bno%4;				//mark the buffer free and return the buffer
	node *temp=hHead[hQNumber];
	while(temp!=NULL && temp->blockNumber!=bno)
		temp=temp->right;
	if(temp!=NULL)
	{
			temp->status="Free";
			temp->fLeft=NULL;		//because this buffer will be appended to the end of free list so making null free list left right pointer
			temp->fRight=NULL;	
			return temp;
	}
	return NULL;
}
void bufferCache::markDelayedWrite(node* temp)
{
	temp->delayedWrite=1;
}
node* bufferCache::addToHashQueue(int bno)
{
	int hQNumber=bno%4;
	node *temp;
	if(hHead[hQNumber]==NULL)			//always add to tail
		{
		
		temp=new node(bno);
		hHead[hQNumber]=temp;
		hHead[hQNumber]->left=NULL;
		hHead[hQNumber]->right=NULL;
		hTail[hQNumber]=hHead[hQNumber];
		}

	else
	{
		temp=new node(bno,hTail[hQNumber]);
		hTail[hQNumber]->right=temp;
		hTail[hQNumber]=hTail[hQNumber]->right;
	}
	return temp;
}
void bufferCache::deleteFromHashQueue(int bno)
{
	int hQNumber=bno%4;				//removes a buffer from hash queue, it can be at any place in the list
	node *temp=hHead[hQNumber];
	while(temp!=NULL && temp->blockNumber!=bno)
		temp=temp->right;
	if(temp)
	{
		if(hHead[hQNumber]==hTail[hQNumber])
			hHead[hQNumber]=hTail[hQNumber]=NULL;
			
		else if(temp==hHead[hQNumber])
		{
			hHead[hQNumber]=hHead[hQNumber]->right;
			hHead[hQNumber]->left=NULL;
		}
		else 
		{
			temp->left->right=temp->right;
			if(temp==hTail[hQNumber])
				hTail[hQNumber]=hTail[hQNumber]->left;
			else
				temp->right->left=temp->left;
		}
		delete temp;
	}
}
void bufferCache::removeFromFreeList(node *temp)
{
	
	if(temp==fHead)
	{
					//delete a particular node
		if(fHead==fTail)
		{
			fHead=fTail=NULL;
		}
		else
		{
			fHead=fHead->fRight;
			fHead->fLeft=NULL;
		}
	}
	else if(temp==fTail)
	{
		fTail=fTail->fLeft;
		fTail->fRight=NULL;
	}
	
	else
	{
		temp->fRight->fLeft=temp->fLeft;
		temp->fLeft->fRight=temp->fRight;
	}

}
node* bufferCache::removeFromFrontFreeList()
{
	if(fHead!=NULL)
	{
		node *temp=fHead;			//always delete from head
		if(fHead==fTail)
		{
			fHead=fTail=NULL;
		}
		else
		{
			fHead=fHead->fRight;
			fHead->fLeft=NULL;
		}
		return temp;
	}
	return NULL;
}
 void bufferCache::addToFrontFreeList(node *temp)
 {

 	if(fHead==NULL)
 	{
 		fHead=fTail=temp;
 		fHead->fLeft=NULL;
 		fHead->fRight=NULL;
 		}
 	else
 	{
 		fHead->fLeft=temp;
 		temp->fRight=fHead;
 		fHead=temp;

 	}
 }


void bufferCache::addToFreeList(node* temp)		//always add to tail never create new node, node is addedd that has been already passed as argument
{
	
	if(fTail==NULL)
	{
		fHead=fTail=temp;
		fHead->fLeft=NULL;
		fHead->fRight=NULL;
	}
	else
	{
		fTail->fRight=temp;
		temp->fLeft=fTail;
		fTail=fTail->fRight;

	}
	
}

void bufferCache::display()
{
	cout<<"\n\n************************";
	cout<<"\n    BUFFER CACHE:";
	cout<<"\n************************";
	int i;
	node *temp;
	for(i=0;i<=2;i++)
	{
		temp=hHead[i];
		cout<<"\nHash Queue: "<<i<<" : ";
		while(temp!=NULL)
		{
			cout<<temp->blockNumber<<"  ";
			temp=temp->right;
		}
	}
	
	cout<<"\nFree List: ";
	temp=fHead;
	while(temp!=NULL)
	{
		cout<<temp->blockNumber<<"  ";
		temp=temp->fRight;
	}
	cout<<endl;
	
}


string bufferCache:: getBlock(int bno)
{
	/* This function returns status whether buffer has been sucessfully allocated for the requested block number
		It returns one of the following:-
		- BUFFER ALLOCATED
		- BUFFER BUSY
		- NO FREE BUFFER
	*/
	struct sigaction act;
	memset(&act,'\0',sizeof(act));
	act.sa_sigaction=&myHandler;
	act.sa_flags=SA_SIGINFO;
	sigaction(SIGINT,&act,NULL);
	
	int hQNumber=bno%4;
	node *temp=hHead[hQNumber];
	while(temp!=NULL)
	{
		if(temp->blockNumber==bno)
		{
			cout<<"\nBlock number :"<<bno<<" is available in hash queue:";
			if(temp->status=="Busy")
			{
				return "Buffer Busy";

			}					//Found in Hash Queue
		
			temp->status="Busy";
			removeFromFreeList(temp);
			return "Buffer Allocated";
	
		}
		temp=temp->right;
	}
	cout<<"Kernel can not find the block on the hash queue\n";  
	
	DELAYED_WRITE:
	if(fHead)     //buffer is not found on hash queue and free list is not empty
	{
		temp=removeFromFrontFreeList();
		cout<<"\nChecking for free buffer in free list...\n";

		while(temp!=NULL && temp->delayedWrite==1)
		{
			cout<<"\nBuffer marked delayed Write\nInitiating assynchronous writting of buffer data on the disk\n";
			cout<<"Assynchronously writting data of block number: "<<temp->blockNumber<<" from buffer to disk...\n";
			temp->status="LOCKED";
			delayedBuffer=temp;
			pidChild=fork();
			aWrite.push_back(make_pair(pidChild,delayedBuffer));
			if(pidChild==0)
			{
				sleep(5);
				kill(getppid(),SIGINT);
				exit(0);
			}
			temp=removeFromFrontFreeList();
			cout<<"Checking for next free buffer in free list...\n";

		}
		if(temp==NULL)
			goto free;
		deleteFromHashQueue(temp->blockNumber);
		temp=addToHashQueue(bno);			//Allocating From Free List 
		temp->status="Busy";
		cout<<"\nReading contents of block from secondary memory and writing onto buffer......";
		sleep(4);
		return "Buffer Allocated";
						
	}
	free:        //if buffer is not found on hashqueue and free list is empty
	cout<<"\nFree List is empty";

	return "No Free Buffer";
	
	

	
}
 
void wakeUp()
{

		if(busyBuffer[freedBlockNo].size() || emptyFreeList.size())
			{
			cout<<"\nKernel waking up sleeping processes ...";	
			}
		if(emptyFreeList.size())
		{
	
			cout<<"\nWaking up processess waiting for any free buffer..";
	
			while(emptyFreeList.size())
			{
				srand(time(0));
				randomIndex=rand()%emptyFreeList.size();		//randomly selecting a sleeping process which is waiting for a free buffer
				processId=emptyFreeList[randomIndex];
				emptyFreeList.erase(emptyFreeList.begin()+randomIndex);    //removing that request 
				kill(processId,SIGINT); //waking up sleeping process by sending signal
			}
		}
		if(busyBuffer[freedBlockNo].size())
		{
			cout<<"\nWaking up processess waiting for block number:"<<freedBlockNo;
	
			while(busyBuffer[freedBlockNo].size())
			{

				srand(time(0));
				randomIndex=rand()%busyBuffer[freedBlockNo].size();
				processId=busyBuffer[freedBlockNo][randomIndex];
				busyBuffer[freedBlockNo].erase(busyBuffer[freedBlockNo].begin()+randomIndex);
				cout<<"\nWaking up process "<<processId<<endl;
				kill(processId,SIGINT); //sending signal 
			}		
		
		}

}
int main() 
{ 
	node *released;
	b.display();
	
	cout<<"\n";
	string status;
	cout<<"\n-----------------------------------------------------------------------------";
	cout<<"\n                 Simulation of getblk algorithm";
	cout<<"\n------------------------------------------------------------------------------";
	key_t keyP = ftok("/home/mohit/Documents/mohit",65);  //file to key give location of any file name
  	key_t keyM= ftok("/home/mohit/Documents/mohit",66);
	shmidP = shmget(keyP,1024,0666|IPC_CREAT);  	 
    	// shmat to attach to shared memory 
    shmP = (int *)shmat(shmidP,(void*)0,0); 
	
	shmidM= shmget(keyM,1024,0666|IPC_CREAT);  	 
    	// shmat to attach to shared memory 
    shmM = (int *)shmat(shmidM,(void*)0,0); 
	s=shmM;
	for(i=0;i<20;i++)
	{
	*s=0;
	}
	*shmP=0;
	*shmM=0;
	s=shmP;
	//Initializing with 0
	for(i=0;i<10;i++)
		{
			*s=0;
			s++;
		}
	
	
	while(1)
	{
		
		s=shmP;
		enteriesP=*shmP;
		if(*shmP>0 && current<=*shmP)
		{	
			++Count;
			
			switch(shmP[current*3-2])
			{
				case 1: //request for free buffer from process
						cout<<"\nProcess with process id: "<<shmP[current*3-1]<<" requested block number: "<<shmP[current*3]<<endl; 
						processId=shmP[current*3-1],requestedBlockNo=shmP[current*3];
						++current;
				
						
					again:	status=b.getBlock(requestedBlockNo);
						if(status=="Buffer Allocated")
							{
								
								cout<<"\nBuffer is allocated to process: "<<processId<<" for block number: "<<requestedBlockNo;
								
								enteriesM=*shmM;
								Count1=0;
								if(enteriesM)
								{
									s=shmM+1;
									while(Count1<enteriesM)
									{

										if(*s==processId)
											{
												*(s+1)=1;
												goto brk;
											}
										s+=2;
										++Count1;
									}

								}
								shmM[enteriesM*2+1]=processId;
								shmM[enteriesM*2+2]=1;

								*shmM=*shmM+1;
								
							}
							
						else if(status=="Buffer Busy"|| "No Free Buffer")
						{
							if(status=="Buffer Busy")
								{
									cout<<"\nBuffer is busy for block number: "<<requestedBlockNo<<endl;
									busyBuffer[requestedBlockNo].push_back(processId);
								}
							
							else
								{
									cout<<"\nNo block on the hash queue for block number: "<<requestedBlockNo<<" and free list of buffers is empty \n";		
									emptyFreeList.push_back(processId);
								}

								enteriesM=*shmM;
								shmM[enteriesM*2+1]=processId;
								shmM[enteriesM*2+2]=2;
								*shmM=*shmM+1;
								
							
						}
						b.display();
							
						brk: break;
			case 2: //buffer released by process
		
					freedBlockNo=shmP[current*3-1],delayedWrite=shmP[current*3];

					++current;
					cout<<"\nBlock number "<<freedBlockNo<<" has been successfully released";
					released=b.updateStatusToFree(freedBlockNo);
					if(released!=NULL)
					{
						b.addToFreeList(released);
						if(delayedWrite)
						{
							cout<<"\nBuffer is marked delayed write by kernel";
							b.markDelayedWrite(released);
						}
						cout<<"\nBuffer successfully added to end of free list\n";
						b.display();
					}
					else
						cout<<"\nError while releasing buffer";
					wakeUp();
					break;

			}
			
		}
		
    			
	}
	
    shmdt(shmP);	//detach from shared memory  
    shmdt(shmM);

   return 0; 
} 

