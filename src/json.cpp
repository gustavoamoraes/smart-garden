#include <WString.h>
#include <stdio.h>
#include "json.h"

int serializeQueryResult (sqlite3* db, sqlite3_stmt *res, void (*write)(const char*)) 
{   
    bool first_row = true;
    int n_col = 0;

    write("[");  

    while (sqlite3_step(res) == SQLITE_ROW) 
    {
        n_col = sqlite3_column_count(res);

        if (!first_row) write(",");
        first_row = false;
        write("{");
        
        char buffer[56];
        int bytes_written = 0;

        for(int i=0; i< n_col; i++)
        {
            if(i) write(",");
            //Quote column name
            bytes_written = sprintf(buffer, "\"%s\":", sqlite3_column_name(res, i));
            bool is_text = sqlite3_column_type(res, i) == SQLITE_TEXT;
            //Quote value if it is text
            sprintf(&buffer[bytes_written], is_text ? "\"%s\"" : "%s", sqlite3_column_text(res, i));
            write(buffer);
        }

        write("}");
    }

    write("]");

    return 0;
}
