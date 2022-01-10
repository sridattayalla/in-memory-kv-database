#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "request_parse.h"
#include "hash_map.h"

#define PONG "+PONG\r\n"
#define OK "+OK\r\n"
#define NIL "$-1\r\n\r\n"
#define SIMPLE_STRING '+'
#define INTEGER ':'
#define BULK_STRING '$'

struct map_entry** hash_map;

void save_to_memory(char* key, char* val, char data_type, int size, long long expiry_timestamp){
	struct value *curr_val;
	curr_val = (struct value*) malloc(sizeof(struct value));
	curr_val->data_type = data_type;
	curr_val->val = val;
	curr_val->size = size;
	curr_val->expiry_timestamp = expiry_timestamp;
	insert(hash_map, key, curr_val);
}

struct value* fetch_from_memory(char* key){
	return fetch(hash_map, key);
}

int send_msg(int client_id, char *msg)
{
	return send(client_id, msg, strlen(msg), MSG_NOSIGNAL);
}

int parse_echo(int client_id, char *req_arr, int start_position)
{
	int i = start_position;
	if (req_arr[i++] != '$')
		return send_msg(client_id, PONG);
	struct rp_text token = get_text(req_arr, i);
	i = token.handle;
	int text_start = i - token.size;
	if (req_arr[i] != '\r' || req_arr[i] == '\0' || req_arr[i + 1] != '\n')
		return send_msg(client_id, PONG);
	int response_size = i - text_start + 6;
	char *text_to_send = malloc(response_size);
	text_to_send[0] = '$';
	text_to_send[1] = (i - text_start) + '0';
	text_to_send[2] = '\r';
	text_to_send[3] = '\n';
	for (int j = 0; j < (i - text_start); j++)
	{
		text_to_send[j + 4] = token.text[j];
	}
	text_to_send[response_size - 2] = '\r';
	text_to_send[response_size - 1] = '\n';
	int ret_val = send_msg(client_id, text_to_send);
	free(text_to_send);
	return ret_val;
}

int parse_set(int client_id, char *req_arr, int start_position)
{
	if (req_arr[start_position++] != BULK_STRING)
		return send_msg(client_id, PONG);
	struct rp_text key_text = get_text(req_arr, start_position);
	if (key_text.size == -1)
		return send_msg(client_id, PONG);
	int i = key_text.handle;
	if (req_arr[i] != '\r' || req_arr[i] == '\0' || req_arr[i + 1] != '\n')
		return send_msg(client_id, PONG);
	i += 2;
	char val_data_type = req_arr[i++];
	switch (val_data_type)
	{
		case SIMPLE_STRING:
		case BULK_STRING:
		{
			struct rp_text val_text = get_text(req_arr, i);
			long long expiry_timestamp = NULL;
			// fetch expiry time, if any
			i = val_text.handle;
			if (req_arr[i] != '\r' || req_arr[i] == '\0' || req_arr[i + 1] != '\n')return send_msg(client_id, PONG);
			i += 2;
			if(req_arr[i++] == '$')
			{
				struct rp_text px_text = get_text(req_arr, i);
				i = px_text.handle;
				if(!(strcmp(px_text.text, "PX")==0 || strcmp(px_text.text, "px")==0)) return send_msg(client_id, PONG);
				if (req_arr[i] != '\r' || req_arr[i] == '\0' || req_arr[i + 1] != '\n')return send_msg(client_id, PONG);
				i += 2;
				if(req_arr[i++] != '$') return send_msg(client_id, PONG);
				struct rp_text time_text = get_text(req_arr, i);
				expiry_timestamp = current_timestamp() + stoi(time_text.text);
			}
			save_to_memory(key_text.text, val_text.text, val_data_type, val_text.size, expiry_timestamp);
			return send_msg(client_id, OK);
		}
		default:
			return send_msg(client_id, PONG);
	}
}

int parse_get(int client_id, char* req_arr, int start_position)
{
	if (req_arr[start_position++] != BULK_STRING)
		return send_msg(client_id, PONG);
	struct rp_text key_text = get_text(req_arr, start_position);
	struct value* current_val = fetch_from_memory(key_text.text);
	if(current_val==NULL) return send_msg(client_id, NIL);
	int response_size = 3+current_val->size+1;
	if(current_val->data_type == BULK_STRING || current_val->data_type == SIMPLE_STRING)
	{
		response_size+=3;
	}
	char data_to_return[response_size];
	data_to_return[0] = current_val->data_type;
	if(current_val->data_type == BULK_STRING || current_val->data_type == SIMPLE_STRING)
	{
		data_to_return[1] = current_val->size + '0';
		data_to_return[2] = '\r';
		data_to_return[3] = '\n';
	}
	for(int j=0; j<current_val->size; j++){
		int increment = (current_val->data_type == BULK_STRING || current_val->data_type == SIMPLE_STRING) ? 4 : 1;
		data_to_return[j+increment] = current_val->val[j];
	}
	data_to_return[response_size-3] = '\r';
	data_to_return[response_size-2] = '\n';
	data_to_return[response_size-1] = '\0';
	printf("%s", data_to_return);
	return send_msg(client_id, data_to_return);
}

// executes the request and returns cursor position
int parse_arr(int client_id, char *req_arr, int start_position)
{	
	int i = start_position;
	struct rp_size arr_size = get_size(req_arr, i);
	if (arr_size.size == -1)
		return send_msg(client_id, PONG);
	i = arr_size.handle;
	if (req_arr[i] != '\r' || req_arr[i] == '\0' || req_arr[i + 1] != '\n')
		return send_msg(client_id, PONG);
	i += 2;
	while (arr_size.size--)
	{
		if (req_arr[i++] != '$')
			return send_msg(client_id, PONG);
		struct rp_text token = get_text(req_arr, i);
		i = token.handle;
		if (req_arr[i] != '\r' || req_arr[i] == '\0' || req_arr[i + 1] != '\n')
			return send_msg(client_id, PONG);
		i += 2;
		if (strcmp(token.text, "ECHO") == 0 || strcmp(token.text, "echo") == 0)
		{
			return parse_echo(client_id, req_arr, i);
		}
		if (strcmp(token.text, "SET") == 0 || strcmp(token.text, "set") == 0)
		{
			return parse_set(client_id, req_arr, i);
		}
		if (strcmp(token.text, "GET")==0 || strcmp(token.text, "get")==0)
		{
			return parse_get(client_id, req_arr, i);
		}
		return send_msg(client_id, PONG);
	}
}

int parse_test(char *req_arr, int pos)
{
	struct rp_text temp = get_text(req_arr, pos);
	printf("%s", temp.text);
}

int parse_req(int client_id, char *req_arr)
{
	int i = 0;
	switch (req_arr[i++])
	{
	case '\0':
		return send_msg(client_id, PONG);
	case '*':
		return parse_arr(client_id, req_arr, i);
	}
	return send_msg(client_id, PONG);
}

void display_request(char *req_arr)
{
	int i = 0;
	char next_char = req_arr[i++];
	while (next_char != '\0')
	{
		if (next_char == '\r')
		{
			printf("\\r");
		}
		else if (next_char == '\n')
		{
			printf("\\n");
		}
		else
		{
			printf("%c", next_char);
		}
		next_char = req_arr[i++];
	}
	printf("\n");
}

void initialize_memory()
{
	hash_map = create_hash_map();
}

void *handle_client(void *client)
{
	int *client_id_ptr = (int *)client;
	int client_id = *client_id_ptr;
	printf("Client Connected: %d\n", client_id);
	while (1)
	{
		char buffer[1024] = {0};
		int val_read = recv(client_id, buffer, 1024, 0);
		printf("%d: ", client_id);
		display_request(buffer);
		int op_res = parse_req(client_id, buffer);
		if (op_res == -1)
		{
			close(client_id);
			break;
		}
	}
}