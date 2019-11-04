#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

typedef enum { false, true } bool;
int parseInput (char* input, char* command[]);
void execute (char* command[], bool background);
int cd (char* path);
void signalHandler (int signal);
void appendToFile ();
void prepareFile ();

char buffer[256];


int main()
{
    char userInput[128];
    char* command[128];
    bool background = false;

    getcwd(buffer, sizeof(buffer)); // Get current working directory and save it to buffer variable (Helpful in the log file)

    signal(SIGCHLD, signalHandler); // Wait for SIHCHLD signal and when it occurs call signalHandler()
    prepareFile(); // Overwrite the old session and start a new one

    while (true) {
        printf("Sherif> ");
        fgets(userInput, sizeof(userInput) , stdin); // Get user input
        int lengthOfInput = strlen(userInput);

        if (userInput[lengthOfInput - 1] == '\n')
            userInput[lengthOfInput - 1] = '\0'; // Remove new line

        int index = parseInput(userInput, command); // Split the input on spaces and store them in an array of pointers
        if (index == 0) continue; // Handle empty input

        if (strcasecmp(command[0], "exit") == 0) // Check if exit
            exit(99);

        if (strcasecmp(command[0], "cd") == 0) { // Check for cd
            int value = cd(command[1]);
            if (value < 0)
                printf("cd: %s: No such file or directory\n", command[1]);
            continue;
        }

        if (strcasecmp(command[index - 1] , "&") == 0){ // Check if we need to wait for the child process to finish execution
            background = true;
            command[index - 1] = NULL;
        }

        execute(command, background); // Execute the command given by the user
        background = false;
    }


    return 0;
}

int parseInput (char* input, char* command[]) {
    char* parsed; // Currently parsed word
    char* separator = " "; // The splitting separator
    int index = 0; // Index of the array of pointers to character

    parsed = strtok(input, separator); // Split the input string on spaces and save them in the array of pointer to characters
    while (parsed != NULL) {
        command[index] = parsed;
        index++;
        parsed = strtok(NULL, separator);
    }

    command[index] = NULL; // execvp requires a NULL terminated array of pointers
    return index;
}

void execute (char* command[], bool background) {
    pid_t childId = fork(); // Create a new process
    int status;

    if (childId < 0){ // Fork failed
        printf("Fork Failed Exiting ...");
        exit(-1);
    } else if (childId == 0) { // In child process
        int hasError = execvp(command[0], command); // Execute the command, If hasError is set to -1 then the command is wrong
        if (hasError == -1) { // Not necessary to check because if execvp was successfully executed, code would change in the process image
            printf("I won't reach this print if execvp executed succesfully (Wrong command)\n");
            kill(getpid(), SIGKILL); // This will kill the process if the entered command is wrong
        }
    } else { // In parent process
        if (background == false){
            /*do {*/
            waitpid(childId, &status, WUNTRACED); // Wait for the child to terminate if the background is false
            /*} while (!WIFEXITED(status) && !WIFSIGNALED(status));*/
        }
    }
}

int cd (char* path) {
    return chdir(path); // Return Value: This command returns zero (0) on success. -1 is returned on an error.
}


void signalHandler (int signal) {
    appendToFile(); // If the parent receives a SIGCHLD signal indicating that his child is terminated, Add to the logs file
}


void appendToFile () {
    FILE* filePointer;
    filePointer = fopen(buffer, "a"); // Open the file (appending)

    if (filePointer == NULL) {
        printf("Unable to open file");
        exit(-1);
    }

    fprintf(filePointer, "A child process was terminated\n");

    fclose(filePointer);
}

void prepareFile () {
    FILE* filePointer;
    filePointer = fopen(strcat(buffer, "/log.txt"), "w"); // Add the the buffer of the current working directory the log.txt (create the file)

    if (filePointer == NULL) {
        printf("Unable to create file");
        exit(-1);
    }

    fprintf(filePointer , "STARTING A NEW SESSION\n");

    fclose(filePointer);
}

