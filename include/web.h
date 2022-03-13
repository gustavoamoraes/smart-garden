#pragma once

struct RequestArgument 
{
  String key;
  String value;
};

int insertJsonObject (String& table_name, String& json);
void startServer ();