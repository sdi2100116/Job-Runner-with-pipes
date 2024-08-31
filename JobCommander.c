#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO1 "/tmp/sdi2100116.1"
#define FIFO2 "/tmp/sdi2100116.2"

int main(int argc, char *argv[])
{

    int readfd, writefd;
    int sfd;
    char *buff = NULL;
    int pid;
    int character;
    int exstatus;

    char *input;
    char *command = argv[1];

    int inp_size = 0;
    for (int i = 1; i < argc; i++)
    {
        inp_size += strlen(argv[i]) + 1;
    }
    buff = malloc(inp_size + 1); // +1 for the null terminator
    if (buff == NULL)
    {
        perror("malloc");
        exit(1);
    }
    buff[0] = '\0';

    // concatenate all arguments into a  string
    for (int i = 1; i < argc; i++)
    {
        strcat(buff, argv[i]);
        if (i != argc - 1)
        {
            strcat(buff, " ");
        }
    }
    int n = strlen(buff);
    // check if input is correct,if not exit with 1
    if (!strcmp(command, "issueJob"))
    {
        if (argc < 3)
        {
            printf("Wrong Usage\n");
            exit(1);
        }
    }
    else if (!strcmp(command, "setConcurrency"))
    {
        if (argc != 3)
        {
            printf("Wrong Usage\n");
            exit(1);
        }
    }
    else if (!strcmp(command, "stop"))
    {
        if (argc != 3)
        {
            printf("Wrong Usage\n");
            exit(1);
        }
    }
    else if (!strcmp(command, "poll"))
    {
        if (argc != 3)
        {
            printf("Wrong Usage\n");
            exit(1);
        }
        char *status = argv[2];
        if (strcmp(status, "running") && strcmp(status, "queued"))
        {
            printf("Invalid status: %s\n", status);
            exit(1);
        }
    }
    else if (!strcmp(command, "exit"))
    {
        if (argc != 2)
        {
            printf("Wrong Usage\n");
            exit(1);
        }
    }
    else
    {
        printf("Unknown command.Exiting...\n");
        exit(1);
    }
    // Pipe creation
    if ((mkfifo(FIFO1, 0666) < 0) && (errno != EEXIST))
    {
        perror("mkfifo");
        exit(1);
    }
    if ((mkfifo(FIFO2, 0666) < 0) && (errno != EEXIST))
    {
        perror("mkfifo");
        exit(1);
    }
    // tries to open the txt file returns -1 if it fails
    if ((sfd = open("jobExecutorServer.txt", O_RDONLY)) == -1)
    {                 // the file does not exist
        pid = fork(); // creates the server

        if (pid == -1)
        {
            perror("fork");
            exit(1);
        }

        if (pid == 0)
        {
            // child process, generates server
            exstatus = execl("jobExecutorServer", "./jobExecutorServer", NULL);
            if (exstatus == -1)
            {
                perror("execl");
                exit(1);
            }
        }
        else
        { // parent
            if ((writefd = open(FIFO1, O_WRONLY)) < 0)
            {
                perror("client: can't open write fifo \n");
            }
            if ((readfd = open(FIFO2, O_RDONLY)) < 0)
            {
                perror("client: can't open read fifo \n");
            }
            // first pass the number of the bytes,so the server knows how many to read

            if (write(writefd, &n, sizeof(n)) < 0)
            {
                perror("write");
                exit(1);
            }
            // then write the input into the pipe
            if (write(writefd, buff, n) != n)
            {
                perror("write");
                exit(1);
            }
        }
    }
    else
    {
        if ((writefd = open(FIFO1, O_WRONLY)) < 0)
        {
            perror("client: can't open write fifo \n");
        }
        if ((readfd = open(FIFO2, O_RDONLY)) < 0)
        {
            perror("client: can't open read fifo \n");
        }

        if (write(writefd, &n, sizeof(n)) < 0)
        {
            perror("write");
            exit(1);
        }
        if (write(writefd, buff, n) != n)
        {
            perror("write");
            exit(1);
        }

        // reads the pid of the server from the JobExecutorServer.txt and then sends a signal that the commander wrote
        // in the pipe,so the server will wake from the pause()
        char serverPID_string[15];
        read(sfd, serverPID_string, 15); // error handling
        int server_PID;
        server_PID = atoi(serverPID_string);
        if (kill(server_PID, SIGUSR1) == -1)
        {
            perror("kill");
            exit(1);
        }
    }

    // then reads from the pipe that the server wrote into
    int msize;
    read(readfd, &msize, sizeof(msize));

    char response[msize];
    int length = read(readfd, response, msize);
    if (length != 1) // length is 1 when the jobCommander input is setConcurrency so it wont print anything
    {                // outputs the buff on the screen
        if (write(1, response, msize) == -1)
        {
            perror("write failed");
            exit(1);
        };
    }

    close(writefd);
    close(readfd);
    free(buff);
    exit(0);
}
