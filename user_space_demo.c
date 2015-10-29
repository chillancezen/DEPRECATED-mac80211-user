
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <pcap.h>

struct meeeow{
	int  u1;
	int u2;
	int u3;
	int u4;
};
#pragma pack(1)
struct  cute_80211_hdr{
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
};

int main()
{
	char buffer[4096];
	struct pcap_file_header pfh;
	struct meeeow meeeow;
	struct cute_80211_hdr *c8h;
	int rc;
	int idx;
	int len=0;
	int fd=open("/dev/cute",O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	int wfd=open("./hello.pcap",O_RDWR|O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(fd>=0);
	assert(fd>=0);
	pfh.magic=0xa1b2c3d4;
	pfh.version_major=0x2;
	pfh.version_minor=0x4;
	pfh.thiszone=0;
	pfh.sigfigs=0;
	pfh.snaplen=0xffff;
	pfh.linktype=105;
	write(wfd,&pfh,sizeof(pfh));
	
	while(1){
		len=rc=read(fd,buffer,sizeof(buffer),0);
		printf("read:%d %d\n",*(short*)(buffer),*(short*)(buffer+2));
		meeeow.u1=0;
		meeeow.u2=0;
		meeeow.u3=meeeow.u4=*(short*)(buffer+2);
		//pp.len=pp.caplen=
		//

		
		rc=write(wfd,&meeeow,sizeof(meeeow));//write to pcap files
		write(wfd,buffer+4,meeeow.u3);
		//change dst-mac and src-mac ,write 80211 frame back 
		c8h=buffer+4;
		for (idx=0;idx<6;idx++){
			c8h->addr3[idx]=c8h->addr2[idx];
			c8h->addr2[idx]=0x48+idx;
		}
		write(fd,buffer,len);
	}
	close(fd);
	close(wfd);
    return 0;
}
