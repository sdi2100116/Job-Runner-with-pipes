

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../QueueImplementation/QueueInterface.h"

#define FIFO1 "/tmp/sdi2100116.1"
#define FIFO2 "/tmp/sdi2100116.2"

extern int errno;
// function that gets a string and returns it without the first word
char *string_rem_first(char *st)
{
    char *first_space = strchr(st, ' ');
    char *remaining_string = first_space + 1;
    char *res = strdup(remaining_string);

    return res;
}

int childend = 0;

void sigusr1_handler(int signum) // sigusr1 handler : does nothing ,just the server to unpause
{
}

void child_handler(int signum)
{

    childend = 1;
}

int main()
{
    struct Q *queue_waiting;
    struct Q *queue_running;
    queue_waiting = queue_initialize();
    queue_running = queue_initialize();
    // Text file creation
    int jobID = 0;
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    struct sigaction sa1;
    sa1.sa_handler = child_handler;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;

    if (sigaction(SIGCHLD, &sa1, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    int ex = 1;
    char *command;
    int Concurrency = 1;
    int charged = 0;
    int con_changed = 0;
    int fd = -1;
    int alrem = 0;
    int writefd = -1, readfd = -1;
    char *buff;
    int n;
    char pid_to_str[15];
    fd = creat("jobExecutorServer.txt", 0666);
    // turning the server pid into a string and putting it into the JobExecutorServer.txt
    pid_t pid = getpid();
    snprintf(pid_to_str, sizeof(pid_to_str), "%d", pid);
    strcat(pid_to_str, "\0");
    write(fd, pid_to_str, strlen(pid_to_str));
    if (fd < 0)
    {
        perror("creat");
        exit(1);
    }
    close(fd);

    // Pipe opening for writing/reading
    if ((readfd = open(FIFO1, O_RDONLY)) < 0)
    {
        perror("open read fifo");
        exit(1);
    }
    if ((writefd = open(FIFO2, O_WRONLY)) < 0)
    {
        perror("open write fifo");
        exit(1);
    }

    while (1) // ends when reads exit from the pipe
    {
        if (!childend)
        {
            if (read(readfd, &n, sizeof(n)) < 0)
            { // n gets the size of the message that is going into the pipe
                perror("read");
                exit(1);
            }

            buff = malloc(sizeof(char) * n);
            if (buff == NULL)
            {
                perror("malloc");
                exit(1);
            }
            int length = read(readfd, buff, n);
            if (length == -1)
            {
                perror("Error reading from file");
                close(readfd);
                exit(1);
            }
            buff[length] = '\0';

            char *unchanged = strdup(buff); // used in issueJob
            unchanged[length] = '\0';

            command = strtok(buff, " ");
            if (strcmp(command, "issueJob") == 0)
            {

                // job triplet making and putting it into the queue
                struct triplet tr;
                jobID++;
                char *nofirstword = malloc(sizeof(char) * strlen(unchanged));
                if (nofirstword == NULL)
                {
                    perror("malloc");
                    exit(1);
                }
                nofirstword[0] = '\0';

                nofirstword = string_rem_first(unchanged); // so we can free unchanged later

                tr.job = malloc(sizeof(char) * strlen(nofirstword));
                if (tr.job == NULL)
                {
                    perror("malloc");
                    exit(1);
                }
                tr.job[0] = '\0';

                strcpy(tr.job, nofirstword);
                char jobIDst[10] = "job_";
                char to_string[10];
                sprintf(to_string, "%d", jobID);
                strcat(jobIDst, to_string); // creates final JobID by combining job_ and the number converted to a string
                strcpy(tr.jobID, jobIDst);
                int qp = queue_insert(queue_waiting, tr);

                // run jobs
                // first we have to charge
                if (!charged) // this happens only the first time that jobs are getting added
                {

                    if (get_size(queue_waiting) >= Concurrency)
                    {
                        charged = 1;
                        for (int i = 0; i < Concurrency; i++)
                        {

                            struct triplet tr = queue_remove(queue_waiting);
                            char **jargs = malloc(sizeof(char) * strlen(tr.job)); // maximum arguments
                            if (jargs == NULL)
                            {
                                perror("malloc");
                                exit(1);
                            }
                            int j = 0;
                            int getpid = fork();

                            if (getpid == 0) // if child token the string into an argument array and execute it
                            {

                                char *ret;

                                ret = strtok(tr.job, " ");
                                while (ret != NULL)
                                {
                                    jargs[j] = strdup(ret);

                                    j++;
                                    ret = strtok(NULL, " ");
                                }
                                jargs[j] = NULL;
                                execvp(jargs[0], jargs);
                            }
                            else // if parent add the child pid in the triplet and add it to the running jobs queue
                            {
                                tr.chid = getpid;
                                queue_insert(queue_running, tr);
                                for (int i = 0; i < j; i++)
                                {
                                    free(jargs[i]);
                                }
                                free(jargs);
                            }
                        }
                    }
                }
                else // every time a new job is added
                {
                    if (Concurrency > get_size(queue_running) && get_size(queue_waiting) > 0)
                    {
                        for (int i = 0; i < Concurrency - get_size(queue_running); i++)
                        {
                            struct triplet tr = queue_remove(queue_waiting);
                            char **jargs = malloc(sizeof(char) * strlen(tr.job)); // maximum arguments
                            if (jargs == NULL)
                            {
                                perror("malloc");
                                exit(1);
                            }
                            int j = 0;
                            int getpid = fork();

                            if (getpid == 0)
                            {

                                char *ret;

                                ret = strtok(tr.job, " ");
                                while (ret != NULL)
                                {
                                    jargs[j] = strdup(ret);

                                    j++;
                                    ret = strtok(NULL, " ");
                                }
                                jargs[j] = NULL;
                                execvp(jargs[0], jargs);
                            }
                            else
                            {

                                tr.chid = getpid;
                                queue_insert(queue_running, tr);
                                for (int i = 0; i < j; i++)
                                {
                                    free(jargs[i]);
                                }
                                free(jargs);
                            }
                        }
                    }
                }
                // prints the inserted job like this <jobID,job,queuePosition>\n, queuePosition is returned by the queue_insert function
                sprintf(to_string, "%d", qp);
                char *retmes = malloc(sizeof(char) * (strlen(unchanged) + strlen(jobIDst) + strlen(to_string) + 4));

                retmes[0] = '\0';
                strcat(retmes, "<");
                strcat(retmes, jobIDst);
                strcat(retmes, ",");
                strcat(retmes, nofirstword);
                strcat(retmes, ",");
                strcat(retmes, to_string);
                strcat(retmes, ">");
                strcat(retmes, "\n");
                int retmessize = strlen(retmes);
                if (write(writefd, &retmessize, sizeof(retmessize)) < 0)
                {
                    perror("write");
                    exit(1);
                }

                if (write(writefd, retmes, strlen(retmes)) != strlen(retmes))
                {
                    perror("write");
                    exit(1);
                }
                free(unchanged);
            }

            else if (strcmp(command, "stop") == 0)
            {
                char *job = strtok(NULL, " "); // takes the JobID
                struct triplet tr;
                char *retmes;
                if (is_inside(queue_waiting, job)) // waiting
                {

                    tr = remove_specific(queue_waiting, job);
                    free(tr.job);
                    retmes = strdup(tr.jobID);
                    strcat(retmes, " removed.\n");
                }
                else if (is_inside(queue_running, job)) // running
                {
                    alrem = 1; // changes to 1 so it doesnt get removed again when the childend turns 1
                    tr = remove_specific(queue_running, job);
                    free(tr.job);
                    kill(tr.chid, SIGKILL);
                    retmes = strdup(tr.jobID);
                    strcat(retmes, " terminated.\n");
                }
                else
                {
                    retmes = strdup("Job does not exist.\n");
                }
                int retmessize = strlen(retmes);
                if (write(writefd, &retmessize, sizeof(retmessize)) < 0)
                {
                    perror("write");
                    exit(1);
                }
                if (write(writefd, retmes, strlen(retmes)) != strlen(retmes))
                {
                    perror("write");
                    exit(1);
                }
                free(retmes);
            }

            else if (strcmp(command, "poll") == 0)
            {

                char *command2 = strtok(NULL, " "); // takes either queued or running
                char *retmes;
                int total = 0;

                if (strcmp(command2, "queued") == 0)
                {
                    if (get_size(queue_waiting) > 0)
                    {
                        int n = get_size(queue_waiting) + 1;
                        struct triplet arrtr[n];
                        ret_queue_data(queue_waiting, arrtr); // arrtr is an array with all the triplets of queue_waiting
                        int i = 0;
                        char *ret_arr[n - 1];
                        // inside ret_arr all triplets are reformed in this look: <jobID,job,queuePosition>\n
                        while (arrtr[i].queuePosition != 0)
                        {
                            ret_arr[i] = malloc(sizeof(char) * (12 + strlen(arrtr[i].job)));
                            if (ret_arr[i] == NULL)
                            {
                                perror("malloc");
                                exit(1);
                            }
                            ret_arr[i][0] = '\0';
                            strcat(ret_arr[i], "<");
                            strcat(ret_arr[i], arrtr[i].jobID);
                            strcat(ret_arr[i], ",");
                            strcat(ret_arr[i], arrtr[i].job);
                            strcat(ret_arr[i], ",");
                            char to_string[10];

                            sprintf(to_string, "%d", arrtr[i].queuePosition);

                            strcat(ret_arr[i], to_string);
                            strcat(ret_arr[i], ">");
                            strcat(ret_arr[i], "\n");
                            total += strlen(ret_arr[i]);
                            i++;
                        }
                        // everything from ret_arr is going inside retmes to get into the pipe
                        retmes = malloc(sizeof(char) * total);
                        if (retmes == NULL)
                        {
                            perror("malloc");
                            exit(1);
                        }
                        retmes[0] = '\0';
                        for (i = 0; i < n - 1; i++)
                        {
                            strcat(retmes, ret_arr[i]);
                        }
                    }
                    else // no triplets
                    {
                        retmes = strdup("No job waiting in line.\n");
                    }
                }
                else if (strcmp(command2, "running") == 0) // same as before but we are working on queue_running now
                {
                    if (get_size(queue_running) > 0)
                    {
                        int n = get_size(queue_running) + 1;
                        struct triplet arrtr[n];

                        ret_queue_data(queue_running, arrtr);
                        int i = 0;
                        char *ret_arr[n - 1];

                        while (arrtr[i].queuePosition != 0)
                        {
                            ret_arr[i] = malloc(sizeof(char) * (12 + strlen(arrtr[i].job)));
                            if (ret_arr[i] == NULL)
                            {
                                perror("malloc");
                                exit(1);
                            }
                            ret_arr[i][0] = '\0';
                            strcat(ret_arr[i], "<");
                            strcat(ret_arr[i], arrtr[i].jobID);
                            strcat(ret_arr[i], ",");
                            strcat(ret_arr[i], arrtr[i].job);
                            strcat(ret_arr[i], ",");
                            char to_string[10];

                            sprintf(to_string, "%d", arrtr[i].queuePosition);

                            strcat(ret_arr[i], to_string);
                            strcat(ret_arr[i], ">");
                            strcat(ret_arr[i], "\n");
                            total += strlen(ret_arr[i]);
                            i++;
                        }
                        retmes = malloc(sizeof(char) * total);
                        retmes[0] = '\0';
                        for (i = 0; i < n - 1; i++)
                        {
                            strcat(retmes, ret_arr[i]);
                        }
                    }
                    else // no triplets
                    {
                        retmes = strdup("No jobs running.\n");
                    }
                }
                int retmessize = strlen(retmes);
                if (write(writefd, &retmessize, sizeof(retmessize)) < 0)
                {
                    perror("write");
                    exit(1);
                }
                if (write(writefd, retmes, strlen(retmes)) != strlen(retmes))
                {
                    perror("write");
                    exit(1);
                }
                free(retmes);
            }
            else if (strcmp(command, "setConcurrency") == 0)
            {
                char *concurrency_string = strtok(NULL, "\n");
                // if the new conurrency is bigger we want jobs that are waiting to start running
                if (Concurrency < atoi(concurrency_string))
                {
                    int qrun_size = get_size(queue_running);
                    if (atoi(concurrency_string) > qrun_size && get_size(queue_waiting) > 0)
                    {

                        for (int i = 0; i < atoi(concurrency_string) - qrun_size; i++)
                        {
                            struct triplet tr = queue_remove(queue_waiting);
                            char **jargs = malloc(sizeof(char) * strlen(tr.job)); // maximum arguments
                            if (jargs == NULL)
                            {
                                perror("malloc");
                                exit(1);
                            }
                            int j = 0;
                            int getpid = fork();

                            if (getpid == 0)
                            {

                                char *ret;

                                ret = strtok(tr.job, " ");
                                while (ret != NULL)
                                {
                                    jargs[j] = strdup(ret);

                                    j++;
                                    ret = strtok(NULL, " ");
                                }
                                jargs[j] = NULL;
                                execvp(jargs[0], jargs);
                            }
                            else
                            {
                                tr.chid = getpid;
                                queue_insert(queue_running, tr);
                                for (int i = 0; i < j; i++)
                                {
                                    free(jargs[i]);
                                }
                                free(jargs);
                            }
                        }
                    }
                }
                Concurrency = atoi(concurrency_string); // changes the concurrency

                char *retmes = " ";
                int retmessize = strlen(retmes);
                if (write(writefd, &retmessize, sizeof(retmessize)) < 0)
                {
                    perror("write");
                    exit(1);
                }
                if (write(writefd, retmes, strlen(retmes)) != strlen(retmes))
                {
                    perror("write");
                    exit(1);
                }
            }
            else if (strcmp(command, "exit") == 0)
            {

                char *retmes = "JobExecutorServer Terminated.\n";
                int retmessize = strlen(retmes);
                if (write(writefd, &retmessize, sizeof(retmessize)) < 0)
                {
                    perror("write");
                    exit(1);
                }
                if (write(writefd, retmes, strlen(retmes)) != strlen(retmes))
                {
                    perror("write");
                    exit(1);
                }

                break; // terminates
            }
            else
            {
                printf("error");
            }
        }
        else // a child process ends means that a job ended,we have to add another job in the running queue
        {
            int status;
            int endpid = waitpid(-1, &status, WNOHANG);

            if (!alrem)
            { // case where the job is already removed from the stop command
                struct triplet tr = remove_specific_w_pid(queue_running, endpid);
                free(tr.job);
                alrem = 0;
            }
            if (get_size(queue_waiting) >= 1 && get_size(queue_running) < Concurrency)
            {
                struct triplet tr = queue_remove(queue_waiting);
                char **jargs = malloc(sizeof(char) * strlen(tr.job)); // maximum arguments
                if (jargs == NULL)
                {
                    perror("malloc");
                    exit(1);
                }
                int j = 0;
                int getpid = fork();

                if (getpid == 0)
                {

                    char *ret;

                    ret = strtok(tr.job, " ");
                    while (ret != NULL)
                    {
                        jargs[j] = strdup(ret);

                        j++;
                        ret = strtok(NULL, " ");
                    }
                    jargs[j] = NULL;
                    execvp(jargs[0], jargs);
                }
                else
                {

                    tr.chid = getpid;
                    queue_insert(queue_running, tr);
                    for (int i = 0; i < j; i++)
                    {
                        free(jargs[i]);
                    }
                    free(jargs);
                }
            }

            childend = 0;
        }

        pause();
    }

    close(writefd);
    close(readfd);
    queue_delete(queue_running);
    queue_delete(queue_waiting);
    // free(command);
    free(buff);
    if (remove("jobExecutorServer.txt") == -1)
    {
        perror("remove");
    }
    return 0;
}
