#pragma once

#include <ArduinoJson.h>
#include "Arduino.h"
#include "web.h"
#include <map>

const std::map<String, String> CONDITION_MAP = {
    { "gt", ">" },
    { "lt", "<" },
};

const char* DELIMITER = "__";

String getQueryToSQL (String& table_name, RequestArgument* args, int arg_count);
String jsonToSQLInsert (String& table, String& json_values);