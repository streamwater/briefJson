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

bool json_obj2boolean(json_object *data, bool deft);
long long json_obj2integer(json_object *data, long long deft);
double json_obj2decimal(json_object *data, double deft);
char* json_obj2text(json_object *data, char *deft);
bool json_has_key(json_object *data, const char *key);

json_type json_get_obj_type(json_object *data);

json_object* json_get_value(json_object *data, const char *key);
bool json_get_boolean(json_object *data, const char *key, bool deft);
long long json_get_integer(json_object *data, const char *key, long long deft);
double json_get_decimal(json_object *data, const char *key, double deft);
char* json_get_text(json_object *data, const char *key, char* deft);

int json_get_array_size(json_object *arr_data);
json_object* json_get_array_element(json_object *arr_data, int index);

#endif // BRIEFJSON_H
