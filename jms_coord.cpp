#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <signal.h>
#include <iostream>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "domes.h"
#define  READ 0
#define  WRITE 1
#define  BUFSIZE 100
#define FINISHED 0
#define ACTIVE 4
using namespace std;
								
								
bool sig_term_recieved=false;

void sig_handler(int signo)
{
  if (signo == SIGTERM){
  	printf("POOL: received SIGTERM\n");
  	sig_term_recieved=true;
  }
	
}



int main( int argc,char ** argv){
	int given_jid, max_jobs, global_jid=0, pool_no=0 ,bytes, jms_in, jms_out;
	char response[BUFSIZE], message[BUFSIZE];
	PoolList * listOfPools = new PoolList();
	char * jmsin_string=NULL;
	char * jmsout_string=NULL;
	
	// PARSING GRAMMI ENTOLWN
	char * path=NULL;
	for (int i = 1; i < argc; i++) {
		if(strcmp(argv[i],"-n")==0) {
			if(i<argc-1) {
				max_jobs=atoi(argv[i+1]);
			} else {
				cout<<"dwste jobs_pool"<<endl;
				return -1;
			}
		} else if(strcmp(argv[i],"-l")==0) {
			if(i<argc-1) {
				path=(char *)malloc(strlen(argv[i+1])+1);
				strcpy(path, argv[i+1]);
			} else {
				cout<<"dwste to path meta to -l"<<endl;
				return -2;
			}
		} else if(strcmp(argv[i],"-w")==0) {
			if(i<argc-1) {
			
				jmsout_string=(char *)malloc(strlen(argv[i+1])+1);
				strcpy(jmsout_string, argv[i+1]);
			} else {
				cout<<"dwste to onoma tou jms out pipe me -w"<<endl;
				return -2;
			}
		} else if(strcmp(argv[i],"-r")==0) {
			if(i<argc-1) {
				jmsin_string=(char *)malloc(strlen(argv[i+1])+1);
				strcpy(jmsin_string, argv[i+1]);
			} else {
				cout<<"dwste to onoma tou jms in pipe me -r"<<endl;
				return -2;
			}
		} 
	}
	
	if(path==NULL){
		path=(char*)malloc(2*sizeof(char));
		strcpy(path,".");
	}
	
	if(jmsin_string==NULL){
		jmsin_string=(char *)malloc(7*sizeof(char));
		sprintf(jmsin_string,"jms_in");
	}
	if(jmsout_string==NULL){
		char * jmsout_string=(char *)malloc(8*sizeof(char));
		sprintf(jmsout_string,"jms_out");
	}
	

	mkfifo(jmsin_string,0666);
	mkfifo(jmsout_string,0666);
	if( (jms_in = open(jmsin_string, O_RDONLY))==-1){
		perror("Error in opening jms_in for reading"); exit(1);
	}
	if( (jms_out = open(jmsout_string, O_WRONLY))==-1){
		perror("Error in opening jms_in for reading"); exit(1);
	}
	free(jmsout_string);
	free(jmsin_string);
	printf("max jobs is %d\n", max_jobs );

	
	
	while( (bytes=read(jms_in, message, sizeof(message)))>0){
		if( strncmp(message,"submit",6)==0 ){
			//printf("PRIN TO POOL %s\n",message);fflush(stdout);
			Pool * available_pool = listOfPools->getAvailablePool();
			if(available_pool==NULL){
				// FTIAXNW NEO POOL
				char pool_pipe_name_in[512];
				char pool_pipe_name_out[512];
				
				sprintf(pool_pipe_name_in, "%s/pool_in_%d",path, pool_no);
				mkfifo(pool_pipe_name_in,0666);
	
				sprintf(pool_pipe_name_out, "%s/pool_out_%d",path, pool_no);
				mkfifo(pool_pipe_name_out,0666);
	
				pool_no++;
				int fd_in;
				int fd_out;
				int pid;
					
				if ( (pid = fork()) == -1 ){ perror("fork"); exit(1); }
				if ( pid == 0 ){    // KWDIKAS POOL
					if (signal(SIGTERM, sig_handler) == SIG_ERR)
						printf("\ncan't catch SIGTERM\n");
					// POOL
					pid_t  pid; 
					int bytes;
					int fd_in,fd_out;
					char message[BUFSIZE];
					
					int times=0;
					if( (fd_in = open(pool_pipe_name_in, O_RDONLY))==-1){
						perror("Error in opening pool_in for reading"); exit(1);
					}
					if( (fd_out = open(pool_pipe_name_out, O_WRONLY))==-1){
						perror("Error in opening pool_out for reading"); exit(1);
					}
					Job * pool_jobs[max_jobs];
					for(int i=0;i<max_jobs;i++){
						pool_jobs[i]=NULL;
					}
					while(!sig_term_recieved && (bytes=read(fd_in, message, sizeof(message)))>0){
						
						char * command=message;
						if(command!=NULL){
// 							if(strncmp(command,"SIGTERM",7) == 0){
// 								
// 								break;
// 							}
// 							else 
							if(strncmp(command,"status",6) == 0){
					
								int child_given=atoi(command+7); //pername tin leksi "status" kai to keno
								int status;
								pid_t result = waitpid(child_given, &status, WNOHANG | WUNTRACED | WCONTINUED);
								//printf("RESULT %d\n",result);
								if(result<0){
									if(errno==EINTR){
										printf("FUCK\n");
									}
									if(errno==ECHILD){
										printf("FUCK 2\n");
									}
									if(errno==EINVAL){
										printf("FUCK 3\n");
									}
									//printf("OOOOOOOOO\n");
									exit(2);
								}
								if(result>0){
									if(WIFEXITED(status)){
										
										sprintf(response, "%d", 0);
										write(fd_out, response, sizeof(response));
									}else if(WIFSIGNALED(status)){
										
										sprintf(response, "%d", 1);
										write(fd_out, response, sizeof(response));
									}
									else if(WIFSTOPPED(status)){
										
										sprintf(response, "%d", 2);
										write(fd_out, response, sizeof(response));
									}
									else if(WIFCONTINUED(status)){
										
										sprintf(response, "%d", 3);
										write(fd_out, response, sizeof(response));
									}
									else{
										printf("status is unknown for pid %d\n",child_given);
										
										sprintf(response, "%d", -1);
										write(fd_out, response, sizeof(response));
									}
								}
								else{
									sprintf(response, "%d", 4);
									write(fd_out, response, sizeof(response));
								}		
							}else{
								if(times>=max_jobs){
									break;
								}
								char * jobid_string= strtok(command," ");
								int jobid=atoi(jobid_string);
								command=command+(strlen(jobid_string)+1);
								
								//printf("PRIN TO FORK\n");fflush(stdout);
								
								if ( (pid = fork()) == -1 ){ perror("fork"); fflush(stdout);fflush(stderr); exit(1); }
							
								
													
								if ( pid == 0 ){    //KWDIKAS JOB
									char directory_name[50];
									char file_output_name[100];
									time_t t = time(NULL);
									struct tm tm = *localtime(&t);
									sprintf(directory_name,"%s/sdi1100060_%d_%d_%d.%d.%d_%d.%d.%d",path,jobid,getpid(),tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_hour,tm.tm_min,tm.tm_sec);
									mkdir(directory_name, 0700);
									sprintf(file_output_name,"%s/%s_%d",directory_name,"stdout",jobid);
									int file = open(file_output_name, O_APPEND | O_WRONLY | O_CREAT);
									if(file < 0){
										printf("Error opening file....\n");
									    exit(1);
									}
									if(dup2(file,1) < 0){
										printf("Error in dup....\n");
										return 1;
									}
									sprintf(file_output_name,"%s/%s_%d",directory_name,"stderr",jobid);
									file = open(file_output_name, O_APPEND | O_WRONLY | O_CREAT);
									if(file < 0){
										printf("EError opening file.....\n");
									    exit(1);
									}
									if(dup2(file,2) < 0){
										printf("Error in dup....\n");
										return 1;
									}
									
							
									char ** args = (char **)malloc(10 * sizeof(char*));
									int arg=0;
									char * token=strtok(command," ");
									args[0]=token;
									arg=1;
									while((token=strtok(NULL," "))!=NULL){
										args[arg]=token;
									
										arg++;
									}
									args[arg]=NULL;
									execv(args[0],args);
									exit(0);
								}
								else{    //PARENT CODE
									pool_jobs[times]=new Job(-1,pid);
									sprintf(response, "%d", pid);
									write(fd_out, response, sizeof(response));
					
									times++;
									if(times>=max_jobs){
										printf("FTASAME MAX JOBS PREPEI NA FTIAXTEI NEO POOL\n");
										break;
									}
								}
							}
						}
					}
					if(sig_term_recieved){
						for(int i=0;i<max_jobs;i++){
							if(pool_jobs[i]!=NULL){
								kill(pool_jobs[i]->getPid(),SIGTERM);
							}
						}
					}

					
					while (wait(NULL) > 0);
					printf("Finished writing to pipe..\n");
					
					sprintf(response, "FINISHED");
					write(fd_out, response, sizeof(response)); 
					exit(0);
				}
				else{                // COORD CODE
					if( (fd_in = open(pool_pipe_name_in, O_WRONLY))==-1){
						perror("ERROR"); exit(1);
					}
					if( (fd_out = open(pool_pipe_name_out, O_RDONLY))==-1){
						perror("ERROR"); exit(1);
					}
					Pool * pool=new Pool(pid,max_jobs,fd_in,fd_out); printf("FTIAXTIKE NEO POOL\n");
					listOfPools->add(pool);
					available_pool=pool;
				}
			}
					
			char entoli[BUFSIZE];
			strcpy(entoli,&message[7]);
			int returned_pid=available_pool->addJob(global_jid,entoli);
			
			sprintf(entoli, "JobID: %d, PID: %d", global_jid,returned_pid);
			write(jms_out, entoli, sizeof(entoli));
			global_jid++;
			printf("%s successfully executed\n", &message[7] );
		} // TELOS IF SUBMIT
		else if( strncmp(message,"status-all",10)==0 ){
			char entoli[BUFSIZE];
			listOfPools->statusAll(jms_out);
			strcpy(entoli,"END-ALL");
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		else if( strncmp(message,"status",6)==0 ){
			given_jid=atoi(&message[7]);
			int status=listOfPools->getJobStatus(given_jid);
			char entoli[BUFSIZE];
			char status_s[15];

			switch(status){
				case -1: strcpy(status_s,"Not exists"); break;
				case 0: strcpy(status_s,"Finished"); break;
				case 1: strcpy(status_s,"Signal"); break;
				case 2: strcpy(status_s,"Suspended"); break;
				case 3: strcpy(status_s,"Continued"); break;
				default: strcpy(status_s,"Active"); break;
			}

			sprintf(entoli, "JobID: %d Status: %s", given_jid,status_s);
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		else if( strncmp(message,"suspend",7)==0 ){
			given_jid=atoi(&message[8]);
			bool success=listOfPools->suspendJob(given_jid);
			char entoli[BUFSIZE];
			if(success)
				sprintf(entoli, "Sent suspend signal to JobID %d", given_jid);
			else
				sprintf(entoli, "Den uparxei Job me id  %d", given_jid);
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		else if( strncmp(message,"resume",6)==0 ){
			given_jid=atoi(&message[7]);
			bool success=listOfPools->continueJob(given_jid);
			char entoli[BUFSIZE];
			if(success)
				sprintf(entoli, "Sent resume signal to JobID %d", given_jid);
			else
				sprintf(entoli, "Den uparxei Job me id  %d", given_jid);
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		else if( strncmp(message,"show-active",11)==0 ){	
			listOfPools->showActive(jms_out);
			char entoli[BUFSIZE];
			strcpy(entoli,"END-ALL");
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
			
		}
		else if( strncmp(message,"show-finished",13)==0 ){
			listOfPools->showFinished(jms_out);
			char entoli[BUFSIZE];
			strcpy(entoli,"END-ALL");
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		else if( strncmp(message,"show-pools",10)==0 ){
			char entoli[BUFSIZE];
			strcpy(entoli,"Pool\tNumOfJobs(Active)\tTotalJobs");
			write(jms_out, entoli, sizeof(entoli));
			listOfPools->showPool(jms_out);
			strcpy(entoli,"END-ALL");
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		else if(strncmp(message,"shutdown",8)==0 ){
			int still_in_progress=listOfPools->sendSigterm();
			char entoli[BUFSIZE];
			sprintf(entoli, "Served %d jobs, %d were still in progress",global_jid, still_in_progress);
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
			break;
		}else{
			char entoli[BUFSIZE];
			sprintf(entoli, "Lathos entoli");
			write(jms_out, entoli, sizeof(entoli));
			printf("%s successfully executed\n", message);
		}
		
		
	}  // TELOS WHILE
	
	wait(NULL); // wait for all pools to finish
	exit(0);
}
