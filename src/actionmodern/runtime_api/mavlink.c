#include <math.h>

#include <arpa/inet.h>

#include <yyjson.h>

#include <objects.h>
#include <flashbang.h>
#include <mavlink.h>

#include <initial_strings_decls.h>

#define BUFFER_SIZE 1024

socklen_t addr_len = sizeof(struct sockaddr_in);

int sockfd;

struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

void recompSITLInit(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	memset(&server_addr, 0, addr_len);
	memset(&client_addr, 0, addr_len);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9002);
	
	bind(sockfd, (const struct sockaddr*) &server_addr, addr_len);
	
	RETURN_VOID();
}

void recompSITLReadPacket(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar out_obj_v;
	popVar(app_context, &out_obj_v);
	
	DISCARD_ARGS(num_args - 1);
	
	char data[BUFFER_SIZE];
	
	int read = recvfrom(sockfd, (char*) data, BUFFER_SIZE - 1, 0, (struct sockaddr*) &client_addr, &addr_len);
	
	if (read != -1)
	{
		u16 magic = VAL(u16, &data[0]);
		u16 frame_rate = VAL(u16, &data[2]);
		u32 frame_count = VAL(u32, &data[4]);
		u16* pwm = (u16*) &data[8];
		
		//~ printf("read: %d, magic: %d, frate: %d, fcount: %d, pwm[0]: %d, pwm[2]: %d\n", read, magic, frame_rate, frame_count, pwm[0], pwm[2]);
		
		ActionVar thruster_v;
		thruster_v.type = ACTION_STACK_VALUE_F64;
		
		thruster_v.f64 = (f64) pwm[2];
		setProperty(app_context, out_obj_v.object, STR_ID_leftThruster, NULL, 0, &thruster_v);
		
		thruster_v.f64 = (f64) pwm[0];
		setProperty(app_context, out_obj_v.object, STR_ID_rightThruster, NULL, 0, &thruster_v);
		
		PUSH_BOOL(true);
	}
	
	else
	{
		PUSH_BOOL(false);
	}
	
	releaseObjectVar(app_context, &out_obj_v);
}

void recompSITLSendJSON(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar text_v;
	popVar(app_context, &text_v);
	
	DISCARD_ARGS(num_args - 1);
	
	char out[1024];
	snprintf(out, text_v.str_size + 1, "%s\n", text_v.str);
	
	//~ printf("out: %s\n", out);
	
	sendto(sockfd, (const char*) out, text_v.str_size + 1, 0, (const struct sockaddr*) &client_addr, addr_len);
	
	releaseObjectVar(app_context, &text_v);
	
	RETURN_VOID();
}

void recompJSON(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	RETURN_VOID();
}

//~ void recompJSON_loadFile(SWFAppContext* app_context, ASObject* this, u32 num_args)
//~ {
	//~ DISCARD_ARGS(num_args);
	
	//~ printf("hi from the actual AS2 runtime\n");
	
	//~ FILE* docf = fopen("test.json", "rb");
	//~ fseek(docf, 0, SEEK_END);
	//~ size_t len = ftell(docf);
	//~ fseek(docf, 0, SEEK_SET);
	
	//~ char text[1024];
	//~ fread(text, 1, len, docf);
	//~ text[len] = '\0';
	//~ fclose(docf);
	
	//~ printf("%s\n", text);
	
	//~ yyjson_doc* doc = yyjson_read(text, len, 0);
	//~ yyjson_val* root = yyjson_doc_get_root(doc);
	//~ yyjson_val* test = yyjson_obj_get(root, "test");
	//~ printf("test: %s\n", yyjson_get_str(test));
	
	//~ RETURN_VOID();
//~ }