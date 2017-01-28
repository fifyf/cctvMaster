#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dvrmain.h"

dvrClient *gdvrList;
MYSQL *gconn;

int initDb()
{
gconn = mysql_init(NULL);
printf("%s:%d\n", __FUNCTION__, __LINE__);
printf("%s, %s, %s, %s, %d\n", dbconf.server, dbconf.username, dbconf.password, dbconf.database, dbconf.service_port);
if (mysql_real_connect(gconn, dbconf.server,
			dbconf.username, dbconf.password, dbconf.database, 0, NULL, 0) == NULL) {
printf("%s:%d %s\n", __FUNCTION__, __LINE__, mysql_error(gconn));
	fprintf(stderr, "%s\n", mysql_error(gconn));
	return EFAILURE;
 }
printf("%s:%d\n", __FUNCTION__, __LINE__);
return ESUCCESS;
}

int readDB_updateConf (char *ipaddress)
{
MYSQL_RES *res;
MYSQL_ROW row;
int mysqlStatus;
int rows=0;
char specific_query[512];
dvrClient *telt=NULL;
int foundClient=0;
int atoi_=0;

char *basic_query="select clientName,ipaddress,ipaddr,port,userName,password,local_alarm,net_alarm,manual_alarm,video_motion,video_loss,video_blind,video_title,video_split,video_tour,storage_not_exist,storage_failure,low_space,net_abort,comm,storage_read_error,storage_write_error,net_ipconflict,alarm_emergency,dec_connect,videoanalyze,camera_ports,camera_connected from configuration";

if(ipaddress) {
	memset(specific_query,0,512);
	snprintf(specific_query, 511, "%s where ipaddress='%s'", basic_query, ipaddress);
	mysqlStatus = mysql_query(gconn, specific_query);
	telt = findbyipaddress(ipaddress);
	if(telt != NULL) {
		telt->delEntry=1;
		telt=NULL;
	}
} else {
	mysqlStatus = mysql_query(gconn, basic_query);
}
if(mysqlStatus == 0) {
	res = mysql_store_result(gconn);
} else {
	return EFAILURE;
}
printf("%s:%d\n", __FUNCTION__, __LINE__);
if(res) {
	int field_count;
	while((row = mysql_fetch_row(res))) {
		if(telt==NULL) {
			telt=(dvrClient *)malloc(sizeof(dvrClient));
			memset(telt, 0, sizeof(dvrClient));
		}
		field_count=0;
		while(field_count < mysql_field_count(gconn)) {
		switch(field_count) {
		case 0:
			if(row[field_count]) {
			strncpy(telt->clientName, row[field_count],strlen(row[field_count]));
			}
			break;
		case 1:
			if(row[field_count]) {
			strncpy(telt->ipaddress, row[field_count],strlen(row[field_count]));
			}
			break;
		case 2:
			if(row[field_count]) {
			telt->ipaddr = atoi( row[field_count]);
			if(telt->ipaddr == 0) {
				inet_aton(telt->ipaddress, (struct in_addr *)&telt->ipaddr);
			}
			}
			break;
		case 3:
			if(row[field_count]) {
			atoi_ = atoi( row[field_count]);
			telt->port=atoi_;
			atoi_=0;
			}
			break;
		case 4:
			if(row[field_count]) {
			strncpy(telt->userName, row[field_count],strlen(row[field_count]));
			}
			break;
		case 5:
			if(row[field_count]) {
			strncpy(telt->password, row[field_count],strlen(row[field_count]));
			}
			break;
		case 6:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_local_alarm;
				break;
			case '2':
				telt->alert |= alert_local_alarm;
				telt->critical |= alert_local_alarm;
				break;
			}
			}
			break;
		case 7:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_net_alarm;
				break;
			case '2':
				telt->alert |= alert_net_alarm;
				telt->critical |= alert_net_alarm;
				break;
			}
			}
			break;
		case 8:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_manual_alarm;
				break;
			case '2':
				telt->alert |= alert_manual_alarm;
				telt->critical |= alert_manual_alarm;
				break;
			}
			}
			break;
		case 9:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_video_motion;
				break;
			case '2':
				telt->alert |= alert_video_motion;
				telt->critical |= alert_video_motion;
				break;
			}
			}
			break;
		case 10:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_video_loss;
				break;
			case '2':
				telt->alert |= alert_video_loss;
				telt->critical |= alert_video_loss;
				break;
			}
			}
			break;
		case 11:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_video_blind;
				break;
			case '2':
				telt->alert |= alert_video_blind;
				telt->critical |= alert_video_blind;
				break;
			}
			}
			break;
		case 12:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_video_title;
				break;
			case '2':
				telt->alert |= alert_video_title;
				telt->critical |= alert_video_title;
				break;
			}
			}
			break;
		case 13:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_video_split;
				break;
			case '2':
				telt->alert |= alert_video_split;
				telt->critical |= alert_video_split;
				break;
			}
			}
			break;
		case 14:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_video_tour;
				break;
			case '2':
				telt->alert |= alert_video_tour;
				telt->critical |= alert_video_tour;
				break;
			}
			}
			break;
		case 15:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_storage_not_exist;
				break;
			case '2':
				telt->alert |= alert_storage_not_exist;
				telt->critical |= alert_storage_not_exist;
				break;
			}
			}
			break;
		case 16:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_storage_failure;
				break;
			case '2':
				telt->alert |= alert_storage_failure;
				telt->critical |= alert_storage_failure;
				break;
			}
			}
			break;
		case 17:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_low_space;
				break;
			case '2':
				telt->alert |= alert_low_space;
				telt->critical |= alert_low_space;
				break;
			}
			}
			break;
		case 18:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_net_abort;
				break;
			case '2':
				telt->alert |= alert_net_abort;
				telt->critical |= alert_net_abort;
				break;
			}
			}
			break;
		case 19:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_comm;
				break;
			case '2':
				telt->alert |= alert_comm;
				telt->critical |= alert_comm;
				break;
			}
			}
			break;
		case 20:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_storage_read_error;
				break;
			case '2':
				telt->alert |= alert_storage_read_error;
				telt->critical |= alert_storage_read_error;
				break;
			}
			}
			break;
		case 21:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_storage_write_error;
				break;
			case '2':
				telt->alert |= alert_storage_write_error;
				telt->critical |= alert_storage_write_error;
				break;
			}
			}
			break;
		case 22:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_net_ipconflict;
				break;
			case '2':
				telt->alert |= alert_net_ipconflict;
				telt->critical |= alert_net_ipconflict;
				break;
			}
			}
			break;
		case 23:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_alarm_emergency;
				break;
			case '2':
				telt->alert |= alert_alarm_emergency;
				telt->critical |= alert_alarm_emergency;
				break;
			}
			}
			break;
		case 24:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_dec_connect;
				break;
			case '2':
				telt->alert |= alert_dec_connect;
				telt->critical |= alert_dec_connect;
				break;
			}
			}
			break;
		case 25:
			if(row[field_count]) {
			switch(row[field_count][0]) {
			case '1':
				telt->alert |= alert_videoanalyze;
				break;
			case '2':
				telt->alert |= alert_videoanalyze;
				telt->critical |= alert_videoanalyze;
				break;
			}
			}
			break;
		case 26:
			if(row[field_count]) {
			telt->cameraPorts=atoi(row[field_count]);
			}
			break;
		case 27:
			if(row[field_count]) {
			telt->camerasConnected=atoi(row[field_count]);
			}
			break;
		}
		field_count++;
		}
		if(ipaddress && foundClient) {
			foundClient=0;
		} else {
			pthread_mutex_lock(&gdvrlistaddmutex);
			if(gdvrList == NULL) {
				telt->next = gdvrList;
				gdvrList=telt;
				pthread_mutex_unlock(&gdvrlistmutex);
			} else {
				telt->next = gdvrList;
				gdvrList=telt;
			}
			pthread_mutex_unlock(&gdvrlistaddmutex);
		}
		telt=NULL;
	}
}
printf("%s:%d\n", __FUNCTION__, __LINE__);
return ESUCCESS;
}

int readChangeConf(dvrchangeconf **confHead)
{
int retval=0;
dvrchangeconf *telt=NULL;
MYSQL_RES *res;
MYSQL_ROW row;
int mysqlStatus;
int rowstart=-1;
int rowend=-1;
char *basic_query="select itr, commandName,attribute1,attribute2,attribute3,attribute4,attribute5,attribute6,attribute7 from changeconf";

mysqlStatus = mysql_query(gconn, basic_query);
if(mysqlStatus == 0) {
	res = mysql_store_result(gconn);
} else {
	return EFAILURE;
}
if(res) {
	int field_count;
	while((row = mysql_fetch_row(res))) {
			if(rowstart == -1)
				rowstart=atoi(row[0]);
			if(telt==NULL) {
				telt=(dvrchangeconf *)malloc(sizeof(dvrchangeconf));
				memset(telt, 0, sizeof(dvrchangeconf));
			}
			if(strncmp(row[1], "REFRESH_IP", strlen("REFRESH_IP")) == 0) {
				field_count=0;
				while(field_count < mysql_field_count(gconn)) {
				switch(field_count) {
				case 0:
					rowend=atoi(row[field_count]);
					break;
				case 1:
					telt->type=REFRESH_IP;
					break;
				case 2:
					strncpy(telt->confEntry.alterconf.ipaddr, row[field_count], strlen(row[field_count]));
					break;
				case 3:
					telt->confEntry.alterconf.action = atoi(row[field_count]);
					break;
				default:
					break;
				}
				field_count++;
				}
			} else if(strncmp(row[1], "DOWNLOAD", strlen("DOWNLOAD")) == 0) {
				field_count=0;
				while(field_count < mysql_field_count(gconn)) {
				switch(field_count) {
				case 0:
					rowend=atoi(row[field_count]);
					break;
				case 1:
					telt->type=DOWNLOAD;
					break;
				case 2:
					strncpy(telt->confEntry.download.ipaddr, row[field_count], strlen(row[field_count]));
					break;
				case 3:
					strncpy(telt->confEntry.download.date, row[field_count], strlen(row[field_count]));
					break;
				case 4:
					strncpy(telt->confEntry.download.time, row[field_count], strlen(row[field_count]));
					break;
				default:
					break;
				}
				field_count++;
				}
			} else if(strncmp(row[1], "HDD_MONITOR", strlen("HDD_MONITOR"))) {
				while(field_count < mysql_field_count(gconn)) {
				switch(field_count) {
				case 0:
					rowend=atoi(row[field_count]);
					break;
				case 1:
					telt->type=HDDINFO;
					break;
				case 2:
					strncpy(telt->confEntry.hddinfoconf.ipaddr, row[field_count], strlen(row[field_count]));
					break;
				case 3:
					telt->confEntry.hddinfoconf.enable = atoi(row[field_count]);
					break;
				default:
					break;
				}
				field_count++;
				}
			} else if(strncmp(row[1], "UPDATE_TIME", strlen("UPDATE_TIME"))) {
				while(field_count < mysql_field_count(gconn)) {
				switch(field_count) {
				case 0:
					rowend=atoi(row[field_count]);
					break;
				case 1:
					telt->type=UPDATE_TIME;
					break;
				case 2:
					strncpy(telt->confEntry.timeconf.ipaddr, row[field_count], strlen(row[field_count]));
					break;
				case 3:
					telt->confEntry.timeconf.isSystemTime = atoi(row[field_count]);
					break;
				case 4:
					telt->confEntry.timeconf.dd = atoi(row[field_count]);
					break;
				case 5:
					telt->confEntry.timeconf.mm = atoi(row[field_count]);
					break;
				case 6:
					telt->confEntry.timeconf.yy = atoi(row[field_count]);
					break;
				case 7:
					telt->confEntry.timeconf.dd = atoi(row[field_count]);
					break;
				case 8:
					telt->confEntry.timeconf.min = atoi(row[field_count]);
					break;
				case 9:
					telt->confEntry.timeconf.sec = atoi(row[field_count]);
					break;
				default:
					break;
				}
				field_count++;
				}
			}
		telt->next=*confHead;
		*confHead=telt;
		telt=NULL;
		retval++;
		}
	}
char delete_query[512];
memset(delete_query, 0, 512);
snprintf(delete_query, 512, "delete from changeconf where itr >= %d and itr <=%d", rowstart, rowend);
mysqlStatus = mysql_query(gconn, delete_query);
return retval;
}

int deleteAllEntries(char *table)
{
int mysqlStatus;
char completeEntry[256];
memset(completeEntry, 0, 256);

snprintf(completeEntry, 256, "delete * from %s", table);
	if((mysqlStatus = mysql_query(gconn, completeEntry)) != 0) {
		return -1;
	} else {
		return 0;
	}
}

int insertEntry(char *table, char *entry)
{
int mysqlStatus;
int ret;
char completeEntry[1024];
memset(completeEntry, 0, 1024);

snprintf(completeEntry, 1024, "insert into %s %s", table, entry);
if((mysqlStatus = mysql_query(gconn, completeEntry)) != 0) {
return -1;
} else {
ret = mysql_insert_id(gconn);
return ret;
}
}
