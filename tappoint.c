
#include <net/mac80211.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include "tappoint.h"
#include "ieee80211_i.h"

int tappoint_dev_open(struct inode*inode,struct file*filp);
int tappoint_dev_release(struct inode*node,struct file*filp);
ssize_t tappoint_dev_read(struct file*filp,char __user *buf,size_t size,loff_t *ppos);
ssize_t tappoint_dev_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos);

struct tappoint_dev g_tap_dev;
#define MAJOR_NUMBER 507

dev_t devno;
int g_major_no=MAJOR_NUMBER;
int register_flag=0;
struct file_operations fops={
    .owner=THIS_MODULE,
    .open=tappoint_dev_open,
    .release=tappoint_dev_release,
    .read=tappoint_dev_read,
    .write=tappoint_dev_write
};

void xmit_80211_data_frame(struct sk_buff *skb)
{
	#if 1
	int idx;
	char *lptr;
	//printk("tx packet:%d\n",g_tap_dev.skb_tx->len);
	if(!skb->dev)/*no attached device found*/
		goto drop_flag;
	//printk("tx packet dev:%d\n",skb->dev->ifindex);
	//try to transform into 802.3 frame ,and we will invoke ieee80211_subif_start_xmit
	lptr=skb->data;
	for (idx=5;idx>=0;idx--){
		lptr[24+idx]=lptr[10+idx];
		lptr[18+idx]=lptr[16+idx];
	}
	skb_pull(skb,18);
	skb_reset_mac_header(skb);
	if(ieee80211_subif_start_xmit(skb,skb->dev))
		goto drop_flag;
	#else
	int idx;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(skb->dev);
	struct ieee80211_chanctx_conf *chanctx_conf;
	struct ieee80211_sub_if_data *ap_sdata;
	enum ieee80211_band band;
	/**/
	struct cute_80211_hdr *c8h=skb->data;
	c8h->u0=0;
	c8h->u1=1;
	for(idx=0;idx<6;idx++){
		c8h->addr3[idx]=c8h->addr2[idx];
		c8h->addr2[idx]++;
	}

	rcu_read_lock();
	ap_sdata = container_of(sdata->bss, struct ieee80211_sub_if_data,u.ap);
	chanctx_conf = rcu_dereference(ap_sdata->vif.chanctx_conf);
	if(!chanctx_conf)
		goto drop_flag;
	band = chanctx_conf->def.chan->band;
	ieee80211_xmit(sdata,skb,band);
	rcu_read_unlock();
	#endif
	
	return ;
	drop_flag:
		printk("drop:%d\n",skb->len);
		//rcu_read_unlock();
		kfree_skb(skb);
}
ssize_t tappoint_dev_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{/*here we keep a limit,that when issueed a write operation,the packet will be written at the same time.I will not handle the exception cases any more*/
/*note above this line is not true ,now*/

	int tx_size=0;
	int avail_size=size;
	char * lptr=buf;
	int real_size=0;

	
	again_flag:
		if(g_tap_dev.itx_ptr==-1&&avail_size>=4){
			__copy_from_user(&g_tap_dev.itx_ifindex,lptr,2);
			__copy_from_user(&g_tap_dev.itx_len,lptr+2,2);
			
			avail_size-=4;
			lptr+=4;
			/*allocate a sk_buff for store data*/
			g_tap_dev.skb_tx=alloc_skb(1600,GFP_KERNEL);
			if(!g_tap_dev.skb_tx)
				goto ret_flag;
			g_tap_dev.itx_ptr=0;
			/*keep these space clean*/
			skb_put(g_tap_dev.skb_tx,g_tap_dev.itx_len);
			tx_size+=4;
		}
		
		if(g_tap_dev.itx_ptr!=-1){//recv more data
			real_size=min(g_tap_dev.itx_len,avail_size);
			__copy_from_user(g_tap_dev.skb_tx->data,lptr,real_size);
			lptr+=real_size;
			avail_size-=real_size;
			g_tap_dev.itx_ptr+=real_size;
			tx_size+=real_size;
		}
		if(g_tap_dev.itx_ptr>=g_tap_dev.itx_len){//data full
			/*xmit the packet*/
			g_tap_dev.skb_tx->dev=dev_get_by_index(&init_net,g_tap_dev.itx_ifindex);
			if(g_tap_dev.skb_tx->dev)
				dev_put(g_tap_dev.skb_tx->dev);
			
			xmit_80211_data_frame(g_tap_dev.skb_tx);
			
			#if 0
			printk("tx packet size:%d\n",g_tap_dev.skb_tx->len);
			kfree_skb(g_tap_dev.skb_tx);
			#endif
			g_tap_dev.itx_ptr=-1;
			g_tap_dev.itx_len=0;
			g_tap_dev.skb_tx=NULL;
			goto again_flag;
		}
		#if 0
#endif
	ret_flag:
	return tx_size;
}

ssize_t tappoint_dev_read(struct file*filp,char __user *buf,size_t size,loff_t *ppos)
{
  	int ret=0;
	char * buff_ptr=buf;
	int avail_size=size;
	int real_size;
    DECLARE_WAITQUEUE(wait,current);
	
    if (size<4)/*not enough base room for headers*/
        return -EFAULT;
	
	#if 1
    if(g_tap_dev.idata_ptr == -1){
      /*wait until a packet's arrival*/
      /*down(&g_tap_dev.sem_guard);*/
	  spin_lock_bh(&g_tap_dev.lock_guard);
      add_wait_queue(&g_tap_dev.skb_wait,&wait);
      
      while((dequeue_skb(&g_tap_dev,1,&g_tap_dev.skb_cur))<0){
		/*give up processor resource */
		spin_unlock_bh(&g_tap_dev.lock_guard);
		/*up(&g_tap_dev.sem_guard);*/
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		/*handle some upexpected events*/
		if(signal_pending(current)){
		  ret=-ERESTARTSYS;
		  goto while_out;
		}
		/*down(&g_tap_dev.sem_guard);*/
		spin_lock_bh(&g_tap_dev.lock_guard);
      }//end of while
    
      /*up(&g_tap_dev.sem_guard);*/
	  spin_unlock_bh(&g_tap_dev.lock_guard);
	  while_out:
      remove_wait_queue(&g_tap_dev.skb_wait,&wait);
      set_current_state(TASK_RUNNING);
      if (g_tap_dev.skb_cur==NULL)
		return ret;
	  g_tap_dev.idata_len=g_tap_dev.skb_cur->len;
	  g_tap_dev.idata_ptr=0;
	  /*try to fill the headers which occupy 4 bytes space*/
	 // printk("if index:%d\n",g_tap_dev.skb_cur->dev->ifindex);
	  copy_to_user(buff_ptr,&g_tap_dev.skb_cur->dev->ifindex,2);
	  copy_to_user(buff_ptr+2,&g_tap_dev.skb_cur->len,2);
	  buff_ptr+=4;
	  avail_size-=4;
	  //*((short*)(buff_ptr))=g_tap_dev.skb_cur->dev->ifindex;
	  //*((short*)(buff_ptr+2))=g_tap_dev.skb_cur->len;
      //printk("rx data length:%d\n",g_tap_dev.skb_cur->len);
      ret+=4;
	  g_tap_dev.idata_ptr=0;
    }
	real_size=min(avail_size,g_tap_dev.idata_len-g_tap_dev.idata_ptr);
	copy_to_user(buff_ptr,g_tap_dev.skb_cur->data+g_tap_dev.idata_ptr,real_size);
	g_tap_dev.idata_ptr+=real_size;
	ret+=real_size;
	if(g_tap_dev.idata_ptr>=g_tap_dev.idata_len){
		g_tap_dev.idata_len=0;
		g_tap_dev.idata_ptr=-1;
		kfree_skb(&g_tap_dev.skb_cur);
		
		g_tap_dev.skb_cur=NULL;
		
	}
	//copy skb data into user buffer 
	#endif
    return ret;
}
int tappoint_dev_open(struct inode*inode,struct file*filp)
{
    printk("tapoint dev open\n");
    return 0;
}
int tappoint_dev_release(struct inode*node,struct file*filp)
{

    printk("tappoint dev released\n");
    return 0;
}
int enqueue_skb(struct tappoint_dev *tap_dev,int is_rx,struct sk_buff*skb)
{
	int ret=-1;
	//down(&tap_dev->sem_guard);
	if(is_rx){//enqueue rx queue
		if(tappoint_queue_enqueue(&tap_dev->rx_queue,(void*)skb) != 0)
			goto norm_proc;
	}else{
		if(tappoint_queue_enqueue(&tap_dev->tx_queue,(void*)skb) != 0)
			goto norm_proc;
	}
	ret =0;
	norm_proc:
	//up(&tap_dev->sem_guard);
	return ret;
}
int dequeue_skb(struct tappoint_dev * tap_dev,int is_rx,struct sk_buff**pskb)
{
  *pskb=NULL;
	if (is_rx){
		*pskb=tappoint_queue_dequeue(&tap_dev->rx_queue);
	}
	else {
		*pskb=tappoint_queue_dequeue(&tap_dev->tx_queue);
	}
	return *pskb?0:-1;
}

int tappoint_init(void)
{

	/*create char device here*/
	int ret;
	devno=MKDEV(g_major_no,0);
	ret=register_chrdev_region(devno,1,"tappoint");
	if (ret<0)
		return ret;
	cdev_init(&g_tap_dev.cdev,&fops);
	ret=cdev_add(&g_tap_dev.cdev,devno,1);
	if(ret<0){
		unregister_chrdev_region(devno,1);
		return ret;
	}
	register_flag=1;
	
	tappoint_queue_init(&g_tap_dev.rx_queue);
	tappoint_queue_init(&g_tap_dev.tx_queue);
	/*sema_init(&g_tap_dev.sem_guard,1);*/
	spin_lock_init(&g_tap_dev.lock_guard);
	g_tap_dev.idata_ptr=-1;
	g_tap_dev.idata_len=0;
	g_tap_dev.skb_cur=NULL;
	g_tap_dev.skb_tx=NULL;
	g_tap_dev.itx_len=0;
	g_tap_dev.itx_ptr=-1;
	init_waitqueue_head(&g_tap_dev.skb_wait);
	#if 0
	tappoint_queue_enqueue(&g_tap_dev.rx_queue,NULL);
	tappoint_queue_enqueue(&g_tap_dev.rx_queue,NULL);
	
	printk("is empty:%d\n",tappoint_queue_is_full(&g_tap_dev.rx_queue));
	#endif
	
	return 0;
}
void tappoint_uninit(void)
{
	if(register_flag){
		cdev_del(&g_tap_dev.cdev);
		unregister_chrdev_region(devno,1);
	}
}
