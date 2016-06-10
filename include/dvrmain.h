#ifndef _DVR_MAIN_
#define _DVR_MAIN_

#include <mysql.h>
#include <my_global.h>

#include <netsdk.h>

#define DVR_LOCK_FILE "/var/lock/dvr"
#define DVR_XML_CONF_FILE "/etc/nanotech/nanotech.conf"

#define EFAILURE -1
#define ESUCCESS 0

typedef struct dvrDbConf{
	char server[16];
	char username[255];
	char password[255];
	char database[255];
	int service_port;
}dvrDbConf;

typedef struct dvrGlobalConf {
int dvr_pid;
int dvr_sid;
dvrDbConf *dbconf;
MYSQL *sqlConn;
}dvr_g_conf;

typedef enum eventBitMap {
alert_local_alarm= 0x00000001,
alert_net_alarm= 0x00000002,
alert_manual_alarm= 0x00000004,
alert_video_motion= 0x00000008,
alert_video_loss= 0x00000010,
alert_video_blind= 0x00000020,
alert_video_title= 0x00000040,
alert_video_split= 0x00000080,
alert_video_tour= 0x00000100,
alert_storage_not_exist= 0x00000200,
alert_storage_failure= 0x00000400,
alert_low_space= 0x00000800,
alert_net_abort= 0x00001000,
alert_comm= 0x00002000,
alert_storage_read_error= 0x00004000,
alert_storage_write_error= 0x00008000,
alert_net_ipconflict= 0x00010000,
alert_alarm_emergency= 0x00020000,
alert_dec_connect= 0x00040000,
alert_videoanalyze= 0x00080000,
}eventBitMap;

typedef struct dvrClient{
	struct dvrClient *next;
	char clientName[255];
	char ipaddress[16];
	unsigned int ipaddr;
	int port;
	char userName[255];
	char password[255];
	int cameraPorts;
	int camerasConnected;
	char delEntry;
	char pingFails;
	eventBitMap alert;
	eventBitMap critical; // This is a subset of alert. This alert is put into critical table.
	int loginId;
	H264_DVR_DEVICEINFO devInfo;
	SDK_CONFIG_NET_COMMON networkCfg;

	int loginID;
	LPH264_DVR_DEVICEINFO devInfo_new;
}dvrClient;

typedef enum confType {
REFRESH_IP=1,
DOWNLOAD
}confType;

typedef struct downloadconf {
char ipaddr[16];
char date[10]; // date in dd:mm:yy
char time[10]; //time in hh:mm:ss
int duration; //download video length, default 2 mins.
}downloadconf;

typedef enum confaction {
ADD=0,
MODIFY,
DELETE
}confaction;

typedef struct new_or_modifyconf {
char ipaddr[16];
confaction action;
}new_or_modifyconf;

typedef union changeconf {
new_or_modifyconf alterconf;
downloadconf download;
}changeconf;

typedef struct dvrchangeconf {
struct dvrchangeconf *next;
confType type;
changeconf confEntry;
}dvrchangeconf;

#ifndef __conf__
#define __conf__
extern dvrClient *gdvrList;
extern dvrDbConf dbconf;
extern dvr_g_conf dvrgconf;
extern dvrClient *dvrClientList;
extern pthread_mutex_t gdvrlistmutex;
extern pthread_mutex_t gdvrlistaddmutex;
#endif //__dbconf__
dvrClient * findbyipaddress(char *ipaddress);
dvrClient * findbyloginid(long int loginid);
int readXmlConf(char *);

#endif //_DVR_MAIN_
