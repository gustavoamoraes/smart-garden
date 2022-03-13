#include "sql.h"

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

        char* column_name = strtok(buf, DELIMITER);
        char* condition_id = strtok(NULL, DELIMITER);     

        String condition;

        if(!condition_id){
          condition = "=";
        }else{
          condition = CONDITION_MAP.at(condition_id);
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