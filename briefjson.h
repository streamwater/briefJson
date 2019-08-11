#ifndef BRIEFJSON_H
#define BRIEFJSON_H
#include <wchar.h>

#ifndef __cplusplus
typedef enum { false, true }bool;
#endif // !__cplusplus

//types of json object
typedef enum { NONE, BOOLEAN, INTEGER, DECIMAL, TEXT, ARRAY, TABLE }json_type;

typedef struct json_object{
	json_type type;	//type of json object
	union
	{
		bool boolean;
		long long integer;
		double decimal;
		char *text;
		struct json_object *item;
	}value;		//union of json value
	struct json_object *next;	//if object included in array or table,it point to next object of the array or table
	char key[1];				//if object included in table,it is key
}json_object;

void json_object_free(json_object* data);	//free the memory of json object created by json_parse
void json_text_free(char json[]);	//free the memory of json text created by json_serialize;
json_object json_parse(char json[], char **message, long* error_pos);//parse json text to json object
char *json_serialize(json_object* data);//serialize json object to json text

#endif // BRIEFJSON_H
