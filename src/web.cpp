#include <uri/UriBraces.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "SPIFFS.h"

#include "json.h"
#include "main.h"
#include "web.h"
#include "sql.h"

//Network
const String static_files_path = "/static/";
WebServer server(80);

//Database
int rc;
const char *tail;
sqlite3_stmt *res;

//JSON
String jsonDocChunck = "";
int chuncksSent = 0;
const int minLenght = 1024;

String getMIMEType(String filename) 
{
  if (server.hasArg("download")) 
  {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  }
  return "text/plain";
}

//Streams file content
bool handleFileRead(String path) 
{
  String contentType = getMIMEType(path);
  File file = SPIFFS.open(path, "r");
  bool sucess = false;

  //If file exists
  if(!file.isDirectory()) 
  {
    server.streamFile(file, contentType);
    sucess = true;
  }

  file.close();
  return sucess;
}

//Redirect to home page file
void handleRoot()
{
  server.send(200, "text/html", "<script type=\"text/javascript\">window.location.href = \"static/index.html\";</script>");
}

void write(const char* text)
{
  jsonDocChunck += text;
  
  if (jsonDocChunck.length() > minLenght){
    server.sendContent(jsonDocChunck);
    chuncksSent++;
    jsonDocChunck = "";
  }
}

//Sends the result in json of the sql
void sendSQLResult(const char* sql)
{
  chuncksSent = 0;
  jsonDocChunck = "";

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json");

  rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);

  if(rc != SQLITE_OK){
    return;
  }

  serializeQueryResult(db, res,  write);
  sqlite3_finalize(res);

  //Send remaining chunck 
  server.sendContent(jsonDocChunck);
}

//Inserts a json object in the table
int insertJsonObject (String& table_name, String& json)
{
  String sql = jsonToSQLInsert(table_name, json);
  rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

  if(rc != SQLITE_OK){
      Serial.print(sqlite3_errmsg(db));
      return 1;
  }
  return 0;
}

//All web paths
void bindAll() 
{
    // Home
    server.on("/", HTTP_GET, handleRoot);

    //Static files
    server.on ( UriBraces(static_files_path + "{}"), HTTP_GET, []()
    {
        String file_path = static_files_path + server.pathArg(0);
        if (!handleFileRead(file_path))
        {
          server.send(404, "text/plain", "FileNotFound");
        }
    });

    server.on ( UriBraces("/water/{}"), HTTP_GET, []()
    {
      int seconds = server.pathArg(0).toInt();
      initiateIrrigation(seconds);
      server.send(200);
    });

    //Delete the database
    server.on ( "/reset", HTTP_GET, []()
    {
      if (SPIFFS.remove("/my.db")){
        server.send(200, "text/plain","Deleted database.");
      }else{
        server.send(200, "text/plain","Something went wrong.");
      }
    });

    //GET: Sends the whole table. POST: inserts a item on the table
    server.on(UriBraces("/api/{}"), []()
    {   
        String table = server.pathArg(0);

        if( server.method() == HTTP_GET)
        {
          int arg_count = server.args();
          RequestArgument args[arg_count];

          for (size_t i = 0; i < arg_count; i++){
              args[i].key = server.argName(i);
              args[i].value = server.arg(i);
          }
          
          String sql = getQueryToSQL(table, args, arg_count);
          sendSQLResult(sql.c_str());
        }
        else if (server.method() == HTTP_POST)
        {
          String post = server.arg(0);
          insertJsonObject(table, post);
          server.send ( 200 );
        }
    });

    //Sends the latests entry of a table
    server.on(UriBraces("/api/{}/latest"), HTTP_GET, []()
    {   
        String table = server.pathArg(0);

        int arg_count = server.args();
        RequestArgument args[arg_count];

        for (size_t i = 0; i < arg_count; i++){
            args[i].key = server.argName(i);
            args[i].value = server.arg(i);
        }
        
        String sql = getQueryToSQL(table, args, arg_count);
        sql += " ORDER BY rowid DESC LIMIT 1";
        sendSQLResult(sql.c_str());
    });
}

//Thread function
void serveFoverer (void* data) 
{
  while (true){
    server.handleClient();
  }
}

void startServer ()
{
  bindAll();

  server.begin();

  //Start web sever thread
  xTaskCreate(&serveFoverer, "WebServer", 50000, NULL, 0, NULL); 
}