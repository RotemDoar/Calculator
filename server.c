// This program is the server part
// The server is waiting for a signal to start calculate. And after he will calculate the request, and sends back the result to the client.

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <signal.h>
#include <errno.h>

void calculate (int sig); // Function that calculates the customer request. This function occur while getting SIGUSER1
void chld_exit(int sig); // Function for getting signal when the son finish his process
void enter_clientpid_into_buf(char *buffer,pid_t client_pid,int start_point); // function that enters the client pid into buffer
void write_str_into_file(char*str,int fd_srv); // function that writes string into file

int main(){
	int fd_srv; // file descriptor to check if to_srv file has already opend
	
	// change the signals handlers
	signal(SIGUSR1,calculate);
	signal(SIGCHLD,chld_exit);
	
	// check if "to_srv" file has already opened
	fd_srv = open("to_srv", O_RDWR | O_APPEND , 0666); // trying to open the file "to_srv"
	if(fd_srv > 0){ // the file already open
		if(fork() == 0){ // removing to_srv file
	 	        execlp("rm","rm","to_srv",NULL); // removing "to_srv" file and check for error
			printf("Error while removing to_srv file");
			exit(-3); // finish and close the server
		}
		else{
			wait(NULL);
		}
	}
	else{
		if(errno != ENOENT){ // checking if the error is not because that the file "to_srv" has not created 
			printf("Error with to_srv file\n");
			exit(-1);
		}
	}
	

	while(1){
		pause(); // waiting for signal
	}
	
	exit(1);
	
}

// Function that calculates the customer request. This function occur while getting SIGUSER1
void calculate (int sig){
	char buffer[50]; // creating char array size 50 to handle with reading files.
	char to_client[] = "to_client_"; 
	char *cwd;
	int i = 1, counter = 0,line = 1; // i -> return value while reading file.
					  	 // counter -> counter for the buffer.
					  	 // line -> sign for which line we are in the file.
					  	
	int fd_srv, fd_client; // file descriptors:
				// fd_srv -> file descriptor for "to_srv" file.
				// fd_client -> file descriptor for “to_client_xxxxxx” file.
				
	int operand_1, operation, operand_2;//result; // integers for operands, operation, and result of the calculation.
	float result; 
	pid_t client_pid; // pid of the client
	signal(SIGUSR1,calculate); // reset the handler
	// creating son that calculate the client request
	if(fork() == 0){
		if((fd_srv = open("to_srv", O_RDONLY , 0666)) < 0 ){ // open "to_srv" file , and check for errors
			printf("Can't open to_srv file\n"); // printing the error
			exit(-2); // exit and delete the file
		}
		
		//start reading the file and save it in the buffer
		while(i!=0){ // as long as we have not reached the end of the file
			if((i = read(fd_srv,&buffer[counter],1)) < 0 ){ // reading char from "to_srv" file, and check for errors
				printf("Error while reading to_srv file\n");
				exit(-2); // exit and delete the file
			}
			if(buffer[counter]=='\n'){ // if got to the end of a line
				buffer[counter] = '\0'; // end the line with '\0'
					
				switch(line){	
					case 1: 	// if we finish line 1
							line = 2;
							client_pid = atoi(buffer); // setting client_pid to be the client pid 
							operand_1 = counter + 1; // set operand_1 to be the next place in the buffer 
							break;
					case 2:	// if we finish line 2
							line = 3;
							operand_1 = atoi(&buffer[operand_1]); // setting operand_1 to be the first operand
							operation = counter + 1; // set operation be the next place in the buffer
							break;
					case 3: 	//if we funish line 3
							line = 4;
							operation = atoi(&buffer[operation]);// setting operation to be the operation value 
							operand_2 = counter + 1; // set operand_2 be the next place in the buffer
							break;
					case 4: 	// if we finish line 4
							i = 0; // set i to be 0 to stop the loop
							operand_2 = atoi(&buffer[operand_2]);// setting operand_2 to the the second operand value
							break;
					default:
							break;
				}
		
			}

			counter++;
		}
		
		if(fork() == 0){
		 	execlp("rm","rm","to_srv",NULL); // removing "to_srv" file and check for error
			printf("Error while removing to_srv file"); // if there was an error while removing
			exit(-3);  // exit and close the server	
		}
		
		else{
		
		     wait(NULL);	
		}
	
		// calculate the client request
		switch(operation){
			case 1:	// if the operation is addition
					result = operand_1 + operand_2;
					break;
			case 2: 	// if the operation in subtraction
					result = operand_1 - operand_2;
					break;
			case 3:	// if the operation is multiplication
					result = operand_1 * operand_2;
					break;
			case 4:	// if the operation is division
					if(operand_2 == 0){ // check if we divide by zero
						printf("Error - divide by zero \n");
						exit(-1);
					}
					else{
						result = ((float)operand_1 / operand_2);
						break;
					}
			default:	// if we got here then the operation is invalid
					printf("Error - invalid operation\n");
					exit(-1);
					break;
		}
		
		i = 0;
		// changing buffer to be “to_client_xxxxxx” 
		do{ // copy to_client array into buffer
			buffer[i] = to_client[i];
			i++;
		}while(to_client[i] != '\0');
		
		// adding the client pid into the buffer
		enter_clientpid_into_buf(buffer,client_pid,i);
		
		//creating file for insert the result 
		if((fd_client = open(buffer, O_RDWR | O_CREAT | O_APPEND , 0666)) < 0){
			printf("Error while creating “to_client_xxxxxx” result file\n");
			exit(-1); 
		}
		cwd = realpath(buffer,NULL); // saving the name of the result file
		if(cwd == NULL){ // checking for error
			perror("Error while using realpath system call\n");
			exit(-1);
		}
		//writing result into the file
		sprintf(buffer, "%.1f", result); // saving in the buffer the result as a string
		write_str_into_file(buffer,fd_client); // writing the result into the file
		
		// sending signal to the client
		if((kill(client_pid,SIGUSR1)) < 0){
			printf("Error- can't send signal to the client\n");
			if(fork() == 0){ // removing “to_client_xxxxxx” result file
				execlp("rm","rm",cwd,NULL); // removing “to_client_xxxxxx” result file and check for error
				printf("Error while removing “to_client_xxxxxx” result file");
				exit(-1); 
			}
			else{
			     wait(NULL);  
			}
			exit(-1);
		}
		free(cwd); // free heap memory
		exit(1);
	}
	

}

// function that write string into file
void write_str_into_file(char*str,int fd_srv){
	
	int i = 0;
	// write the string into the file.
	while(str[i]!= '\0'){
			
		if(write(fd_srv,&str[i],1) < -1){
			printf("Error while writing to to_srv file\n");
			exit(-1);
		}
		i++;
	}
	if(write(fd_srv,"\n",1) < -1){
	        printf("Error while writing to to_srv file\n");
	        exit(-1);
	}

}
// function for SIGCHLD
void chld_exit(int sig){
	
	pid_t pid;
	int status;
	signal(SIGCHLD,chld_exit); // reset the handler
	while(1){ 
		pid = waitpid(-1,&status,WNOHANG); // free the son process from being ZOMBIE
		if(pid <= 0){
			if(errno == ECHILD){ // if there is no son process
				break;
			}
		}
		if(WIFEXITED(status)){ // if the son process worked normally
			if(WEXITSTATUS(status) == 254){// if the exit value was: -2
				if(fork() == 0){ // removing to_srv file
					execlp("rm","rm","to_srv",NULL); // removing "to_srv" file and check for error
					printf("Error while removing to_srv file");
					exit(-1); // finish the server program 
					
				}
				else{
				     wait(NULL);  
				}
			}
			if(WEXITSTATUS(status) == 253){ // if the exit value was: -3
				exit(-1); // finish the server program
			}
		}
	}
}



// function that enters the client pid into buffer
void enter_clientpid_into_buf(char *buffer,pid_t client_pid,int start_point){

	int i = 1,j = 0, temp = client_pid ,sp = start_point;
	char c;
	// checking how long is the number (unity / tens / hundreds / thousands .... )
	while(1){
		temp = temp/10;
		if(temp == 0){
			break;
		}
		j++;
	}
	while(j!=0){
		i = i*10;
		j--;
	}
	
	// inset the number to the buffer
	while( i != 0){
		c = (char)((client_pid/i)+48); // transfer the number to char
		buffer[sp] = c;
		sp++;
		client_pid = client_pid % i;
		i = i/10;
	}
	buffer[sp] = '\0'; 
	
}



