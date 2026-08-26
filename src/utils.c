#include <string.h>

#include <swf.h>
#include <heap.h>
#include <utils.h>

size_t get_power_two_size(size_t old_size, size_t size)
{
	while (old_size < size)
	{
		old_size <<= 1;
	}
	
	return old_size;
}

void grow_ptr(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	char* new_data = HALLOC(old_data_size << 1);
	
	memcpy(new_data, data, old_data_size);
	
	FREE(data);
	
	*ptr = new_data;
	*capacity_ptr = capacity << 1;
}

void grow_ptr_far(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size, size_t new_size)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	size_t new_capacity = get_power_two_size(capacity, new_size);
	size_t new_data_size = new_capacity*elem_size;
	
	char* new_data = HALLOC(new_data_size);
	
	memcpy(new_data, data, old_data_size);
	
	FREE(data);
	
	*ptr = new_data;
	*capacity_ptr = new_capacity;
}

#if defined(_MSC_VER)
// Microsoft

#include <windows.h>
#include <process.h>
#include <Winbase.h>

#include <dwmapi.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")

#pragma comment(lib, "ws2_32.lib")

// windows-only machine-specific global:
LARGE_INTEGER counter_frequency;

void recomp_init_utils(SWFAppContext* app_context)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	QueryPerformanceFrequency(&counter_frequency);
	
	timeBeginPeriod(1);
}

void recomp_sync_window(SWFAppContext* app_context)
{
	DwmFlush();
}

void recomp_deinit_utils(SWFAppContext* app_context)
{
	timeEndPeriod(1);
}

u32 get_elapsed_ms()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	
	u32 time = (u32) (1000*counter.QuadPart/counter_frequency.QuadPart);
	
	return time;
}

void recomp_sleep(u32 ms)
{
	Sleep(ms);
}

int getpagesize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	
	return si.dwPageSize;
}

char* vmem_reserve(size_t size)
{
	return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void vmem_release(char* addr, size_t size)
{
	VirtualFree(addr, 0, MEM_RELEASE);
}

void thread_start(SWFAppContext* app_context, runtime_thread_func f, recomp_thread_t* handle)
{
	*handle = _beginthreadex(NULL, 0, f, app_context, 0, NULL);
}

void thread_exit()
{
	_endthreadex(0);
}

void thread_join(recomp_thread_t* handle)
{
	WaitForSingleObject((HANDLE) *handle, INFINITE);
	CloseHandle((HANDLE) *handle);
}

void rwlock_init(recomp_rwlock_t* rwlock)
{
	InitializeSRWLock((PSRWLOCK) rwlock);
}

void rwlock_lock_read(recomp_rwlock_t* rwlock)
{
	AcquireSRWLockShared((PSRWLOCK) rwlock);
}

void rwlock_unlock_read(recomp_rwlock_t* rwlock)
{
	ReleaseSRWLockShared((PSRWLOCK) rwlock);
}

void rwlock_lock_write(recomp_rwlock_t* rwlock)
{
	AcquireSRWLockExclusive((PSRWLOCK) rwlock);
}

void rwlock_unlock_write(recomp_rwlock_t* rwlock)
{
	ReleaseSRWLockExclusive((PSRWLOCK) rwlock);
}

void rwlock_destroy(recomp_rwlock_t* rwlock)
{
	
}

int addr_len = sizeof(struct sockaddr_in);

SOCKET udp_sockfd;
SOCKET tcp_sockfd;

struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

void sitl_init(u16 port)
{
	udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	u_long nonblock_mode = 1;
	ioctlsocket(udp_sockfd, FIONBIO, &nonblock_mode);
	
	memset(&server_addr, 0, addr_len);
	memset(&client_addr, 0, addr_len);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9002);
	
	bind(udp_sockfd, (SOCKADDR*) &server_addr, addr_len);
	
	tcp_sockfd = -1;
}

int sitl_udp_recv(char* data, size_t data_size)
{
	return recvfrom(udp_sockfd, (char*) data, (int) data_size, 0, (SOCKADDR*) &client_addr, &addr_len);
}

void sitl_tcp_init(u16 port)
{
	if (tcp_sockfd == -1)
	{
		tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		
		memset(&server_addr, 0, addr_len);
		
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		
		memcpy(&server_addr.sin_addr, &client_addr.sin_addr, sizeof(client_addr.sin_addr));
		
		connect(tcp_sockfd, (SOCKADDR*) &server_addr, sizeof(server_addr));
		
		u_long nonblock_mode = 1;
		ioctlsocket(tcp_sockfd, FIONBIO, &nonblock_mode);
	}
}

void sitl_send_json(char* data, size_t data_size)
{
	sendto(udp_sockfd, (const char*) data, (int) data_size, 0, (SOCKADDR*) &client_addr, addr_len);
}

int sitl_tcp_read(char* out, size_t out_size)
{
	return (int) recv(tcp_sockfd, out, (int) out_size, 0);
}

void sitl_tcp_write(char* data, size_t data_size)
{
	send(tcp_sockfd, data, (int) data_size, 0);
}

#elif defined(__GNUC__)
// GCC

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <arpa/inet.h>

void recomp_handle_exit(int sig)
{
	signaled = 1;
}

void recomp_init_utils(SWFAppContext* app_context)
{
	struct sigaction s;
	s.sa_handler = recomp_handle_exit;
	sigemptyset(&s.sa_mask);
	s.sa_flags = 0;
	sigaction(SIGINT, &s, NULL);
}

extern int tcp_sockfd;

void recomp_sync_window(SWFAppContext* app_context)
{
	if (signaled)
	{
		shutdown(tcp_sockfd, SHUT_WR);
		
		char buf[128];
		while (read(tcp_sockfd, buf, 128) > 0);
		
		close(tcp_sockfd);
		
		signaled_quit = true;
	}
}

void recomp_deinit_utils(SWFAppContext* app_context)
{
	
}

u32 get_elapsed_ms()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return (now.tv_sec)*1000 + (now.tv_nsec)/1000000;
}

void recomp_sleep(u32 ms)
{
	struct timespec ms_ts;
	ms_ts.tv_sec = ms/1000;
	ms_ts.tv_nsec = (ms % 1000)*1000000;
	nanosleep(&ms_ts, NULL);
}

char* vmem_reserve(size_t size)
{
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

void vmem_release(char* addr, size_t size)
{
	munmap(addr, size);
}

void thread_start(SWFAppContext* app_context, runtime_thread_func f, recomp_thread_t* handle)
{
	pthread_create(handle, NULL, (void* (*)(void*)) f, app_context);
}

void thread_exit()
{
	
}

void thread_join(recomp_thread_t* handle)
{
	pthread_join(*handle, NULL);
}

void rwlock_init(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_init(rwlock, NULL);
}

void rwlock_lock_read(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_rdlock(rwlock);
}

void rwlock_unlock_read(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_unlock(rwlock);
}

void rwlock_lock_write(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_wrlock(rwlock);
}

void rwlock_unlock_write(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_unlock(rwlock);
}

void rwlock_destroy(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_destroy(rwlock);
}

socklen_t addr_len = sizeof(struct sockaddr_in);

int udp_sockfd;
int tcp_sockfd;

struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

void sitl_init(u16 port)
{
	udp_sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	memset(&server_addr, 0, addr_len);
	memset(&client_addr, 0, addr_len);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9002);
	
	bind(udp_sockfd, (const struct sockaddr*) &server_addr, addr_len);
	
	tcp_sockfd = -1;
}

int sitl_udp_recv(char* data, size_t data_size)
{
	return recvfrom(udp_sockfd, (char*) data, data_size, 0, (struct sockaddr*) &client_addr, &addr_len);
}

void sitl_tcp_init(u16 port)
{
	if (tcp_sockfd == -1)
	{
		tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		
		memset(&server_addr, 0, addr_len);
		
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		
		memcpy(&server_addr.sin_addr, &client_addr.sin_addr, sizeof(client_addr.sin_addr));
		
		connect(tcp_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
		
		int prev = fcntl(tcp_sockfd, F_GETFL, 0);
		fcntl(tcp_sockfd, F_SETFL, prev | O_NONBLOCK);
	}
}

void sitl_send_json(char* data, size_t data_size)
{
	sendto(udp_sockfd, (const char*) data, data_size, 0, (const struct sockaddr*) &client_addr, addr_len);
}

int sitl_tcp_read(char* out, size_t out_size)
{
	return (int) read(tcp_sockfd, out, out_size);
}

void sitl_tcp_write(char* data, size_t data_size)
{
	write(tcp_sockfd, data, data_size);
}

#endif