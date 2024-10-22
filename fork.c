// gcc -o fork fork.c
// gcc -o fork fork.c; gcc -o daemon daemon.c; gcc -o echo_input echo_input.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int response_listener(int main_pipe[]) {
	char buffer[1024];
	int bytes_read;
	
	while (1) {
		// Attempt to read the message from the pipe
		bytes_read = read(main_pipe[0], buffer, sizeof(buffer) -1);
		if (bytes_read > 0) {
			// null-terminate the buffer
			buffer[bytes_read] = '\0';
			
			// Split the buffer by newline characters
			char *line = strtok(buffer, "\n");
			while (line != NULL) {
				if (strcmp(line, "checks=pass") == 0) {
					return 1;
				} else if (strcmp(line, "checks=fail") == 0) {
					return 0;
				}
				
				// Get the next line
				line = strtok(NULL, "\n");
			}
		}
		sleep(1);
	}
	
	return 0;
}

/*
execute_command():

Execute a command and create a pipe associated with that command so that message handling can occur.
Contents of executed command will be printed out to screen.
Special strings for memory attachment will be suppressed from screen.
Function returns the process id of the command executed.
*/
//pid_t execute_command(char *command, char *const argv[], int main_pipe[]) {
pid_t execute_command(char *command, char *argv[], int main_pipe[]) {

	pid_t main_pid;
	
	// create pipe for parent-child communication
	if (pipe(main_pipe) == -1) {
		perror("pipe failed");
		exit(EXIT_FAILURE);
	}
	
	// create a fork for the main process you are executing
	main_pid = fork();
	if (main_pid == -1) {
		perror("main fork failed");
		return -1;
	}

	// child process
	if (main_pid == 0) {
		
		close(main_pipe[0]); // close read-end of the pipe

		int stdout_pipe[2];
		if (pipe(stdout_pipe) == -1) {
			perror("stdout pipe failed");
			exit(EXIT_FAILURE);
		}

		pid_t tee_pid = fork();
		if (tee_pid == -1) {
			perror("tee fork failed");
			exit(EXIT_FAILURE);
		}
		
		if (tee_pid == 0) {
			
			dup2(stdout_pipe[0], STDIN_FILENO);
			close(stdout_pipe[0]);
			close(stdout_pipe[1]);

			// set up arguments
			// use the tee command to output all lines accept "memattach=pass" and "memattach=fail" to the screen
			char *tee_argv[] = {"sh", "-c", "tee /dev/fd/3 | grep -Ev 'checks=pass|checks=fail'", NULL};
			//char *tee_argv[] = {"tee", "/dev/fd/3", NULL};  // this line will print EVERYTHING
			
			// redirect stdout to the pipe (write end)
			if (dup2(main_pipe[1], 3) == -1) {
				perror("tee dup2 failed");
				exit(EXIT_FAILURE);
			}
		
			execvp(tee_argv[0], tee_argv);
			perror("tee execvp failed");
			exit(EXIT_FAILURE);
		
		} else {
			
			close(stdout_pipe[0]);
			dup2(stdout_pipe[1], STDOUT_FILENO);
			close(stdout_pipe[1]);
			
			execvp(command, argv); // execute command
			perror("main execvp failed"); // if execvp returns, an error occured
			exit(EXIT_FAILURE);
			
		}
	}
	
	// parent process
	close(main_pipe[1]);
	return main_pid; // return child pid
}

/*
execute_command_with_wait():

Execute a command and wait for a response of either 1 or 0.
If 1 is returned then assume successful and return process id.
If 0 is returned then assume a failure took place and re-attempt execution based on the wait time used.
Executed command must include a "pass" or "fail" printf statement to pass listener.
*/
int execute_command_with_wait(char *command, char *argv[], int waittime_ms) {

	int main_pipe[2];
	pid_t cmd_pid;
	int passed = 0;

	while (!passed) {
		cmd_pid = execute_command(command, argv, main_pipe);
		
		if (cmd_pid == -1) {
			perror("Failed to start process");
			return -1;
		}

		passed = response_listener(main_pipe);

		if (!passed) {
			printf("Received 'checks=fail', restarting the command...\n");
      fflush(stdout);
			usleep(waittime_ms);
		}
	}

	return cmd_pid;
}


int main(int argc, char **argv) {

	for (int i = 0; i < 2; i++) {
		char *daemon_argv[4];
		char instance[10];
		sprintf(instance, "%d", i);

		daemon_argv[0] = "./daemon";
		daemon_argv[1] = "true";
		//daemon_argv[1] = "false";
		daemon_argv[2] = instance;
		daemon_argv[3] = NULL;

		pid_t cmd_pid = execute_command_with_wait(daemon_argv[0], daemon_argv, 1000000);
		printf("checks passed to start daemon[%d]. pid=%d\n", i, cmd_pid);
	}

	while (1) {
		sleep(1);
	}
	
	return 1;
}