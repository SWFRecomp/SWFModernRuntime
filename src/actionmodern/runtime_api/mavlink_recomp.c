#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include <arpa/inet.h>

#include <mavlink/common/mavlink.h>
#include <yyjson.h>

#include <objects.h>
#include <flashbang.h>
#include <mavlink_recomp.h>

#include <initial_strings_decls.h>

#define BUFFER_SIZE 1024

socklen_t addr_len = sizeof(struct sockaddr_in);

int udp_sockfd;
int tcp_sockfd;

struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

bool got_target;
u8 target_system;
u8 target_component;

void recompSITLInit(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	udp_sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	memset(&server_addr, 0, addr_len);
	memset(&client_addr, 0, addr_len);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9002);
	
	bind(udp_sockfd, (const struct sockaddr*) &server_addr, addr_len);
	
	tcp_sockfd = -1;
	got_target = false;
	
	RETURN_VOID();
}

void recompSITLReadPacket(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar out_obj_v;
	popVar(app_context, &out_obj_v);
	
	DISCARD_ARGS(num_args - 1);
	
	char data[BUFFER_SIZE];
	
	int bytes_read = recvfrom(udp_sockfd, (char*) data, BUFFER_SIZE - 1, 0, (struct sockaddr*) &client_addr, &addr_len);
	
	if (bytes_read != -1)
	{
		if (tcp_sockfd == -1)
		{
			tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
			
			memset(&server_addr, 0, addr_len);
			
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(5762);
			
			memcpy(&server_addr.sin_addr, &client_addr.sin_addr, sizeof(client_addr.sin_addr));
			
			connect(tcp_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
		}
		
		u16 magic = VAL(u16, &data[0]);
		u16 frame_rate = VAL(u16, &data[2]);
		u32 frame_count = VAL(u32, &data[4]);
		u16* pwm = (u16*) &data[8];
		
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
	ActionVar time_v;
	popVar(app_context, &time_v);
	
	DISCARD_ARGS(num_args - 1);
	
	convertNumericToNumber(app_context, &time_v);
	f64 time = time_v.f64;
	
	char out[1024];
	int length = snprintf(out, 1024, "{\"timestamp\":%f,\"imu\":{\"gyro\":[0,0,0],\"accel_body\":[0,0,0]},\"position\":[0,0,0],\"attitude\":[0,0,0],\"velocity\":[0,0,0]}", time);
	
	sendto(udp_sockfd, (const char*) out, length + 1, 0, (const struct sockaddr*) &client_addr, addr_len);
	
	releaseObjectVar(app_context, &time_v);
	
	RETURN_VOID();
}

void recompSITLSendSensor(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar range_v;
	popVar(app_context, &range_v);
	
	ActionVar orientation_v;
	popVar(app_context, &orientation_v);
	
	DISCARD_ARGS(num_args - 2);
	
	convertNumericToNumber(app_context, &range_v);
	f64 range = range_v.f64;
	
	convertNumericToInteger(app_context, &orientation_v);
	s32 orientation = orientation_v.s32;
	
	u8 b;
	mavlink_message_t m;
	mavlink_status_t s;
	
	if (!got_target)
	{
		int prev = fcntl(tcp_sockfd, F_GETFL, 0);
		fcntl(tcp_sockfd, F_SETFL, prev | O_NONBLOCK);
		
		if (read(tcp_sockfd, &b, 1) > 0)
		{
			if (mavlink_parse_char(MAVLINK_COMM_0, b, &m, &s))
			{
				if (m.msgid != MAVLINK_MSG_ID_HEARTBEAT)
				{
					goto release;
				}
				
				mavlink_heartbeat_t h;
				mavlink_msg_heartbeat_decode(&m, &h);
				
				if (h.autopilot != MAV_AUTOPILOT_INVALID)
				{
					goto release;
				}
				
				target_system = m.sysid;
				target_component = m.compid;
				got_target = true;
			}
		}
		
		else
		{
			goto release;
		}
	}
	
	mavlink_message_t out;
	
	mavlink_msg_distance_sensor_pack(target_system, target_component, &out, get_elapsed_ms(), 0, 1000, (u16) range, MAV_DISTANCE_SENSOR_LASER, 1, orientation, 0, 0, 0, 0, 100);
	
	u8 out_buffer[MAVLINK_MAX_PACKET_LEN];
	u16 length = mavlink_msg_to_send_buffer(out_buffer, &out);
	
	write(tcp_sockfd, out_buffer, length);
	
release:
	releaseObjectVar(app_context, &orientation_v);
	releaseObjectVar(app_context, &range_v);
	
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