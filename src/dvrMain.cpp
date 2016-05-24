#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dvrmain.h"
#include "dvrSql.h"
#include "netsdk.h"

#define ALERT_FIELDS "(n_date, n_time, ch_id, message) values"
#define CRITICAL_FIELDS "(n_date, n_time, ch_id, message, client_name, alert_id) values"

char *critical_table="critical";

dvr_g_conf dvrgconf;
int g_stopPing;

typedef struct holdAlarm {
struct holdAlarm *next;
long int loginid;
int buflen;
char *buf;
}holdAlarm;

pthread_mutex_t alarmmutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gdvrlistmutex=PTHREAD_MUTEX_INITIALIZER;

typedef struct alarmList {
holdAlarm *head;
holdAlarm *tail;
}alarmList;

alarmList alarmList_;

void AddToTail(alarmList *list, holdAlarm *node) {
pthread_mutex_lock(&alarmmutex);
if(list->head == NULL) {
list->head = list->tail = node;
} else {
list->tail->next=node;
list->tail=node;
}
pthread_mutex_unlock(&alarmmutex);
}

void * fetchFromHead(alarmList *list) {
holdAlarm *node=NULL;
pthread_mutex_lock(&alarmmutex);
if (list->head != NULL) {
	node=list->head;
	list->head = node->next;
	if(node->next == NULL)
		list->tail = NULL;
}
pthread_mutex_unlock(&alarmmutex);
return (void *)node;
}

int daemonize()
{
int fd;
int read_fd;
FILE *pidfile=NULL;

if ((dvrgconf.dvr_sid=setsid()) < 0) {
	printf("daemonize failed to set sid\n");
	return EFAILURE;
}

if((fd = access(DVR_LOCK_FILE, F_OK|W_OK)) == ESUCCESS) {
	if ( EFAILURE!= (read_fd = open ( DVR_LOCK_FILE , O_RDONLY )) )
	{
	   char buffer[64] = {0};
	   struct stat file_info = {0};
	   int length = 0;
                /* Get information about the file.  */
       	   pid_t dvr_pid = 0;
       	   fstat (read_fd, &file_info);
           length = file_info.st_size;
           read ( read_fd, buffer, length );
       	   dvr_pid = (pid_t)atoi(buffer);
           if ( dvr_pid == 0 ) {
		printf ("Cant read the dvr pid.  Restarting cifs daemon");
		close(read_fd);
		unlink(DVR_LOCK_FILE);
           }
          close(read_fd);
          if ( kill(dvr_pid, 0) == 0)  {
                printf("\nDVR daemon is already running\n");
		return EFAILURE;
          }
   	  else {
		printf ("\nIgnoring the existing dvr lock file. Restarting dvr daemon\n");
		unlink (DVR_LOCK_FILE);
	  }
	}
}

	if (NULL ==(pidfile = fopen(DVR_LOCK_FILE, "w"))) {
	    	printf( "Cannot open file %s", DVR_LOCK_FILE );
	    	return EFAILURE;
	}
	char pidstring[255];
	sprintf(pidstring, "%u", getpid());
	if ( EOF == fputs(pidstring, pidfile) ) {
		printf("PID is not written to file %s", DVR_LOCK_FILE);
	}
	fclose(pidfile);
	if ( 0!= chmod(DVR_LOCK_FILE, S_IRUSR | S_IWUSR )) {
		printf("chmod failed for CIFS pid");
	}

	if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
          (void)dup2(fd, STDIN_FILENO);
          (void)dup2(fd, STDOUT_FILENO);
          (void)dup2(fd, STDERR_FILENO);
          if (fd > STDERR_FILENO)
            (void)close(fd);
        } else {
          printf("Could not redirect stdout, stderr, stdin - proceeding nontheless\n");
        }

return ESUCCESS;
}

void * ping_routine(void *)
{
char *ping="/bin/ping";
char pingString[255];
dvrClient *tmpdvr=NULL;
dvrClient *prevdvr=NULL;
int repeatCounter=0;
char flag;
int ret=0;
while (!g_stopPing) {
	flag=0;
	if(gdvrList == NULL)
		pthread_mutex_lock(&gdvrlistmutex);
		
	if(tmpdvr==NULL) {
		prevdvr=tmpdvr;
		tmpdvr=gdvrList;
	} else {
		prevdvr=tmpdvr;
		if(tmpdvr->next)
			tmpdvr=tmpdvr->next;
		else
			tmpdvr=gdvrList;
	}
	if (tmpdvr->delEntry) {
		if(tmpdvr == gdvrList) {
			gdvrList=tmpdvr->next;
			free(tmpdvr);
		} else {
			prevdvr->next=tmpdvr->next;
			free(tmpdvr);
		}
	}
	memset(pingString,0,255);
	snprintf(pingString, 255, "%s %s %s >> /dev/null", ping, "-w 3", tmpdvr->ipaddress);
	do {
		if(repeatCounter)
			repeatCounter--;
		ret=system(pingString);
		if(ret == 0)
			tmpdvr->pingFails=0;
		if((ret != 0) && (flag == 0)) {
			//Repeat ping test 3 times.
			repeatCounter=2;
			flag=1;
		}
		if((repeatCounter == 0) && (flag == 1)) {
			tmpdvr->pingFails++;;
			if(tmpdvr->pingFails == 1) {
			// Fill sql table.
			time_t t=time(NULL);
			struct tm tt=*localtime(&t);
			char str[256];
			char str1[512];
			memset(str, 0, 255);
			memset(str1, 0, 511);
			snprintf(str, 255, "%s ('%d-%d-%d', '%d:%d:%d', -1, 'Network unreachable')", ALERT_FIELDS, tt.tm_mday, 
					tt.tm_mon + 1, tt.tm_year + 1900, tt.tm_hour, tt.tm_min, tt.tm_sec);
			printf("insert into %s %s\n", tmpdvr->clientName, str);
			long ret=insertEntry(tmpdvr->clientName, str);
			snprintf(str1, 255, "%s ('%d-%d-%d', '%d:%d:%d', -1, 'Network unreachable', '%s', '%d')", CRITICAL_FIELDS, tt.tm_mday, 
					tt.tm_mon + 1, tt.tm_year + 1900, tt.tm_hour, tt.tm_min, tt.tm_sec, tmpdvr->clientName, ret);
			printf("insert into %s %s\n", critical_table, str1);
			insertEntry(critical_table, str1);
			}
		}
	} while(repeatCounter);
	tmpdvr=tmpdvr->next;
}

}

void *processAlarm(void *)
{
holdAlarm *node;
dvrClient *who=NULL;
char insertrow_critical[512];
char insertrow_general[512];
int rowno=0;
while (1) {
	if((node = fetchFromHead(&alarmList_)) != NULL) {
		if((who = findbyloginid(node->loginid)) == NULL) {
			//Add to critical;
			free(node->buf);
			free(node);
		} else {
			char time[15];
			char date[15];
			SDK_ALARM_INFO *info = (SDK_ALARM_INFO *)node->buf;
			snprintf(time, 15, "%d:%d:%d", info->SysTime.hour,info->SysTime.minute,info->SysTime.second);
			snprintf(date, 15, "%d-%d-%d", info->SysTime.day, info->SysTime.month,info->SysTime.year);
			switch(info->iEvent) {
        case SDK_EVENT_CODE_LOCAL_ALARM:
		if(who->critical | alert_local_alarm) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_LOCAL_ALARM");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_LOCAL_ALARM", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_local_alarm) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_LOCAL_ALARM");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_NET_ALARM:
		if(who->critical | alert_net_alarm) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_ALARM");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_ALARM", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_net_alarm) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_ALARM");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_MANUAL_ALARM:
		if(who->critical | alert_manual_alarm){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_MANUAL_ALARM");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_MANUAL_ALARM", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_manual_alarm){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_MANUAL_ALARM");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VIDEO_MOTION:
		if(who->critical | alert_video_motion){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_MOTION");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_MOTION", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_video_motion){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_MOTION");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VIDEO_LOSS:
		if(who->critical | alert_video_loss) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_LOSS");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_LOSS", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_video_loss) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_LOSS");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VIDEO_BLIND:
		if(who->critical | alert_video_blind) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_BLIND");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_BLIND", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_video_blind) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_BLIND");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VIDEO_TITLE:
		if(who->critical | alert_video_title) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_TITLE");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_TITLE", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_video_title) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_TITLE");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VIDEO_SPLIT:
		if(who->critical | alert_video_split) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_SPLIT");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_SPLIT", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_video_split) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_SPLIT");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VIDEO_TOUR:
		if(who->critical | alert_video_tour) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_TOUR");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_TOUR", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_video_tour) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VIDEO_TOUR");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_STORAGE_NOT_EXIST:
		if(who->critical | alert_storage_not_exist) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_NOT_EXIST");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_NOT_EXIST", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_storage_not_exist) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_NOT_EXIST");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_STORAGE_FAILURE:
		if(who->critical | alert_storage_failure) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_FAILURE");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_FAILURE", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_storage_failure) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_FAILURE");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_LOW_SPACE:
		if(who->critical | alert_low_space) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_LOW_SPACE");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_LOW_SPACE", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_low_space) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_LOW_SPACE");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_NET_ABORT:
		if(who->critical | alert_net_abort) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_ABORT");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_ABORT", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_net_abort) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_ABORT");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_COMM:
		if(who->critical | alert_comm) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_COMM");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_COMM", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_comm) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_COMM");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_STORAGE_READ_ERROR:
		if(who->critical | alert_storage_read_error) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_READ_ERROR");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_READ_ERROR", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_storage_read_error) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_READ_ERROR");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_STORAGE_WRITE_ERROR:
		if(who->critical | alert_storage_write_error) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_WRITE_ERROR");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_WRITE_ERROR", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_storage_write_error){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_STORAGE_WRITE_ERROR");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_NET_IPCONFLICT:
		if(who->critical | alert_net_ipconflict){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_IPCONFLICT");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_IPCONFLICT", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_net_ipconflict){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_NET_IPCONFLICT");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_ALARM_EMERGENCY:
		if(who->critical | alert_alarm_emergency){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_ALARM_EMERGENCY");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_ALARM_EMERGENCY", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_alarm_emergency){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_ALARM_EMERGENCY");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_DEC_CONNECT:
		if(who->critical | alert_dec_connect) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_DEC_CONNECT");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_DEC_CONNECT", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_dec_connect) {
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_DEC_CONNECT");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
        case SDK_EVENT_CODE_VideoAnalyze:
		if(who->critical | alert_videoanalyze){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s')", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VideoAnalyze");
			rowno = insertEntry(who->clientName, insertrow_general);
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s', '%s', '%d')", CRITICAL_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VideoAnalyze", who->clientName, rowno);
			insertEntry(critical_table, insertrow_general);
		}
		else if(who->alert | alert_videoanalyze){
			snprintf(insertrow_general, 511, "%s ('%s','%s','%d','%s'", ALERT_FIELDS, date, time, info->nChannel, "SDK_EVENT_CODE_VideoAnalyze");
			insertEntry(who->clientName, insertrow_general);
		}
		break;
			}
			free(node->buf);
			free(node);
		}
	} else 
		sleep(5);
}
}

void * pollconf(void *)
{
int confCount=0;
dvrchangeconf *confHead = NULL;
dvrchangeconf *nodetofree = NULL;
while (1) {
	if((confCount=readChangeConf(&confHead)) != 0) {
printf("%s:%d\n", __FUNCTION__, __LINE__);
		while(confCount > 0) {
printf("%s:%d\n", __FUNCTION__, __LINE__);
			if (confHead->type == REFRESH_IP) {
printf("%s:%d ipaddress : %s\n", __FUNCTION__, __LINE__,confHead->confEntry.alterconf.ipaddr);
				// Case for locking
				readDB_updateConf(confHead->confEntry.alterconf.ipaddr);
			} else if (confHead->type == DOWNLOAD) {
				//Not implemented.
			}
			nodetofree = confHead;
			confHead=confHead->next;
			free(nodetofree);
			nodetofree = NULL;
			confCount--;
		}
	}
	sleep(10);
}
}

bool alarmCallBack(long int loginid, char *pBuf, unsigned long dwBufLen, long dwuser)
{
SDK_ALARM_INFO *info=(SDK_ALARM_INFO *)pBuf;
holdAlarm *halarm=calloc(sizeof(holdAlarm),1);
halarm->buf = calloc(dwBufLen,1);
halarm->loginid=loginid;
halarm->buflen=dwBufLen;
memcpy(halarm->buf, pBuf, dwBufLen);
AddToTail(&alarmList_, halarm);
return true;
}

bool startListner()
{
printf("%s:%d\n", __FUNCTION__, __LINE__);
H264_DVR_Init(0,0);
H264_DVR_StartAlarmCenterListen(dbconf.service_port, alarmCallBack, 0);
printf("%s:%d t=%d, f=%d\n", __FUNCTION__, __LINE__, true, false);
return true;
}

void initClient(dvrClient *clnt)
{
int error=0;
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if(clnt->loginId) {
	// Being done to cleanup the existing client.
		H264_DVR_CloseAlarmChan(clnt->loginId);
		H264_DVR_Logout(clnt->loginId);
	}
printf("%s:%d clnt->ipaddress : %s, clnt->port : %d, clnt->userName : %s, clnt->password : %s\n", __FUNCTION__, __LINE__, clnt->ipaddress, clnt->port, clnt->userName, clnt->password);

	clnt->loginId=H264_DVR_LoginEx((char *)clnt->ipaddress, (int)clnt->port, (char *)clnt->userName, (char *)""/*clnt->password*/, &clnt->devInfo, 6, &error);
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (clnt->loginId <= 0) {
printf("%s:%d Login Id : %d\n", __FUNCTION__, __LINE__, clnt->loginId);
			time_t t=time(NULL);
			struct tm tt=*localtime(&t);
			char str[256];
			char str1[512];
			snprintf(str, 255, "%s ('%d-%d-%d', '%d:%d:%d', -1, 'Unable to login')", ALERT_FIELDS, tt.tm_mday, 
					tt.tm_mon + 1, tt.tm_year + 1900, tt.tm_hour, tt.tm_min, tt.tm_sec);
			long ret=insertEntry(clnt->clientName, str);
			snprintf(str1, 255, "%s ('%d-%d-%d', '%d:%d:%d', -1, 'Unable to login', %s, '%d')", CRITICAL_FIELDS, tt.tm_mday, 
					tt.tm_mon + 1, tt.tm_year + 1900, tt.tm_hour, tt.tm_min, tt.tm_sec, clnt->clientName, ret);
			insertEntry(critical_table, str1);
	} else {
printf("%s:%d\n", __FUNCTION__, __LINE__);
		H264_DVR_SetDVRMessCallBack(alarmCallBack, clnt->loginId);
		H264_DVR_SetupAlarmChan(clnt->loginId);
	}
}

dvrClient * findbyloginid(long int loginid) {
dvrClient *tmp = gdvrList;
	while(tmp != NULL) {
		if(tmp->loginId == loginid)
			return tmp;
		tmp=tmp->next;
	}
return NULL;
}

dvrClient * findbyipaddress(char *ipaddress) {
dvrClient *tmp = gdvrList;
unsigned int ip_num;
inet_aton(ipaddress, (struct in_addr *)&ip_num);
	while(tmp != NULL) {
		if(tmp->ipaddr == ip_num)
			return tmp;
		tmp=tmp->next;
	}
return NULL;
}

int initiateClients()
{
dvrClient *tmp = gdvrList;

printf("%s:%d\n", __FUNCTION__, __LINE__);
	while(tmp != NULL) {
		initClient(tmp);
		tmp=tmp->next;
	}

}

int main()
{
pthread_t ping_thread;
pthread_t processAlarm_thread;
pthread_t pollconf_thread;
/*
	if(daemonize != 0) {
		return EFAILURE;
	}
*/
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (ESUCCESS != readXmlConf(DVR_XML_CONF_FILE)) {
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (ESUCCESS != initDb()) {
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if(ESUCCESS != readDB_updateConf(NULL)) {
printf("%s:%d\n", __FUNCTION__, __LINE__);
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (pthread_create(&ping_thread, (pthread_attr_t *)NULL, ping_routine, (void *)NULL) != 0) {
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (pthread_create(&pollconf_thread, (pthread_attr_t *)NULL, pollconf, (void *)NULL) != 0) {
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if(startListner() != true ) {
printf("%s:%d\n", __FUNCTION__, __LINE__);
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (pthread_create(&processAlarm_thread, (pthread_attr_t *)NULL, processAlarm, (void *)NULL) != 0) {
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	if(initiateClients() != 0 ) {
		return EFAILURE;
	}
printf("%s:%d\n", __FUNCTION__, __LINE__);
	while (1) {
		sleep(3);
	}
}
