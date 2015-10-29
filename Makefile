obj-$(CONFIG_MAC80211) += mac80211.o

# mac80211 objects
mac80211-y := \
	main.o status.o \
	sta_info.o \
	wep.o \
	wpa.o \
	scan.o offchannel.o \
	ht.o agg-tx.o agg-rx.o \
	vht.o \
	ibss.o \
	iface.o \
	rate.o \
	michael.o \
	tkip.o \
	aes_ccm.o \
	aes_cmac.o \
	cfg.o \
	ethtool.o \
	rx.o \
	spectmgmt.o \
	tx.o \
	key.o \
	util.o \
	wme.o \
	event.o \
	chan.o \
	trace.o mlme.o \
	tdls.o \
	taputil.o \
	tappoint.o

mac80211-$(CONFIG_MAC80211_LEDS) += led.o
mac80211-$(CONFIG_MAC80211_DEBUGFS) += \
	debugfs.o \
	debugfs_sta.o \
	debugfs_netdev.o \
	debugfs_key.o

mac80211-$(CONFIG_MAC80211_MESH) += \
	mesh.o \
	mesh_pathtbl.o \
	mesh_plink.o \
	mesh_hwmp.o \
	mesh_sync.o \
	mesh_ps.o 

mac80211-$(CONFIG_PM) += pm.o

CFLAGS_trace.o := -I$(src)

rc80211_minstrel-y := rc80211_minstrel.o
rc80211_minstrel-$(CONFIG_MAC80211_DEBUGFS) += rc80211_minstrel_debugfs.o

rc80211_minstrel_ht-y := rc80211_minstrel_ht.o
rc80211_minstrel_ht-$(CONFIG_MAC80211_DEBUGFS) += rc80211_minstrel_ht_debugfs.o

mac80211-$(CONFIG_MAC80211_RC_MINSTREL) += $(rc80211_minstrel-y)
mac80211-$(CONFIG_MAC80211_RC_MINSTREL_HT) += $(rc80211_minstrel_ht-y)

ccflags-y += -D__CHECK_ENDIAN__ -DDEBUG
KVERS=$(shell uname -r)

CURDIR=$(shell pwd)

all:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
	#make -C /lib/modules/3.17.8-200.fc20.x86_64/build M=/home/chillance/hostapd/mac80211 modules
clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
	#make -C /lib/modules/3.17.8-200.fc20.x86_64/build M=/home/chillance/hostapd/mac80211 clean

uninstall:
	#-rmmod ath9k
	#-rmmod  mac80211
	
	-rmmod rtl8188ee
	-rmmod rtl_pci 
	-rmmod rtlwifi
	-rmmod  mac80211
	-rm -f /dev/cute
	#-rmmod b43
	#-rmmod brcmsmac
	#-rmmod  mac80211
install:
	insmod ./mac80211.ko
	#modprobe brcmsmac
	#modprobe b43
	#modprobe ath9k
	mknod /dev/cute c 507 0
	modprobe rtl8188ee
reset:
	make -C . uninstall
	make -C . install
start:
	./reset.sh
	hostapd /etc/hostapd/hostapd.conf

user:
	gcc -o meeeow user_space_demo.c
	rm -f ./meeeow.pcap
	./meeeow