#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // getpid()

#define PROCESS_NUM 5
#define MAX_TIME 50
#define TIME_QUANTUM 2

static int runMutex = 0;
typedef struct{
        int PID;
        int arrivalTime;
        int CPU_burstTime;
        int IO_startTime; //IO start time after process starts running
        int IO_burstTime;
        int priority;
        int CPU_endTime;
        int CPU_runningTime;
        int initial_CPU_burstTime;
        int initial_IO_burstTime;
} Process;

// Comparing priority between two processes
typedef int PriorityComp(Process p1, Process p2);

typedef struct{
	PriorityComp * comp;
	int numProcess;
	Process processArr[MAX_TIME];
} Queue;

// Generate random number
int generateRandNum(int n){
  int result;
  result = (rand() % n) + 1;
  return result;
}

// Initialize process
void initProcess(Process * proc){
        int randomArrival = generateRandNum(5);
        int randomCPU     = generateRandNum(10);
        int randomIO      = generateRandNum(5);
        int randomIOStart = generateRandNum(3);
        int priority      = generateRandNum(PROCESS_NUM);
        proc->arrivalTime   = randomArrival;
        proc->CPU_burstTime = randomCPU;
        proc->IO_burstTime  = randomIO;
        proc->IO_startTime  = randomIOStart;
        proc->priority      = priority;
        proc->CPU_endTime   = -1;
	      proc->CPU_runningTime = 0;
        proc->initial_CPU_burstTime = proc->CPU_burstTime;
        proc->initial_IO_burstTime = proc->IO_burstTime;
}

void printProcess(Process proc){
  //printf("Process ID, arrivalTime, CPU_burstTime, IO_burstTime, IO_startTime, priority, CPU_endTime, CPU_runningTime, initial_CPU_burstTime, initial_IO_burstTime %d %d %d %d %d %d %d %d %d %d\n", proc.PID, proc.arrivalTime, proc.CPU_burstTime, proc.IO_burstTime, proc.IO_startTime, proc.priority, proc.CPU_endTime, proc.CPU_runningTime, proc.initial_CPU_burstTime, proc.initial_IO_burstTime);	
	printf("Process ID %d\n", proc.PID);
	printf("- arrivalTime: %d\n", proc.arrivalTime);
	printf("- CPU_burstTime: %d\n", proc.CPU_burstTime);
	printf("- IO_burstTime: %d\n", proc.IO_burstTime);
	printf("- IO_startTime: %d\n", proc.IO_startTime);
	printf("- Priority: %d\n", proc.priority);
	printf("-------------------\n");
}

// ----
// Queue for FCFS, RR scheduling
// ----
typedef struct node{
	Process proc;
	struct node * next;
} Node;

typedef struct{
	Node * front;
	Node * rear;
} QueueList;

void queueListInit(QueueList * queue){
	queue->front = NULL;
	queue->rear = NULL;
}

int isQueueListEmpty(QueueList * queue){
	if(queue->front == NULL){
		return 1;
	}else{
		return 0;
	}
}

void enqueueList(QueueList * queue, Process new_proc){
	Node * newNode = (Node *)malloc(sizeof(Node));
	newNode->proc = new_proc;
	newNode->next = NULL;

	if(isQueueListEmpty(queue)){
		queue->front = newNode;
		queue->rear = newNode;
	}else{
		queue->rear->next = newNode;
		queue->rear = newNode;
	}
}

Process dequeueList(QueueList * queue){
	Node * delNode;
	delNode = queue->front;
	Process result = delNode->proc;
	queue->front = delNode->next;

	free(delNode);
	return result;
}

// ----
// Queue for SJF, Priority Scheduling
// ----
// Initialize Queue
void initQueue(Queue * queue, PriorityComp pc){
  int i;
  for(i=0;i<MAX_TIME;i++){
    Process * p = &(queue->processArr[i]);
    p->PID=-1;
  }
	queue->numProcess = 0;
	queue->comp = pc;	// Initialize standard for priority of the queue
}

// Check if queue is empty
int isQueueEmpty(Queue * queue){
	if(queue->numProcess <= 0){
		return 1;
	}else{
		return 0;
	}
}

// Get parent node index
int getParentIdx(int idx){
	return idx/2;
}

// Get left child node index
int getLeftChildIdx(int idx){
	return idx*2;
}

// Get right child node index
int getRightChildIdx(int idx){
	return idx*2 + 1;
}

// Get higher priority child node
int getHigherChild(Queue * queue, int idx){
	if(getLeftChildIdx(idx) > queue->numProcess){
	// Because priority queue is made of heap, which is complete binary tree
	// it cannot have only right child node. In other words, the maxmimum value of queue->numProcess should be <= than left child's index
		return 0;
	}else if(getLeftChildIdx(idx) == queue->numProcess){
		return getLeftChildIdx(idx);
	}else{
		if(queue->comp(queue->processArr[getLeftChildIdx(idx)],
				queue->processArr[getRightChildIdx(idx)]) < 0 ){
			return getRightChildIdx(idx);
		}else{
			return getLeftChildIdx(idx);
		}
	}
}

// Insert process to queue
void enqueue(Queue * queue, Process new_proc){
	// First insert at the last index
	int idx = queue->numProcess + 1;

	while(idx != 1){
		if(queue->comp(new_proc, queue->processArr[getParentIdx(idx)]) > 0){
			// Switch with parent node
			queue->processArr[idx] = queue->processArr[getParentIdx(idx)];
			idx = getParentIdx(idx);
		}else{
			// Stay if it's priority is lower than parent node
			break;
		}
	}

	queue->processArr[idx] = new_proc;
	queue->numProcess += 1;
}

// Dequeue a process from queue
Process dequeue(Queue * queue){
	Process dequeuedProcess = queue->processArr[1];
	Process lastProcess = queue->processArr[queue->numProcess];

	int parentIdx = 1;
	int childIdx;

	// Start from higher child node
	while((childIdx = getHigherChild(queue, parentIdx))){
		// Break if last process's priority is higher than current one
		if(queue->comp(lastProcess, queue->processArr[childIdx]) >= 0){
			break;
		}

		queue->processArr[parentIdx] = queue->processArr[childIdx];
		parentIdx = childIdx;
	}
	queue->processArr[parentIdx] = lastProcess;
	queue->numProcess -= 1;
	return dequeuedProcess;
}

int SJFDataComp(Process p1, Process p2){
	return p2.CPU_burstTime - p1.CPU_burstTime;
}

int LJFDataComp(Process p1, Process p2){
	return p1.CPU_burstTime - p2.CPU_burstTime;
}

int PriorityDataComp(Process p1, Process p2){
	return p2.priority - p1.priority;
}

int LIFODataComp(Process p1, Process p2){
	return p1.arrivalTime - p2.arrivalTime;
}

// ----
// Scheduler for SJF, Priority, LJF, LIFO
// ----
void schedule(Process processes[], int preempt, int algorithm){
  Process nullProcess = {-1,-1,-1,-1,-1,-1,-1,-1};
  Queue readyQueue;
  Process waitingQueue[PROCESS_NUM];
  int currentTime = -1, i, j;
  Process runningProcess = nullProcess;
  int time[PROCESS_NUM]={0,};

  // Initialize readyQueue
  if((algorithm == 2) || (algorithm == 3)){
    initQueue(&readyQueue, SJFDataComp);
  }else if((algorithm == 4) || (algorithm == 5)){
    initQueue(&readyQueue, PriorityDataComp);
  }else if(algorithm == 7){
	initQueue(&readyQueue, LIFODataComp);
  }else if(algorithm == 8){
	 initQueue(&readyQueue, LJFDataComp);
  }

  for(i=0;i<PROCESS_NUM;i++){
    waitingQueue[i] = nullProcess;
  }

  while(currentTime < MAX_TIME){
    currentTime++;
    printf("=================================\n");
    printf("Current Time : %d~%d\n", currentTime, currentTime+1);

    //Enqueue processes when it arrive
    for(i=0;i<PROCESS_NUM;i++){
      if(processes[i].arrivalTime == currentTime){
        printf("Process %d arrived on time %d!\n", processes[i].PID, currentTime);
        enqueue(&readyQueue, processes[i]);
      }

      // Perform IO
      if((processes[i].IO_burstTime >= 0) && (processes[i].CPU_runningTime == processes[i].IO_startTime)){
        if(processes[i].IO_burstTime != 0){
          printf("Process %d handling IO \n", processes[i].PID);
        }
        if(runningProcess.PID == i){
                runningProcess = nullProcess;
        }

        processes[i].IO_burstTime--;
        waitingQueue[i] = processes[i];
      }
    }

    // Run Process
    if((runningProcess.PID == -1) && !(isQueueEmpty(&readyQueue))){
      runningProcess = dequeue(&readyQueue);
    }
    if(runningProcess.PID != -1){
      if(preempt == 1){
        enqueue(&readyQueue, processes[runningProcess.PID]);
        runningProcess = dequeue(&readyQueue);
      }

      printf("-------Process %d running..\n", runningProcess.PID);
      processes[runningProcess.PID].CPU_burstTime--;
      processes[runningProcess.PID].CPU_runningTime++;
      time[runningProcess.PID]++;
      if(processes[runningProcess.PID].CPU_burstTime == 0){
        processes[runningProcess.PID].CPU_endTime = currentTime+1;
        printf("Process %d is Done!\n", runningProcess.PID);
        runningProcess = nullProcess;
      }
    }

    // Back to readyQueue after IO
    for(i=0;i<PROCESS_NUM;i++){
      if(waitingQueue[i].IO_burstTime == 0){
        waitingQueue[i] = nullProcess;
        enqueue(&readyQueue, processes[i]);
      }
    }
  } // While
} // schedule

// ----
// Scheduler for FCFS, RR
// ----
void run_FCFS_RR(Process processes[], int roundRobin){
  Process nullProcess = {-1,-1,-1,-1,-1,-1,-1,-1};
  QueueList readyQueue;
  Process waitingQueue[PROCESS_NUM];
  int currentTime = -1, i, j;
  Process runningProcess = nullProcess;
  int time[PROCESS_NUM]={0,};

  // Initialize readyQueue
  for(i=0;i<PROCESS_NUM;i++){
    queueListInit(&readyQueue);
    waitingQueue[i] = nullProcess;
  }
  while(currentTime < MAX_TIME){
    currentTime++;
    printf("=================================\n");
    printf("Current Time : %d~%d\n", currentTime, currentTime+1);

    //Enqueue processes when it arrive
    for(i=0;i<PROCESS_NUM;i++){
      if(processes[i].arrivalTime == currentTime){
        printf("Process %d arrived on time %d!\n", processes[i].PID, currentTime);
        enqueueList(&readyQueue, processes[i]);
      }

      // Perform IO
      if((processes[i].IO_burstTime >= 0) && (processes[i].CPU_runningTime == processes[i].IO_startTime)){
        if(processes[i].IO_burstTime != 0){
          printf("Process %d handling IO \n", processes[i].PID);
        }
        time[i] = 0;
        if(runningProcess.PID == i){
                runningProcess = nullProcess;
        }
        waitingQueue[i] = processes[i];
        processes[i].IO_burstTime--;
      }
    }

    if((roundRobin==1) && (runningProcess.PID!= -1) && (time[runningProcess.PID] == TIME_QUANTUM)){
      enqueueList(&readyQueue, processes[runningProcess.PID]);
      time[runningProcess.PID] = 0;
      runningProcess = nullProcess;
    }

    // Run Process
    if((runningProcess.PID == -1) && !(isQueueListEmpty(&readyQueue))){
      runningProcess = dequeueList(&readyQueue);
    }
    if(runningProcess.PID != -1){
      printf("-------Process %d running..\n", runningProcess.PID);
      processes[runningProcess.PID].CPU_burstTime--;
      processes[runningProcess.PID].CPU_runningTime++;
      time[runningProcess.PID]++;

      if(processes[runningProcess.PID].CPU_burstTime == 0){
        processes[runningProcess.PID].CPU_endTime = currentTime+1;
        printf("Process %d is Done!\n", runningProcess.PID);
        runningProcess = nullProcess;
      }
    }

    // Back to readyQueue after IO
    for(i=0;i<PROCESS_NUM;i++){
      if(waitingQueue[i].IO_burstTime == 0){
        waitingQueue[i] = nullProcess;
        enqueueList(&readyQueue, processes[i]);
      }
    }

  } // While
} // run_RR

void print_stat(Process processes[]){
  int waiting_time;
  int turnaround_time;

  int total_waiting_time=0;
  int total_turnaround_time=0;

  int i;
  for(i=0;i<PROCESS_NUM;i++){
    waiting_time = processes[i].CPU_endTime - processes[i].arrivalTime - processes[i].initial_IO_burstTime - processes[i].initial_CPU_burstTime;
    turnaround_time = processes[i].CPU_endTime - processes[i].arrivalTime;
    if(waiting_time < 0){
		waiting_time = 0;
	}
	if(turnaround_time < 0){
		turnaround_time = 0;
	}
    printf("Process %d waiting time : %d, turnaround time: %d\n", i, waiting_time, turnaround_time);
   total_waiting_time += waiting_time;
   total_turnaround_time += turnaround_time;
  }

  printf("Average waiting time : %d\n", total_waiting_time / PROCESS_NUM);
  printf("Average turnaround time : %d\n", total_turnaround_time / PROCESS_NUM);
}

int main(){
  int i;
  int select;
  Process myProcess[PROCESS_NUM];

  //Init Processes
  for(i=0;i<PROCESS_NUM;i++){
	//Mute for debugging
    //srand(time(NULL)); 
    //sleep(1);
    myProcess[i].PID = i;
    initProcess(&myProcess[i]);
    printProcess(myProcess[i]);
  }

  printf("SELECT ALGORITHM\n");
  printf("1. FCFS\n2. Non-Preemptive SJF\n3. Preemptive SJF\n4. Priority\n5. Preemptive Priority\n6. RoundRobin\n7. LIFO\n8. Longest-Job First\n");
  scanf("%d", &select);

  switch(select){
    case 1:
    run_FCFS_RR(myProcess, 0);
    break;

    case 2:
    schedule(myProcess, 0, 2);
    break;

    case 3:
    schedule(myProcess, 1, 3);
    break;

    case 4:
    schedule(myProcess, 0, 4);
    break;

    case 5:
    schedule(myProcess, 1, 5);
    break;

    case 6:
    run_FCFS_RR(myProcess, 1);
    break;

	case 7:
	schedule(myProcess, 0, 7);
	break;
	
	case 8:
	schedule(myProcess, 0, 8);
	break;
	
    default:
    printf("Please input valid number\n");
  }

  print_stat(myProcess);

	return 0;
}
