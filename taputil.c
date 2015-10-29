#include "tappoint.h"

void tappoint_queue_init(struct tappoint_queue * que)
{
	que->front=0;
	que->rear=0;
}

/*negative value indicate failure*/
int tappoint_queue_enqueue(struct tappoint_queue*que,void *ele)
{
	if (tappoint_queue_is_full(que))
		return -1;
	que->queue_buff[que->rear]=ele;
	que->rear=(que->rear+1)%MAX_QUEUE_SIZE;
	return 0;
}
void * tappoint_queue_dequeue(struct tappoint_queue*que)
{
	void * ele=NULL;
	if(tappoint_queue_is_empty(que))
		return NULL;
	ele=que->queue_buff[que->front];
	que->front=(que->front+1)%MAX_QUEUE_SIZE;
	return ele;
}
