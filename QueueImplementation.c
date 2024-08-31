#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "QueueInterface.h"

struct Qnode
{
    struct triplet data;
    struct Qnode *next;
};

struct Q
{
    int size;
    struct Qnode *head;
    struct Qnode *tail;
};

struct Q *queue_initialize()
{
    struct Q *queue = malloc(sizeof(struct Q));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

int queue_insert(struct Q *q, struct triplet tr)
{
    struct Qnode *new = malloc(sizeof(struct Qnode));
    new->data = tr;
    new->next = NULL;
    if (q->head == NULL)
    {
        new->data.queuePosition = 1;
        q->head = new;
        q->tail = q->head;
    }
    else
    {

        new->data.queuePosition = q->tail->data.queuePosition + 1;
        q->tail->next = new;
        q->tail = q->tail->next;
    }
    q->size++;
    return new->data.queuePosition;
}

struct triplet remove_specific(struct Q *q, char *jobID)
{
    struct Qnode *temp = q->head;
    struct triplet ret_data;
    struct Qnode *start_ch;
    struct Qnode *prev=NULL;
    if(temp==NULL){
        ret_data.job=strdup("NULL");
    }
    while (temp != NULL)
    {
        if (strcmp(temp->data.jobID, jobID) == 0)
        {
            if (temp == q->head) {
                ret_data = temp->data;
                q->head = temp->next;
                if (q->head == NULL) {
                    q->tail = NULL;
                }
                start_ch=q->head;
            }
            else{
                ret_data=temp->data;
                prev->next=temp->next;
                start_ch=prev->next;
                if(temp->next==NULL){
                    q->tail=prev;
                }
                
            }
           
            
            free(temp);
            break;
        }
        else
        {
            prev=temp;
            temp = temp->next;
        }
    }

    // changing all the queue positions
    while (start_ch != NULL)
    {
        start_ch->data.queuePosition--;
        start_ch = start_ch->next;
    }
    q->size--;
    return ret_data;
}

struct triplet queue_remove(struct Q *q)
{
    struct triplet ret_data = q->head->data;
    struct Qnode *temp = q->head;
    if(temp==NULL){
        ret_data.job=strdup("NULL");
        return ret_data;
    }
    q->head = q->head->next;
    free(temp);
    temp=q->head;
    // changing all the queue positions
    while (temp != NULL)
    {
        temp->data.queuePosition--;
        temp = temp->next;
    }
    q->size--;
    return ret_data;
}

struct triplet remove_specific_w_pid(struct Q *q,int pid){
    struct Qnode* temp=q->head;
    while(temp!=NULL){
        if(temp->data.chid==pid){
            return remove_specific(q,temp->data.jobID);
        }
        else{
            temp=temp->next;
        }
    }

}

int get_size(struct Q *q)
{
    return q->size;
}

void ret_queue_data(struct Q *q, struct triplet *ret)
{
    struct Qnode *temp = q->head;
    struct triplet last;
    last.queuePosition = 0;
    int i;
    int n = get_size(q) + 1;
    if(temp==NULL){
        return;
    }
    for (i = 0; i < get_size(q); i++) //putting all the triplets inside the array
    {
        ret[i] = temp->data;
        temp = temp->next;
    }
    ret[i] = last;
}

int is_inside(struct Q *q, char *jobID)
{
    struct Qnode *temp = q->head;
    while (temp != NULL)
    {
        if (strcmp(temp->data.jobID, jobID) == 0)
        {
            return 1;
        }
        else
        {
            temp = temp->next;
        }
    }
    return 0;
}

void queue_delete(struct Q* queue){
    struct Qnode* temp=queue->head;
    while(temp!=NULL){
        free(temp->data.job);
        free(temp);
        temp=temp->next;
    }
}