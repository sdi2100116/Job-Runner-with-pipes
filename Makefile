# compile with gcc
CC=gcc

# Compiler options:
#

CXXFLAGS = -g


# set the name of the executable file to compile here
PROGRAM1 = jobCommander
PROGRAM2 = jobExecutorServer
PROGRAM3 = progDelay


OBJS1 = JobCommander/JobCommander.o
OBJS2 = JobExecutorServer/JobExecutorServer.o QueueImplementation/QueueImplementation.o
OBJS3 = progDelay.o


$(PROGRAM1): $(OBJS1)
	$(CC) $(CXXLAGS) $(OBJS1) -o $(PROGRAM1)

$(PROGRAM2): $(OBJS2)
	$(CC) $(CXXLAGS) $(OBJS2)  -o $(PROGRAM2)

$(PROGRAM3): $(OBJS3)
	$(CC) $(CXXLAGS) $(OBJS3) -o $(PROGRAM3)

file:
	rm jobExecutorServer.txt
clean:
	rm -f $(OBJS1) $(OBJS2)  $(OBJS3) 
	rm progDelay jobCommander jobExecutorServer