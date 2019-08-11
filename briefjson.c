#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "briefjson.h"

#define BUFFER_SIZE 1024

const static char espsrc[] = "\b\t\n\f\r\"'";
const static char *const espdes[] = { "\\b","\\t","\\n","\\f","\\r","\\\"","\\'" };
const static char espuni[] = "\\u";
const static size_t sz_esp = sizeof(espdes) / sizeof(void *);

typedef struct
{
	json_object data;
	char *json;
	char *pos;
	char *message;
}parse_engine;

typedef struct string_node
{
	size_t size;
	struct string_node *next;
	char string[BUFFER_SIZE];
}string_node;

typedef struct
{
	size_t size;
	string_node *last;
	string_node first;
}string_buffer;

static void buffer_append(string_buffer *buffer, const char* string, size_t length)
{
	if (!buffer->last)
		buffer->last = &buffer->first;
	const char *pos = string;
	while (length)
	{
		size_t copysize = length;
		string_node *copynode = buffer->last;
		if (copynode->size + length > BUFFER_SIZE)
		{
			copysize = BUFFER_SIZE - copynode->size;
			string_node *node = (string_node *)malloc(sizeof(string_node));
			node->next = 0;
			node->size = 0;
			copynode->next = node;
			buffer->last = node;
		}
		strncpy(copynode->string + copynode->size, pos, copysize);
		pos += copysize;
		length -= copysize;
		buffer->size += copysize;
		copynode->size += copysize;
	}
}

static void buffer_addchar(string_buffer *buffer, const char ch)
{
	if (!buffer->last)
		buffer->last = &buffer->first;
	if (buffer->last->size == BUFFER_SIZE)
	{
		buffer->last->next = (string_node *)malloc(sizeof(string_node));
		buffer->last = buffer->last->next;
		buffer->last->next = 0;
		buffer->last->size = 0;
	}
	buffer->last->string[buffer->last->size++] = ch;
	++buffer->size;
}

static char *buffer_tostr(string_buffer *buffer)
{
	char *string = (char *)malloc(sizeof(char)*(buffer->size + 1));
	char *pos = string;
	string[buffer->size] = 0;
	string_node *first = &buffer->first;
	strncpy(pos, first->string, first->size);
	pos += first->size;
	while (first->next)
	{
		string_node *node = first->next;
		first->next = node->next;
		strncpy(pos, node->string, node->size);
		pos += node->size;
		free(node);
	}
	return string;
}

static void buffer_free(string_buffer *buffer)
{
	string_node *first = &buffer->first;
	while (first->next)
	{
		string_node *node = first->next;
		first->next = node->next;
		free(node);
	}
}

static void string_escape(string_buffer *sb, char string[], size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		const char ch = string[i];
		const char *posesp = strchr(espsrc, ch);
		if (posesp)
		{
			const char *str = espdes[posesp - espsrc];
			buffer_append(sb, str, strlen(str));
		}
		else
			buffer_addchar(sb, ch);
	}
}

static char* string_revesp(char string[], char *end)
{
	char *pos = string;
	string_buffer sb = { 0 };
	while (pos < end)
	{
		if (!strncmp(pos, espuni, 2))
		{
			if (end - pos < 6) {
				buffer_free(&sb);
				return 0;
			}
			pos += 2;
			char num = 0;
			for (int i = 0; i < 4; ++i)
			{
				char tmp = *pos++;
				if (tmp >= '0'&&tmp <= '9')
					tmp = tmp - '0';
				else if (tmp >= 'A'&&tmp <= 'F')
					tmp = tmp - ('A' - 10);
				else if (tmp >= 'a'&&tmp <= 'f')
					tmp = tmp - ('a' - 10);
				num = num << 4 | tmp;
			}
			buffer_addchar(&sb, num);
			continue;
		}
		size_t i = 0;
		for (; i < sz_esp; ++i)
		{
			size_t len = strlen(espdes[i]);
			if (!strncmp(pos, espdes[i], len))
			{
				buffer_addchar(&sb, espsrc[i]);
				pos += len;
				break;
			}
		}
		if (i == sz_esp)
			buffer_addchar(&sb, *pos++);
	}
	return buffer_tostr(&sb);
}

static json_object* insert_item(json_object *list, char *key)
{
	json_object *item;
	size_t len = 0;
	if (key)
	{
		len = strlen(key);
		item = (json_object *)malloc(sizeof(json_object) + sizeof(char)*len);
		strncpy(item->key, key, len);
	}
	else item = (json_object *)malloc(sizeof(json_object));
	item->type = NONE;
	item->key[len] = 0;
	item->next = 0;
	if (!list->value.item) list->value.item = item;
	else {
		static json_object *head = 0;
		static json_object *rear = 0;
		if (!rear || list != head)
		{
			head = list;
			for (json_object *i = list->value.item;; i = i->next)
				if (!i->next)
				{
					rear = i;
					break;
				}
		}
		rear->next = item;
		rear = item;
	}
	return item;
}

static char next_token(parse_engine* engine) {
	char ch;
	while ((ch = *engine->pos++) <= ' '&&ch>0);
	return *(engine->pos - 1);
}

static int parsing(parse_engine* engine, json_object *pos_parse)
{
	char c = next_token(engine);
	switch (c)
	{
	case 0:
		engine->message = (char *)"Unexpected end";
		return 1;

	case '[':
	{
		pos_parse->type = ARRAY;
		pos_parse->value.item = 0;
		if (next_token(engine) == ']') return 0;
		--engine->pos;
		while (1)
		{
			json_object *item = insert_item(pos_parse, 0);
			if (next_token(engine) == ',')
			{
				--engine->pos;
			}
			else
			{
				--engine->pos;
				if (parsing(engine, item))
					return 1;
			}
			switch (next_token(engine))
			{
			case ',':
				if (next_token(engine) == ']')
					return 0;
				--engine->pos;
				break;
			case ']':
				return 0;
			default:
				engine->message = (char *)":Expected a ',' or ']'";
				return 1;
			}
		}
	}

	case '{':
	{
		pos_parse->type = TABLE;
		pos_parse->value.item = 0;
		if (next_token(engine) == '}') return 0;
		--engine->pos;
		while (1)
		{
			json_object key;
			if (parsing(engine, &key) || key.type != TEXT)
			{
				json_object_free(&key);
				engine->message = (char *)"Illegal key of pair";
				return 1;
			}
			if (next_token(engine) != ':')
			{
				engine->message = (char *)"Expected a ':'";
				return 1;
			}
			json_object* item = insert_item(pos_parse, key.value.text);
			json_object_free(&key);
			if (parsing(engine, item))
			{
				return 1;
			}
			switch (next_token(engine))
			{
			case ';':
			case ',':
				if (next_token(engine) == '}')
					return 0;
				--engine->pos;
				break;
			case '}':
				return 0;
			default:
				engine->message = (char *)"Expected a ',' or '}'";
				return 1;
			}
		}
	}

	case '\'':
	case '"':
	{
		char *start = engine->pos;
		while (*engine->pos != c)
		{
			engine->pos += *engine->pos == '\\';
			if (!*engine->pos++) {
				engine->message = (char *)"Unterminated string";
				return 1;
			}
		}
		pos_parse->type = TEXT;
		pos_parse->value.text = string_revesp(start, engine->pos++);
		return !pos_parse->value.text;
	}
	}
	char *start = engine->pos - 1;
	while (c >= ' ') {
		if (strchr(",:]}/\"[{;=#", c))
			break;
		c = *engine->pos++;
	}
	char *string = string_revesp(start, --engine->pos);
	if (!string) return 1;
	if (!strcmp(string, "TRUE") || !strcmp(string, "true"))
	{
		pos_parse->type = BOOLEAN;
		pos_parse->value.boolean = true;
	}
	else if (!strcmp(string, "FALSE") || !strcmp(string, "false"))
	{
		pos_parse->type = BOOLEAN;
		pos_parse->value.boolean = false;
	}
	else if (!strcmp(string, "NUL") || !strcmp(string, "nul"))
	{
		pos_parse->type = NONE;
	}
	else
	{
		pos_parse->type = (strchr(string, '.') || strchr(string, 'e') || strchr(string, 'E')) ? DECIMAL : INTEGER;
		const char *format = pos_parse->type == INTEGER ? "%lld" : "%lf";
		if (!sscanf(string, format, &pos_parse->value))
		{
			pos_parse->type = TEXT;
			pos_parse->value.text = string;
			return 0;
		}
	}
	free(string);
	return 0;
}

static void object_to_string(json_object *data, string_buffer *sb)
{
	switch (data->type) {
	case NONE:
	{
		buffer_append(sb, "nul", 4);
		break;
	}
	case INTEGER:
	case DECIMAL:
	{
		char buffer[32] = { 0 };
		if (data->type == INTEGER)
			snprintf(buffer, sizeof(buffer), "%lld", data->value.integer);
		else
			snprintf(buffer, sizeof(buffer), "%lf", data->value.decimal);
		buffer_append(sb, buffer, strlen(buffer));
		break;
	}
	case BOOLEAN:
	{
		if (data->value.boolean)
			buffer_append(sb, "true", 4);
		else
			buffer_append(sb, "false", 5);
		break;
	}
	case TEXT:
	{
		buffer_addchar(sb, '"');
		string_escape(sb, data->value.text, strlen(data->value.text));
		buffer_addchar(sb, '"');
		break;
	}
	case TABLE:
	{
		json_object *item = data->value.item;
		int index = 0;
		while (item)
		{
			if (index) buffer_addchar(sb, ',');
			else buffer_addchar(sb, '{');
			json_object key;
			key.type = TEXT;
			key.value.text = item->key;
			object_to_string(&key, sb);
			buffer_addchar(sb, ':');
			object_to_string(item, sb);
			item = item->next;
			++index;
		}
		buffer_addchar(sb, '}');
		break;
	}
	case ARRAY:
	{
		json_object *item = data->value.item;
		int index = 0;
		while (item)
		{
			buffer_addchar(sb, index ? ',' : '[');
			object_to_string(item, sb);
			item = item->next;
			++index;
		}
		buffer_addchar(sb, ']');
		break;
	}
	}
}

json_object json_parse(char json[], char **message, long* error_pos)
{
	parse_engine result;
	result.data.type = NONE;
	result.pos = json;
	result.json = json;
	if (parsing(&result, &result.data))
	{
		if (message)		*message = result.message;
		json_object_free(&result.data);
		if (error_pos)		*error_pos = result.pos - result.json;
		json_object null_item;
		null_item.type = NONE;
		return null_item;
	}
	if (message)	*message = (char *)"SUCCEED";
	if (error_pos) *error_pos = 0;
	return result.data;
}

char *json_serialize(json_object *data)
{
	string_buffer head = { 0 };
	object_to_string(data, &head);
	char *string = buffer_tostr(&head);
	return string;
}

void json_object_free(json_object *data)
{
	if (data->type == ARRAY || data->type == TABLE)
		while (data->value.item)
		{
			json_object *item = data->value.item;
			data->value.item = item->next;
			json_object_free(item);
			free(item);
		}
	else if (data->type == TEXT)
		free(data->value.text);
}

void json_text_free(char json[])
{
	free(json);
}


bool json_obj2boolean(json_object *data, bool deft) {
	return NULL == data ? deft : data->value.boolean;
}

long long json_obj2integer(json_object *data, long long deft) {
	return NULL == data ? deft : data->value.integer;
}

double json_obj2decimal(json_object *data, double deft) {
	return NULL == data ? deft : data->value.decimal;
}

char* json_obj2text(json_object *data, char *deft) {
	return NULL == data ? deft : data->value.text;
}

bool json_has_key(json_object *data, const char *key) {
	if (data->type == TABLE) {
		json_object *item = (json_object *)data->value.item;
		while (item)
		{
			if (0 == strcmp((const char* )item->key, key)) {
				return true;
			}
			item = item->next;
		}
	}
	return false;
}

json_type json_get_obj_type(json_object *data) {
	return data->type;
}

json_object* json_get_value(json_object *data, const char *key) {
	if (data->type == TABLE) {
		json_object *item = (json_object *)data->value.item;
		while (item) {
			if (0 == strcmp((const char* )item->key, key)) {
				return item;
			}
			item = item->next;
		}
	}
	return NULL;
}

bool json_get_boolean(json_object *data, const char *key, bool deft) {
	json_object *value = json_get_value(data, key);
	return NULL == value ? deft : json_obj2boolean(value, deft);
}

long long json_get_integer(json_object *data, const char *key, long long deft) {
	json_object *value = json_get_value(data, key);
	return NULL == value ? deft : json_obj2integer(value, deft);
}

double json_get_decimal(json_object *data, const char *key, double deft) {
	json_object *value = json_get_value(data, key);
	return NULL == value ? deft : json_obj2decimal(value, deft);
}

char* json_get_text(json_object *data, const char *key, char* deft) {
	json_object *value = json_get_value(data, key);
	return NULL == value ? deft : json_obj2text(value, deft);
}

int json_get_array_size(json_object *arr_data) {
	int size = 0;

	if (arr_data->type == ARRAY) {
		json_object *item = (json_object *)arr_data->value.item;

		while (item) {
			item = item->next;
			size++;
		}
	}
	return size;
}

json_object* json_get_array_element(json_object *arr_data, int index) {
	if (arr_data->type == ARRAY) {
		json_object *item = (json_object *)arr_data->value.item;
		int i = 0;

		while (item) {
			if (i == index) {
				return item;
			}
			item = item->next;
			i++;
		}
	}
	return NULL;
}
