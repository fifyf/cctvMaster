#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml2/libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "dvrmain.h"

dvrDbConf dbconf;

static void
parseOneConf (xmlDocPtr doc, xmlNodePtr cur) {
	xmlChar *key;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"db_server"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    strncpy(dbconf.server, (char *)key, 15);
		    xmlFree(key);
 	    }
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"db_user"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    strncpy(dbconf.username, (char *)key, 255);
		    xmlFree(key);
 	    }
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"db_password"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    strncpy(dbconf.password, (char *)key, 255);
		    xmlFree(key);
 	    }
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"database"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    strncpy(dbconf.database, (char *)key, 15);
		    xmlFree(key);
 	    }
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"service_port"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    dbconf.service_port = atoi((char *)key);
		    xmlFree(key);
 	    }
	cur = cur->next;
	}
    return;
}

int readXmlConf(char *docname) 
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);
	
	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return EFAILURE;
	}
	
	cur = xmlDocGetRootElement(doc);
	
	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return EFAILURE;
	}
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "configuration")) {
		fprintf(stderr,"document of the wrong type, root node != story");
		xmlFreeDoc(doc);
		return EFAILURE;
	}
	
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		parseOneConf (doc, cur);
		cur = cur->next;
	}
	printf("%s\n%s\n%s\n%s\n", dbconf.server, dbconf.username, dbconf.password, dbconf.database);
	xmlFreeDoc(doc);
	return ESUCCESS;
}
