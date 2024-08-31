struct triplet
{
    char jobID[6];
    char *job;
    int queuePosition;
    int chid;
};
struct Q * queue_initialize();
int queue_insert(struct Q *q, struct triplet tr);
struct triplet queue_remove(struct Q* q);
struct triplet remove_specific(struct Q* q,char* jobID);
struct triplet remove_specific_w_pid(struct Q *q,int pid);
int get_size(struct Q* q);
void ret_queue_data(struct Q *q,struct triplet* ret);
int is_inside(struct Q* q,char* jobID);
void queue_delete(struct Q* queue);