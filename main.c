#include <stdio.h>
#include "briefJson.h"

static void print(json_object *data, int n,char *key)
{
	for (int i = 0; i < n; ++i)
		printf(" |");
	printf("-");
	if(key) printf("key:%s\t", key);
	switch (data->type)
	{
	case TABLE: 
	{
		printf("type:map\n");
		json_object *item = (json_object *)data->value.item;
		while (item)
		{
			print(item, n + 1, item->key);
			item = item->next;
		}
		return;
	}
	case ARRAY:
	{
		printf("type:array\n");
		json_object *item = (json_object *)data->value.item;
		while (item)
		{
			print(item, n + 1, 0);
			item = item->next;
		}
		return;
	}
	case BOOLEAN:
	{
		printf("type:bool\tvalue:%s\n",data->value.boolean?"true":"false");
		break;
	}
	case DECIMAL:
	{
		printf("type:decimal\tvalue:%lf\n", data->value.decimal);
		break;
	}
	case INTEGER:
	{
		printf("type:integer\tvalue:%lld\n", data->value.integer);
		break;
	}
	case TEXT:
	{
		printf("type:text\tvalue:%s\n", data->value.text);
		break;
	}
	case NONE:
	{
		printf("type:null\n");
		break;
	}
	default:
		break;		
	}
}


int main()
{
	char json[] = "{\"programmers\":[{\"firstName\":\"Brett\",\"lastName\":\"McLaughlin\",\"email\":\"aaaa\",\"age\":3.3},{\"firstName\":\"Jason\",\"lastName\":\"Hunter\",\"email\":\"bbbb\",\"age\":25},{\"firstName\":\"Elliotte\",\"lastName\":\"Harold\",\"email\":\"cccc\",\"age\":30}],\"authors\":[{\"firstName\":\"Isaac\",\"lastName\":\"Asimov\",\"genre\":\"sciencefiction\",\"age\":53},{\"firstName\":\"Tad\",\"lastName\":\"Williams\",\"genre\":\"fantasy\",\"age\":47},{\"firstName\":\"Frank\",\"lastName\":\"Peretti\",\"genre\":\"christianfiction\",\"age\":28}],\"musicians\":[{\"firstName\":\"Eric\",\"lastName\":\"Clapton\",\"instrument\":\"guitar\",\"age\":19},{\"firstName\":\"Sergei\",\"lastName\":\"Rachmaninoff\",\"instrument\":\"piano\",\"age\":26}]}";
	json_object item = json_parse(json,0,0);
	print(&item, 0, 0);
	printf("\n");
	char *text = json_serialize(&item);
	printf("%s\n", text);
	json_object_free(&item);
	json_text_free(text);
 	return 0;
}
