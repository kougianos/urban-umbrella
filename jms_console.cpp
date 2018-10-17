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
#include <errno.h>
#define  READ 0
#define  WRITE 1
#define  BUFSIZE 100
using namespace std;


int main( int argc,char ** argv){
	int bytes;
	char message[BUFSIZE];
	char * jmsin_string=NULL;
	char * jmsout_string=NULL;
	
	// PARSING GRAMMI ENTOLWN
	char * path=NULL;
	char * operations_file=NULL;
	for (int i = 1; i < argc; i++) {
		if(strcmp(argv[i],"-o")==0) { 
			if(i<argc-1) {
				operations_file=(char *)malloc(strlen(argv[i+1])+1); 
				strcpy(operations_file, argv[i+1]);
			} else {
				cout<<"dwste operations file"<<endl;
				return -1;
			}
		} else if(strcmp(argv[i],"-r")==0) {
			if(i<argc-1) {
			
				jmsout_string=(char *)malloc(strlen(argv[i+1])+1);
				strcpy(jmsout_string, argv[i+1]);
			} else {
				cout<<"dwste to onoma tou jms out pipe me -w"<<endl;
				return -2;
			}
		} else if(strcmp(argv[i],"-w")==0) {
			if(i<argc-1) {
				jmsin_string=(char *)malloc(strlen(argv[i+1])+1);
				strcpy(jmsin_string, argv[i+1]);
			} else {
				cout<<"dwste to onoma tou jms in pipe me -r"<<endl;
				return -2;
			}
		} 
	}
	
	if(jmsin_string==NULL){
		jmsin_string=(char *)malloc(7*sizeof(char));
		sprintf(jmsin_string,"jms_in");
	}
	if(jmsout_string==NULL){
		char * jmsout_string=(char *)malloc(8*sizeof(char));
		sprintf(jmsout_string,"jms_out");
	}
	
	
	int jms_in,jms_out;
	if( (jms_in = open(jmsin_string, O_WRONLY))==-1){
		perror("pool: pipe jms_in error opening for writing"); exit(1);
	}
	if( (jms_out = open(jmsout_string, O_RDONLY))==-1){
		perror("pool: pipe jms_out error opening for reading"); exit(1);
	}
	
	char *line = NULL;
    size_t len = 0;
    size_t read1;
	if(operations_file!=NULL){
		int file=open(operations_file,O_RDONLY);
		dup2(file,0);
	}
    
	while ((read1 = getline(&line, &len, stdin)) != -1) {
     
        line[strlen(line)-1]='\0';
        write(jms_in, line, BUFSIZE);
		
		if(strncmp(line,"show",4)==0 || strncmp(line,"status-all",10)==0){
			while((bytes=read(jms_out, message, sizeof(message)))>0){
				if(strcmp(message,"END-ALL")==0){
					break;
				}
				printf("%s\n",message);
			}
		}else{
			bytes=read(jms_out, message, sizeof(message));
			printf("%s\n",message);
			if(strncmp(message, "Served", 6)==0) exit(0);
		}
		printf("\n");
    }
    free(line);
	
	exit(0);
	
}