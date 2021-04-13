# Calculator
Linux System Programming in C: Development a Calculator Program.

This is a client and server system based on signal communication, multiprocessing, and information transmission via files. 

How to run the system:
1. Run the server as a job (background).
2. Run the client program. The Client program revieces 4 parameters: 
   * P1 : The server PID.
   * P2 : The first numeric value on which the calculation operation must be performed.
   * P3 : The calculation operation: 1-addition, 2-subtraction, 3-multiplication, 4-division.
   * P4 : The second numeric value.
3. As the client receiving the answer from the server software, the client will print the answer on the screen and stop working.
4. To stop the server, move back the server to the foreground and send him a signal (for example: SIGINT by using ctrl + c).


