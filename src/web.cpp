#include <uri/UriBraces.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <sqlite3.h>
#include "SPIFFS.h"
#include "json.h"
#include "main.h"
#include <map>

//Network
const String static_files_path = "/static/";
WebServer server(80);

//Database
int rc;
sqlite3 *db;
const char *tail;
sqlite3_stmt *res;

//JSON
String jsonDocChunck = "";
int chuncksSent = 0;
const int minLenght = 1024;

//API 
const std::map<String, String> condition_map = 
{
    { "gt", ">" },
    { "lt", "<" },
};

const char* delimiter = "__";

struct RequestArgument 
{
  String key;
  String value;
};
//

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
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
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

String getQueryToSQL (String& table_name, RequestArgument* args, int arg_count)
{
    String sql = "SELECT * FROM " + table_name;

    if(arg_count){
      sql += " WHERE ";

      for (size_t i = 0; i < arg_count; i++)
      {
        if (i) sql += " AND ";

        String param_name = args[i].key;
        String param_value = args[i].value;

        char buf[36];
        param_name.toCharArray(buf, param_name.length()+1);

        char* column_name = strtok(buf, delimiter);
        char* condition_id = strtok(NULL, delimiter);     

        String condition;

        if(!condition_id){
          condition = "=";
        }else{
          condition = condition_map.at(condition_id);
        }

        sql += column_name;
        sql += " ";
        sql += condition;
        sql += " ";
        sql += param_value;
      }
    }

    return sql;
}

String jsonToSQLInsert (String& table, String& json_values)
{
    String sql = "INSERT INTO " + table;

    DynamicJsonDocument doc(128);
    deserializeJson(doc, json_values);
    JsonObject root = doc.as<JsonObject>();

    String columns = " (";
    String values = " (";

    bool first = true;

    for (JsonPair kv : root) 
    {
      if(!first){
        columns += ","; 
        values += ","; 
      }else{
        first=false;
      }

      columns += kv.key().c_str();

      bool isText = kv.value().is<JsonString>();
      bool isFloat = kv.value().is<JsonFloat>();
      bool isInteger = kv.value().is<JsonInteger>();     

      if(isText){
        values += "\'" ;
        values += kv.value().as<const char*>() ;
        values += "\'";
      }
      else if (isFloat){
        values += kv.value().as<float>();
      }
      else if (isInteger){
        values += kv.value().as<int>();
      }
    }

    columns += ")";
    values += ")";

    sql += columns;
    sql += " VALUES" + values;

    return sql;
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

void sendSQLResult(const char* sql)
{
  chuncksSent = 0;
  jsonDocChunck = "";

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json");

  rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);

  if(rc != SQLITE_OK){
    Serial.print(sqlite3_errmsg(db));
    return;
  }

  serializeQueryResult(db, res,  write);
  sqlite3_finalize(res);

  //Send remaining chunck 
  server.sendContent(jsonDocChunck);
}

int openDb(const char *filename, sqlite3 **db) 
{
  int rc = sqlite3_open(filename, db);
  if (rc) {
      Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
      return rc;
  } else {
      Serial.printf("Opened database successfully\n");
  }
  return rc;
}

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

    // server.on ( "/config/", []()
    // {
    //   if( server.method() == HTTP_POST)
    //   {
    //     if (!server.args())
    //       return;

    //     String config_json = server.arg(0);
    //     Config* new_config = new Config(config_json);
    //     updateMainConfig(new_config);
    //     server.send ( 200 );
    //   }
    //   else if (server.method() == HTTP_GET)
    //   { 
    //     String content; main_config->serialize(content);
    //     server.send ( 200 , "application/json", content);
    //   }
    // });

    server.on ( UriBraces("/water/{}"), HTTP_GET, []()
    {
      int seconds = server.pathArg(0).toInt();
      initiateIrrigation(seconds);
      server.send(200);
    });

    server.on ( "/reset", HTTP_GET, []()
    {
      if (SPIFFS.remove("/my.db")){
        server.send(200, "text/plain","Deleted database.");
      }else{
        server.send(200, "text/plain","Something went wrong.");
      }
    });

    //APIs
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

void startServer ()
{
  if (openDb("/spiffs/my.db", &db))
    return;

  sqlite3_initialize();

  sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS measurements (value FLOAT, type TEXT, date DATETIME, sensor_id INT)", NULL, NULL, NULL);

  bindAll();
  server.begin();
}

//Thread function
void serveFoverer (void* data) 
{
  while (true)
  {
    server.handleClient();
  }
}