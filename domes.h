#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // mkfifo
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#define  READ 0
#define  WRITE 1
#define  BUFSIZE 100
#define MAX_JOBS 3
#define FINISHED 0
#define ACTIVE 4
using namespace std;



class Job{
	int jid;
	int pid;
	int status;
	bool finished;
public:

	Job(int jid,int pid){
		this->jid=jid;
		this->pid=pid;
		this->finished=false;
	}
	
	int getPid(){
		return pid;
	}
	int getJid(){
		return jid;
	}
	void setFinished(){
		finished=true;
	}
	bool isFinished(){
		if(finished) return true;
		else return false;
	}
};

class Pool{
	const int max_jobs;
	int current_jobs;
	bool dead;
	Job **jobs;
	
	int fd_in;
	int fd_out;
	int pid;
	
public:
	Pool(int pid,int max,int fd_in,int fd_out):max_jobs(max){
		this->pid=pid;
		jobs=new Job*[max_jobs];
		current_jobs=0;
		dead=false;
		this->fd_in=fd_in;
		this->fd_out=fd_out;
	}
	
	int getPid(){
		return pid;
	}
	
	~Pool(){
		for(int i=0;i<current_jobs;i++){
			delete jobs[i];
		}
		delete [] jobs;
	}
	
	bool isFull(){
		if(current_jobs>=max_jobs) return true;
		else return false;
	}
	
	int addJob(int jid,char * entoli){
		if(isFull() == true){
			return -1;
		}
		//strcpy(entoli,"/bin/ls -l");
		
		// if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
// 			perror("signal");
		char newentoli[100];
		sprintf(newentoli,"%d %s",jid,entoli);
		if(write(fd_in, newentoli, BUFSIZE)==-1){
			if (errno == EPIPE) {
				/* Closed pipe. */
				printf("ERROR IN PIPE");
				exit(1);
			}
		}else{
			int bytes=read(fd_out, entoli, BUFSIZE); // diavazw to response to opoio einai to pid
			//printf("3e--KOLLISE EDW\n"); fflush(stdout);
			if(bytes>0){
				//printf("Got response: %s from pool\n",entoli);
				//fflush(stdout);
			}else{
				printf("Didn't get response from pool\n");
				fflush(stdout);
				exit(1);
			}
		}
		
		//write(fd_in, entoli, BUFSIZE);
		//printf("KOLLISE EDW\n"); fflush(stdout);
		
		int pid=atoi(entoli);
		
		jobs[current_jobs]=new Job(jid,pid);
		current_jobs++;
		
		return pid;
	}
	
	bool containsJob(int jid){
		int flag = 0;
		for(int i=0;i<current_jobs;i++){
			if(jobs[i]->getJid()==jid){
				flag = 1;
			}
		}
		if(flag == 1) return true;
		return false;
	}
	
	int getJobStatus(int jid){
		// if pool is dead
		if(isDead() == true){
			return 0; // finished
		}
		for(int i=0;i<current_jobs;i++){
			if(jobs[i]->getJid()==jid){
				if(jobs[i]->isFinished()){
					//printf("CHECK THIS\n");
					fflush(stdout);
					return 0;
				}
				
				char entoli[BUFSIZE];
				sprintf(entoli, "status %d", jobs[i]->getPid());
				
				
				if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    				perror("signal");
				write(fd_in, entoli, sizeof(entoli));
				int bytes=read(fd_out,entoli,sizeof(entoli));
		
				if(bytes<=0){
					printf("Error getting response for status...\n");
					exit(1);
				}else{
					//printf("Got response %d bytes from pool for state: %s\n",bytes,entoli);
				}
				if(strcmp(entoli,"FINISHED")==0){
					//printf("Coordinator[] Got FINISHED from pool...Marking dead...\n");fflush(stdout);
					dead=true;
					return 0;
				}else{
					int response=atoi(entoli);
					if(response==0){
						jobs[i]->setFinished();
					}
					return response;				
				}
				
			}
		}
		return -1;
	}
	
	bool suspendJob(int jid){
		for(int i=0;i<current_jobs;i++){
			if(jobs[i]->getJid()==jid){
				kill(jobs[i]->getPid(),SIGSTOP);
				return true;
			}
		}
		return false;
	}
	bool continueJob(int jid){
		for(int i=0;i<current_jobs;i++){
			if(jobs[i]->getJid()==jid){
				kill(jobs[i]->getPid(),SIGCONT);
				return true;
			}
		}
		return false;
	}
	
	bool isFinishedJob(int jid){
		for(int i=0;i<current_jobs;i++){
			if(jobs[i]->getJid()==jid){
				if(jobs[i]->isFinished() == true){
					return true;
				}else{
					// get status
					if(getJobStatus(jid)==0){
						return true;
					}
				}
				
			}
		}
	}
	
	void statusAll(int jms_out){
		for(int i=0;i<current_jobs;i++){
		//	printf("KOLLISE\n"); fflush(stdout);
			int status=getJobStatus(jobs[i]->getJid());
			char entoli[BUFSIZE];
			char status_s[20];
			switch(status){
				case -1: strcpy(status_s,"Not exists"); break;
				case 0: strcpy(status_s,"Finished"); break;
				case 1: strcpy(status_s,"Signal"); break;
				case 2: strcpy(status_s,"Suspended"); break;
				case 3: strcpy(status_s,"Continued"); break;
				default: strcpy(status_s,"Active"); break;
			}
			sprintf(entoli, "JobID: %d Status: %s", jobs[i]->getJid(),status_s);
			write(jms_out, entoli, sizeof(entoli));
		}
	}
	
	void showActive(int jms_out){ 
		for(int i=0;i<current_jobs;i++){
			int status=getJobStatus(jobs[i]->getJid());
			if(status==ACTIVE){
				char entoli[BUFSIZE];
				sprintf(entoli, "JobID: %d", jobs[i]->getJid());
				write(jms_out, entoli, sizeof(entoli));
			}
		}
	}
	
	int showActive(){
		int active=0;
		for(int i=0;i<current_jobs;i++){
			int status=getJobStatus(jobs[i]->getJid());
			if(status==ACTIVE){
				active++;
			}
		}
		return active;
	}
	
	void showFinished(int jms_out){
		for(int i=0;i<current_jobs;i++){
			int status=getJobStatus(jobs[i]->getJid());
			if(status==FINISHED){
				char entoli[BUFSIZE];
				sprintf(entoli, "JobID: %d", jobs[i]->getJid());
				write(jms_out, entoli, sizeof(entoli));
			}
		}
	}
	
	void showPool(int jms_out){
		if(isDead()){
			return;
		}
		int active=0;
		for(int i=0;i<current_jobs;i++){
			int status=getJobStatus(jobs[i]->getJid());
			fflush(stdout);
			if(status==ACTIVE){
				active++;
			}
		}
		char entoli[BUFSIZE];
		sprintf(entoli, "%d\t%d\t%d",pid, active,current_jobs);
		write(jms_out, entoli, sizeof(entoli));
	}
	
	int sendSigterm(){
		if(isDead() == true){ //already dead
			return 0;
		}
		//char entoli[BUFSIZE];
		//sprintf(entoli, "SIGTERM");
		int active=showActive();
		kill(pid,SIGTERM);
//		write(fd_in, entoli, sizeof(entoli));
		return active;
	}
		
	bool isDead(){
		return dead;
	}
};

class PoolNode
{
public:
	Pool* data;
	PoolNode* previous;
	PoolNode* next;

	PoolNode(Pool* d, PoolNode * n){
		this->data = d;
		this->next = n;
	}
	~PoolNode(){
		delete data;
	}
};

class PoolList
{
	PoolNode * start;
public:
	PoolList(){
		start = NULL;
	}

	~PoolList(){
		PoolNode * temp1 = start;
		while(temp1!=NULL){
			PoolNode * temp2 = temp1->next;
			delete temp1;
			temp1 = temp2;
		}
	}

	PoolNode *getStart(){
		return start;
	}

	bool add(Pool * p){
		if(start ==  NULL){
			start = new PoolNode(p, NULL);
		}else{
			PoolNode * newnode = new PoolNode(p, start);
			start = newnode;
		}
		if(start == NULL){
			return false;
		}
		return true;
	}
	
	Pool * getAvailablePool(){
		if(start!=NULL && !start->data->isFull()){
			return start->data;
		}
		return NULL;
	}
	
	int getJobStatus(int jid){
		PoolNode * temp = start;
		int jobStatus;
		
		while(temp!=NULL){
		//printf("EEEEE 1\n");
			if(temp->data->containsJob(jid) == true){
				jobStatus = temp->data->getJobStatus(jid);
				//printf("EEEEE 2\n");
				return jobStatus;
			}
			temp = temp->next;
		}
		//printf("EEEEE 3\n");
		return -1; // an epistrafei -1 simainei oti den uparxei job me to dothen jid
	}
	
	bool suspendJob(int jid){
		PoolNode * temp = start;
		bool flag = false;
		while(temp!=NULL){
			if(temp->data->containsJob(jid) == true){
				flag = temp->data->suspendJob(jid);
				return flag;
			}
			temp = temp->next;
		}
		return false; 
	}
	bool continueJob(int jid){
		PoolNode * temp = start;
		bool flag = false;
		while(temp!=NULL){
			if(temp->data->containsJob(jid)){
				flag = temp->data->continueJob(jid);
				return flag;
			}
			temp = temp->next;
		}
		return false; 
	}

	void statusAll(int jms_out){
		PoolNode * temp = start;
		while(temp!=NULL){
			temp->data->statusAll(jms_out);
			temp = temp->next;
		}
	}
	
	void showActive(int jms_out){
		PoolNode * temp = start;
		while(temp!=NULL){
			temp->data->showActive(jms_out);
			temp = temp->next;
		}
	}
	void showFinished(int jms_out){
		PoolNode * temp = start;
		while(temp!=NULL){
			temp->data->showFinished(jms_out);
			temp = temp->next;
		}
	}
	
	void showPool(int jms_out){
		PoolNode * temp = start;
		while(temp!=NULL){
			temp->data->showPool(jms_out);
			temp = temp->next;
		}
	}
	
	int sendSigterm(){
		int active=0;
		PoolNode * temp = start;
		while(temp!=NULL){
			active+= temp->data->sendSigterm();
			temp = temp->next;
		}
		return active;
	}
};
