#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#define MAX_QUEUE_SIZE 1000
#undef NULL
#define NULL ((void *)0)

struct tappoint_queue{
	void * queue_buff[MAX_QUEUE_SIZE];
	int rear;
	int front;
};
struct tappoint_dev{
        struct cdev cdev;
        struct tappoint_queue rx_queue;
        struct tappoint_queue tx_queue;
		spinlock_t lock_guard;
        //struct semaphore  sem_guard;
        int idata_ptr;
        int idata_len;
  struct sk_buff * skb_cur;
  wait_queue_head_t skb_wait;


  struct sk_buff * skb_tx;
  int itx_ifindex;
  int itx_len;
  int itx_ptr;
};

struct cute_80211_hdr{
	unsigned char proto:2;
	unsigned char type:2;
	unsigned char sub_type:4;
	unsigned char u0:1;
	unsigned char u1:1;
	unsigned char u2:1;
	unsigned char u3:1;
	unsigned char u4:1;
	unsigned char u5:1;
	unsigned char u6:1;
	unsigned char u7:1;
	unsigned short dur_id;
	unsigned char addr1[6];
	unsigned char addr2[6];
	unsigned char addr3[6];
}__packed __aligned(1);

void tappoint_queue_init(struct tappoint_queue * que);

int tappoint_queue_enqueue(struct tappoint_queue*que,void *ele);
void * tappoint_queue_dequeue(struct tappoint_queue*que);

#define tappoint_queue_is_empty(que) ((que)->front==(que)->rear)
#define tappoint_queue_is_full(que) ((que)->front==(((que)->rear+1)%MAX_QUEUE_SIZE))

int tappoint_init(void);
void tappoint_uninit(void);
int enqueue_skb(struct tappoint_dev *tap_dev,int is_rx,struct sk_buff*skb);
int dequeue_skb(struct tappoint_dev * tap_dev,int is_rx,struct sk_buff**pskb);



