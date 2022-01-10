#include <ctype.h>

struct rp_size{
	int size;
	int handle;
};

struct rp_text{
	char* text;
	int size;
	int handle;
};

int stoi(char* s){
	int num = 0;
	int i = 0;
	char next_char = s[i++];
	while (next_char != '\0')
	{
		num = (num*10)+(next_char-'0');
		next_char = s[i++];
	}
	return num;
}

int get_next_clrf(char *req_arr, int start_position)
{
	char next_char = req_arr[start_position++];
	while (next_char != '\r' && next_char != '\0')
	{
		next_char = req_arr[start_position++];
	}
	return --start_position;
}

struct rp_size get_size(char *req_arr, int start_postition)
{	
	int end_position = get_next_clrf(req_arr, start_postition);
	int arr_size = 0;
	for (int j = start_postition; j < end_position; j++)
	{
		if (!isdigit(req_arr[j]))
			return (struct rp_size){-1, -1};
		arr_size = (arr_size * 10) + (req_arr[j] - '0');
	}
	struct rp_size current_size = {arr_size, end_position};
	return current_size;
}

struct rp_text get_text(char *req_arr, int start_position)
{
	struct rp_size current_size = get_size(req_arr, start_position);
	if(!(req_arr[current_size.handle] == '\r' && req_arr[current_size.handle+1] == '\n')) return (struct rp_text){"", -1};
	int text_start = current_size.handle+2;
	char *token;
	token = (char*) malloc((current_size.size+1) * sizeof(char));
	for (int j = text_start; j < text_start+current_size.size; j++)
	{
		token[j - text_start] = req_arr[j];
	}
	token[current_size.size] = '\0';
	struct rp_text current_text = {token, current_size.size, text_start+current_size.size};
	return current_text;
}