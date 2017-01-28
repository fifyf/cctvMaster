#define hddtable "hddInfo"

int insertEntry(char *, char *);
int readDB_updateConf(char *);
int initDb();
int readChangeConf(dvrchangeconf **confHead);
int deleteAllEntries(char *table);

extern dvrClient *gdvrList;
