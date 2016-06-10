#!/usr/bin/python

import sys
import MySQLdb

option=1;
if ( len(sys.argv) > 2 ):
	print "Improper arguments"
	sys.exit(1)

if ( len(sys.argv) == 2 ):
	if ( str(sys.argv[1]) == "DEL" ):
		option=2;
	elif ( str(sys.argv[1]) == "ADD" ):
		option=0;

# Open database connection
db = MySQLdb.connect("192.168.1.6","nanotech","nanotech","nanotechdb" )

# prepare a cursor object using cursor() method
cursor = db.cursor()

sql = "select * from configuration where clientName='client1'"
cursor.execute(sql);
results=cursor.fetchall()

for row in results:
	ipaddr=row[1];
	videomotion=row[9];

print "videomotion = %s" % videomotion

if ( videomotion == 1 ):
	print "videomotion = 1; setting to 2"
	upd = "update configuration set video_motion=2 where clientName='client1'"
if ( videomotion == 2 ):
	print "videomotion = 2; setting to 1"
	upd = "update configuration set video_motion=1 where clientName='client1'"

cursor.execute(upd);
db.commit()

sql = "select * from configuration where clientName='client1'"
cursor.execute(sql);
results=cursor.fetchall()

for row in results:
	newvideomotion=row[9];

print "newvideomotion = %s" % newvideomotion

sql="insert into changeconf (commandName, attribute1, attribute2) values ('REFRESH_IP', '%s', '%s')" % (ipaddr, option);

print "%s" % sql;
cursor.execute(sql);
db.commit()

# disconnect from server
db.close()
