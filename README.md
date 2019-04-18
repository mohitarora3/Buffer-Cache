# Buffer Cache

A simple buffer cache implementation for simulation of `getblk` and `brelse` algorithms that that fulfills allocation and release requests of processes for blocks.

***Note:-*** Because there is one to one correspondence between block and buffer, buffer and block are used interchangeable in this readme.


## There are two programs:-
1.   main_handler.cpp
2.   process.cpp

## How to run:- 
    g++ -o main main_handler.cpp
    ./main

    g++ -o p1 process.cpp
    ./p1
    
    g++ -o p2 process.cpp       
    ./p2                        
    
    g++ -o p3 process.cpp
    ./p3
    
    g++ -o p4 process.cpp
    ./p4
        . 
        .
        .

    g++ -o p<n> process.cpp
    ./p<n>



Main_handler runs in **infinite loop** that continuously looking for buffer allocation and buffer release requests.

Processes request main_handler for a particular block through **shared memory**.

**Multiprogramming** is achieved by running same process.cpp from various terminals.

Process runs until buffer is **allocated**  for requested block. 

Main_handler fulfills **requests** of block for every process. 



## Case 1: Process requests for a block and it exists in hashQueue and  corresponding buffer is free.
- Main_handler simply marks the buffer busy and removes the buffer from free list. 
- Main_handler then writes on shared memory (shmM) to inform the process that its buffer allocation request is accomplished.

## Case 2: Process requests for a block and it exists in hashQueue but its corresponding buffer is busy.
- Main_handler puts the corresponding blockNumber and processId (for later reference i.e to wake up the corresponding process when that buffer becomes free/available) into vector (busyBuffer). 
- And informs the process that corresponding buffer is busy through shared memory (shM). 
- Process goes to sleep indefinitely until it is woken up by the signal from main_handler.

## Case 3: Process requests for a block and it doesn't exist in hashQueue.
- Main_handler then removes the first free buffer from freeList given that it not marked delayed write, and updates the blockNumber to the requested  blockNumber, removes the buffer from previous hashQueue and adds the buffer to the corresponding new hashQueue.
- Main_handler then writes on shared memory (shmM) to inform the process that its buffer allocation request is accomplished.

## Case 4: Process requests for a block and it doesn't exist in hashQueue, and first free buffer from freeList is marked delayed write. 
- To perform asynchronous write of a delayed write marked buffer, child is created using fork system call by main_handler.
- Main_handler repeats this (above point) until it finds the non delayed write marked buffer.
- Main_handler updates the blockNumber of buffer from freeList that is not marked delayed write to the requested blockNumber, removes the buffer from previous hashQueue and adds the buffer to the corresponding new hashQueue. 
- Main_handler then writes on shared memory (shmM) to inform the process that its buffer allocation request is accomplished.
- And later when the asynchronous write of a buffer is completed by child, main_handler is interrupted by signal. Then main_handler adds the corresponding buffer to the front of free list.
- Moreover main_handler wakes up sleeping processes (info exists in two vectors: freeBuffer and emptyFreeList ) if exists through signal.

## Case 5: Process request for a block, it doesn't exist in hashQueue and freeList is empty.
- Main_handler informs the process that there is no buffer available through shared memory(shmM).
- Main_handler puts the corresponding blockNumber and processId (for later reference i.e to wake up the corresponding process when any buffer becomes free/available) into vector (emptyFreeList).
- When later any buffer is released by a process, all sleeping processes are woken up by main_handler through signal.

 
***Note:-*** When a buffer is released by a process, main_handler will automatically wake up all sleeping processes.


## Assumptions:-

- Process will done with block after a fixed amount of time (10 seconds) and  corresponding buffer will be released by main_handler and buffer will be marked free. 

- A process can't request for another block until its first request is completed i.e a    process can request for a single block at a time.



## Communication between main_handler(that runs getBlock) and processes(that requests for buffer allocation or release) is accomplished through 2 shared memories.


1. shmP (requests from processes for a block)
    - direction from process to main_handler i.e processes write and main_handler reads.

2. shmM (process get to know whether buffer is allocated or not through this shared memory)
    - direction from main_handler to process i.e main_handler writes processes read.


shmP:-
-    It is an integer pointer which points to shared memory.
-    Processes insert into this integer array to request for a block and also to release for a block.
-    First entry in shmP tells the count of number of total requests i.e block allocation and block release requests.
-    Main handler initializes the first entry with 0 to indicate no requests in the beginning.
-    Starting from second entry each triplet indicates either a buffer allocation request or a buffer release request.
### Two types of triplets are used to distinguish between block allocation and release requests.
 
'''
1.  (1, processId, blockNumber)
                - 1 indicates buffer allocation request.
                - processId- process id of process.
                blockNumber- blockNumber of disk block.
2. (2, processId, 0/1)
                - 2 indicates buffer release request.
                - processId- process id of process.
                - 0-release request of read block (i.e initially buffer is allocated for read request)
                - 1-release request of write block (i.e initially buffer is allocated for write request)
'''
***Processes increment first entry of shared memory (shmP) for each request.***

shmM:-
-      It is an integer pointer which points to shared memory.
-       Main_handler writes response of processes request's into it and processes read from it.
-       First entry in shmM tells the count of number of total responses.
-       Starting from second entry each pair indicates response of buffer allocation request.
         
           ``` Pair :- (processId,1/2)
                - processId- process id of process.
                - 1 indicates buffer has been successfully allocated to the corresponding process.
                - 2 indicates buffer has not been allocated. ```



    


