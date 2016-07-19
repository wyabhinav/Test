/**
    @uthor: Abhinav Jain
    Run Command: g++ main.cpp -I./mysql/ -L./ -L/usr/lib64 -lpthread -ldl -lmariadbclient -lssl -lcrypto -I./sqlite-snapshot-201607151001 -lsqlite3
**/

#include<stdio.h>
#include<mysql.h>
#include<sqlite3.h>
#include<string>
#include<cstring>
using namespace std;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int main(int argc, char* argv[]){
    MYSQL *connectionLink;
    MYSQL_RES *result;
    MYSQL_ROW row;
    const char *server = "192.168.2.175";          /// System itself acts as the server

    ///Credentials initialization (Please update as per your system)
    const char *user = "abhinav";
    const char *password = "abhinav";
    const char *database = "TestDB";

    connectionLink = mysql_init(NULL);        /// Allocates a MySQL object

    if(!mysql_real_connect(connectionLink,server,user,password,database,0,NULL,0)){        /// Connection establishment
        printf("MySQL: Connection error\n%s",mysql_error(connectionLink));
        return 1;
    }

    const char *query = "SHOW GLOBAL STATUS";
    if(mysql_query(connectionLink,query)){
        printf("MySQL: Query execution failed:\n%s",mysql_error(connectionLink));
        return 2;
    }
    result = mysql_use_result(connectionLink);               /// Fetching result

    sqlite3 *db;
    char *errorMessage=0;
    int rc;
    rc = sqlite3_open("TestDB.db",&db);
    if(rc){
        printf("SQLite: Cannot Open Database\n%s",sqlite3_errmsg(db));
        return 3;
    }

    query = "CREATE TABLE IF NOT EXISTS mysqlOutput(Variable_Name CHAR(255) PRIMARY KEY, Variable_Value CHAR(255));";
    /*query = "CREATE TABLE IF NOT EXISTS mysqlOutput("  \
         "Variable_Name CHAR(255)," \
         "Variable_Value CHAR(255));";*/
    rc = sqlite3_exec(db,query,callback,0,&errorMessage);
    if(rc != SQLITE_OK){
        printf("SQLite: Query Execution failed\n%s",sqlite3_errmsg(db));
        return 4;
    }

    sqlite3_stmt *insert_stmt;
    query = "INSERT INTO mysqlOutput VALUES(?,?);";
    rc = sqlite3_prepare_v2(db,query,-1,&insert_stmt,NULL);
    if(rc != SQLITE_OK) {
		printf("SQLite: Can't prepare statement for inserting\n%s",sqlite3_errmsg(db));
		sqlite3_close(db);
		return 5;
	}
    int i=1;
    while((row = mysql_fetch_row(result)) != NULL){
        rc = sqlite3_bind_text(insert_stmt,1,row[0],-1,0);
        if(rc != SQLITE_OK){
            printf("Error binding 1st parameter:\n%s",sqlite3_errmsg(db));
            return 6;
        }
        rc = sqlite3_bind_text(insert_stmt,2,row[1],-1,0);
        if(rc != SQLITE_OK){
            printf("Error binding 2nd parameter:\n%s",sqlite3_errmsg(db));
            return 7;
        }
        rc = sqlite3_step(insert_stmt);
        if(rc != SQLITE_DONE){
            printf("SQLite: Problem inserting values\n%s",sqlite3_errmsg(db));
            return 8;
        }
        sqlite3_reset(insert_stmt);
    }
    sqlite3_finalize(insert_stmt);
    sqlite3_stmt *select_stmt;
    query = "SELECT * FROM mysqlOutput;";
    //rc = sqlite3_exec(db,query,callback,0,&errorMessage);
    if(sqlite3_prepare_v2(db,query,-1,&select_stmt,NULL) != SQLITE_OK){
        printf("SQLite: Display values unsuccessful\n");
        return 9;
    }
    else{
        int colCount = sqlite3_column_count(select_stmt);
        printf("Column Count: %d\n",colCount);
        while(true){
            rc = sqlite3_step(select_stmt);
            if(rc == SQLITE_ROW){
                for(int i=0;i<colCount;++i){
                    char *s = (char*)sqlite3_column_text(select_stmt,i);
                    printf("%s\t\t\t",s);
                }
                printf("\n");
            }
            if(rc == SQLITE_DONE)   break;
        }
    }
    sqlite3_finalize(select_stmt);
    sqlite3_close(db);
    mysql_free_result(result);                ///Release the buffer
    mysql_close(connectionLink);              ///Close the connection
    return 0;
}
