// This program is the client part
// The program gets input - pid of server, operand_1, operation and operand_2
// The client is creating "to_srv" file, insert to the file the pid of the client, operand_1, operation and operand_2
// Then the program is sending a signal to the server, and waiting for a signal back.
// The program is getting a signal when the server finish the calculation, and print the result to the screen.

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <signal.h>
#include <errno.h>

int try = 0; // global integer that counts how many times we tried to connect the server
void write_str_into_file(char*str,int fd_srv); // function that writes string into a file
void enter_mypid_into_buf(char *buffer,pid_t my_pid,int start_point); // function that enters pid into buffer
void sig1(int sig); // function for SIGUSR1 signal
void alarm_hand(int sig); // function for alarm
void alarm_finish(int sig); // function for finish process if the server didn't response

int main(int argc, char* argv[]){

	int fd_srv; // file descriptor for "to_srv" file
	int random; // integer for random number
	char buffer[50]; // buffer to store strings
	pid_t pid_server; // pid for server pid
	// change the signals handler
	signal(SIGUSR1, sig1);
	signal(SIGALRM,alarm_hand);
	
	if(argc < 5){ // checking if we got enougth arguments
		printf("Not enough argument \n");
		exit(-1);
	}
	pid_server = atoi(argv[1]); // setting server pid
	// checking if the PID is llegal
	if((kill(pid_server,0)) < 0){
		printf("Error - illegal PID \n");
		exit(-1);
	}
	// trying to connect the server
	while(try < 10){
		// create "to_srv" file
		fd_srv = open("to_srv", O_WRONLY | O_CREAT | O_EXCL , 0666);
		if(fd_srv < 0){ 
			if(errno == EEXIST){ // check if the file already exist
				// creating random number between 1 to 5
				random = (rand() % 5) + 1;
				// setting alarm for random number and sleep
				alarm(random);
				sleep(random);
			}
			else{ // if the error is because of somthing else
				printf("Error - can't create to_srv file\n");
				exit(-1);
			}
		}
		else{
			break; // if we got here then create the file and we can continue
		}
	
	}
	// if we tried to connect 10 times
	if(try == 10){
		printf("Cant connect to the server \n");
		exit(-1);
	}
	
	// if we succeeded to connect the server
	sprintf(buffer, "%d", getpid()); // saving in the buffer the pid as a string
	write_str_into_file(buffer,fd_srv); // writing our pid into to_srv file
	write_str_into_file(argv[2],fd_srv); // writing first operand into to_srv file
	write_str_into_file(argv[3],fd_srv); // writing the operation into to_srv file
	write_str_into_file(argv[4],fd_srv); // writing the second operand into to_srv file
	
	// sending signal to the server
	if((kill(pid_server,SIGUSR1)) < 0){
		printf("Error while sending signal to the server \n"); 
		if(fork() == 0){ // removing to_srv file
			execlp("rm","rm","to_srv",NULL); // removing "to_srv" file and check for error
			printf("Error while removing to_srv file");
			exit(-1); // finish the server program 
		}
		else{
		     wait(NULL);  
		}
		exit(-1);
	}
	
	// changing the alarm function to alarm_finish
	signal(SIGALRM,alarm_finish); // change the signal handler
	alarm(15); // alarm for 15 second for the server
	pause(); // waiting for signal from the server
	
	exit(1);
	
}

// function that writes string into file
void write_str_into_file(char*str,int fd_srv){
	
	int i = 0;
	// writing the string into the file
	while(str[i]!= '\0'){	
		if(write(fd_srv,&str[i],1) < -1){ // writing the char and check if the writing didn't success
			printf("Error while writing to to_srv file\n");
			if(fork() == 0){ // removing to_srv file
				execlp("rm","rm","to_srv",NULL);  // removing "to_srv" file 
				printf("Error while removing to_srv file"); // if error eccured
				exit(-1); // finish the client program 	
			}
			else{ // waiting until we finish remove
			     wait(NULL);  
			}
			exit(-1);
		}
		i++;
	}
	if(write(fd_srv,"\n",1) < -1){ // writing "\n" to the file and check for error
	        printf("Error while writing to to_srv file\n");
	        if(fork() == 0){ // removing to_srv file
				execlp("rm","rm","to_srv",NULL); // removing "to_srv" file 
				printf("Error while removing to_srv file"); // if error eccured
				exit(-1); // finish the client program 
			}
		else{ // waiting until we finish remove
		     wait(NULL);  
		}
		exit(-1);
	}

}


// function that enters pid into buffer
void enter_mypid_into_buf(char *buffer,pid_t my_pid,int start_point){

	int i = 1,j = 0, temp = my_pid ,sp = start_point;
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
		c = (char)((my_pid/i)+48); // transfer the number to char
		buffer[sp] = c; // insert the char into the buffer
		sp++;
		my_pid = my_pid % i;
		i = i/10;
	}
	buffer[sp] = '\0'; 
}
 

// function for SIGUSR1 signal
void sig1(int sig){
	char buffer[50];
	char to_client[] = "to_client_"; 
	int i = 0;
	int fd_to_client;
	char buffer_for_print;
	signal(SIGUSR1, sig1); // reset the signal handler
	// changing buffer to be “to_client_xxxxxx” (xxxxxx will be the client pid)
	do{ // changing the buffer to be: "to_client_"
		buffer[i] = to_client[i];
		i++;
	}while(to_client[i] != '\0');
	enter_mypid_into_buf(buffer,getpid(),i); // adding the pid into the buffer
	
	// open “to_client_xxxxxx” file and check for errors
	if((fd_to_client = open(buffer, O_RDONLY , 0666)) < 0){
		printf("Error while openning to_client_xxxxxx file\n");
		exit(-1);
	}	
	
	i = 1;
	// printing the content of the file into the screen
	while(i!=0){ // as long as we have not reached the end of the file
		i = read(fd_to_client,&buffer_for_print,1); // reading char from “to_client_xxxxxx”  file
		if(i<0){ // if we got error while reading 
			printf("Error while reading to_client_xxxxxx file\n");
			if(fork() == 0){ // removing to_client_xxxxxx file
	 	        	execlp("rm","rm",buffer,NULL); 
				printf("Error while removing to_client_xxxxxx file");
				exit(-1); // finish the client program 
			}
			else{
			     wait(NULL);  
			}
			exit(-1);
		}
		if(i > 0){ // printing the content of the file 
			printf("%c",buffer_for_print);
		}
	}
	
	close(fd_to_client);
	
	if(fork() == 0){ // removing to_client_xxxxxx file
 	       execlp("rm","rm",buffer,NULL); 
		printf("Error while removing to_client_xxxxxx file");
		exit(-1); // finish the client program 
	}
	else{
	     wait(NULL);  
	}
	exit(1);
	
}

// function for ALARM - count how many times we try to connect the server
void alarm_hand(int sig){
	signal(SIGALRM,alarm_hand); // reset the signal handler
	try = try + 1;	
}	
// function for the ALARM - finish process if the server didn't response after 15 sec
void alarm_finish(int sig){
	exit(-1);
}




