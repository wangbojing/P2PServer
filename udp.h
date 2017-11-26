
/*
 * Author: WangBoJing
 * email: 1989wangbojing@gmail.com 
 * github: https://github.com/wangbojing
 */


#ifndef __UDP_H__
#define __UDP_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <time.h>

typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;
typedef volatile long UATOMIC;
typedef void* (*KING_CALLBACK)(void *arg);

typedef enum {
	KING_RESULT_FAILED = -1,
	KING_RESULT_SUCCESS = 0,
} KING_RESULT;

typedef enum {
	KING_STATUS_NULL,
	KING_STATUS_LOGIN,
	KING_STATUS_HEARTBEAT,
	KING_STATUS_CONNECT,
	KING_STATUS_MESSAGE,
	KING_STATUS_NOTIFY,
	KING_STATUS_P2P_CONNECT,
	KING_STATUS_P2P_MESSAGE,
} KING_STATUS_SET;

#define KING_CLIENT_MAX				1024
#define KING_CLIENT_ADDR_LENGTH		6


#define KING_BUFFER_LENGTH		512

#define KING_NUMBER_ID_LENGTH			4

typedef struct _CLIENT_TABLE {
	U8 addr[KING_CLIENT_ADDR_LENGTH]; 
	U32 client_id;
	long stamp;
} client_table;



/**************************** status define ****************************/


#define KING_PROTO_LOGIN_REQ				0x01
#define KING_PROTO_LOGIN_ACK				0x81


#define KING_PROTO_HEARTBEAT_REQ			0x02
#define KING_PROTO_HEARTBEAT_ACK			0x82


#define KING_PROTO_CONNECT_REQ				0x11
#define KING_PROTO_CONNECT_ACK				0x91

#define NTY_PROTO_NOTIFY_REQ				0x12
#define NTY_PROTO_NOTIFY_ACK				0x92

#define NTY_PROTO_P2P_CONNECT_REQ			0x13
#define NTY_PROTO_P2P_CONNECT_ACK			0x93

#define NTY_RPORO_MESSAGE_REQ				0x21
#define NTY_RPORO_MESSAGE_ACK				0xA1


/**************************** context define ****************************/

#define KING_PROTO_BUFFER_VERSION_IDX		0
#define KING_PROTO_BUFFER_STATUS_IDX		1

#define KING_PROTO_BUFFER_LENGTH_IDX		(KING_PROTO_BUFFER_STATUS_IDX+1)
#define KING_PROTO_BUFFER_SELFID_IDX		(KING_PROTO_BUFFER_LENGTH_IDX+2)

//login
#define KING_PROTO_LOGIN_SELFID_IDX			KING_PROTO_BUFFER_SELFID_IDX 

//heartbeat
#define KING_PROTO_HEARTBEAT_SELFID_IDX		KING_PROTO_BUFFER_SELFID_IDX

//connect
#define KING_PROTO_CONNECT_SELFID_IDX		KING_PROTO_BUFFER_SELFID_IDX
#define KING_PROTO_CONNECT_OTHERID_IDX		(KING_PROTO_BUFFER_SELFID_IDX+KING_NUMBER_ID_LENGTH)

//notify
#define KING_PROTO_NOTIFY_SELFID_IDX			KING_PROTO_BUFFER_SELFID_IDX
#define KING_PROTO_NOTIFY_ADDR_IDX			(KING_PROTO_BUFFER_SELFID_IDX+KING_NUMBER_ID_LENGTH)

//p2p connect
#define KING_PROTO_P2P_CONNECT_SELFID_IDX	KING_PROTO_BUFFER_SELFID_IDX

//p2p connect ack
#define KING_PROTO_P2P_CONNECT_ACK_SELFID_IDX	KING_PROTO_BUFFER_SELFID_IDX


//message
#define KING_RPORO_MESSAGE_SELFID_IDX		KING_PROTO_BUFFER_SELFID_IDX
#define KING_PROTO_MESSAGE_OTHERID_IDX		(KING_RPORO_MESSAGE_SELFID_IDX+KING_NUMBER_ID_LENGTH)
#define KING_RPORO_MESSAGE_CONTENT_IDX		(KING_PROTO_MESSAGE_OTHERID_IDX+KING_NUMBER_ID_LENGTH)

//message ack
#define KING_RPORO_MESSAGE_ACK_SELFID_IDX	KING_PROTO_BUFFER_SELFID_IDX




static unsigned long cmpxchg(UATOMIC *addr, unsigned long _old, unsigned long _new) {
	U8 res;

	__asm__ volatile (
        "lock; cmpxchg %3, %1;sete %0;"
        : "=a" (res)
        : "m" (*addr), "a" (_old), "r" (_new)
        : "cc", "memory");

	return res;
}



static long time_genrator(void) {
	static long lTimeStamp = 0;
	static long timeStampMutex = 0;

	if(cmpxchg(&timeStampMutex, 0, 1)) {
		lTimeStamp = time(NULL);
		timeStampMutex = 0;
	}

	return lTimeStamp;
}



static int addr_to_array(U8 *array, struct sockaddr_in *p_addr) {
	int i = 0;

	for (i = 0;i < 4;i ++) {
		array[i] = *((unsigned char*)(&p_addr->sin_addr.s_addr) + i);
	}

	for (i = 0;i < 2;i ++) {
		array[4+i] = *((unsigned char*)(&p_addr->sin_port)+i);
	}
}

static int array_to_addr(U8 *array, struct sockaddr_in *p_addr) {
	int i = 0;
	
	for (i = 0;i < 4;i ++) {
		*((unsigned char*)(&p_addr->sin_addr.s_addr) + i) = array[i];
	}
	for (i = 0;i < 2;i ++) {
		*((unsigned char*)(&p_addr->sin_port)+i) = array[4+i];
	}
}





static int king_send_login(int sockfd, int self_id, struct sockaddr_in *paddr) {

	U8 buffer[KING_BUFFER_LENGTH] = {0};

	buffer[KING_PROTO_BUFFER_STATUS_IDX] = KING_PROTO_LOGIN_REQ;
	*(int *)(buffer+KING_PROTO_LOGIN_SELFID_IDX) = self_id;

	int n = KING_PROTO_LOGIN_SELFID_IDX + KING_NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	
	return n;

}

static int king_send_heartbeat(int sockfd, int self_id, struct sockaddr_in *paddr) {
	
	U8 buffer[KING_BUFFER_LENGTH] = {0};

	buffer[KING_PROTO_BUFFER_STATUS_IDX] = KING_PROTO_HEARTBEAT_REQ;
	*(int *)(buffer+KING_PROTO_HEARTBEAT_SELFID_IDX) = self_id;

	int n = KING_PROTO_HEARTBEAT_SELFID_IDX + KING_NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	
	return n;
}

static int king_send_connect(int sockfd, int self_id, int other_id, struct sockaddr_in *paddr) {
	
	U8 buffer[KING_BUFFER_LENGTH] = {0};

	buffer[KING_PROTO_BUFFER_STATUS_IDX] = KING_PROTO_CONNECT_REQ;
	*(int *)(buffer+KING_PROTO_CONNECT_SELFID_IDX) = self_id;
	*(int *)(buffer+KING_PROTO_CONNECT_OTHERID_IDX) = other_id;

	int n = KING_PROTO_CONNECT_OTHERID_IDX + KING_NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	
	return n;	
}

static int king_send_p2pconnect(int sockfd, int self_id, struct sockaddr_in *paddr) {

	U8 buffer[KING_BUFFER_LENGTH] = {0};
	
	buffer[KING_PROTO_BUFFER_STATUS_IDX] = NTY_PROTO_P2P_CONNECT_REQ;
	*(int *)(buffer+KING_PROTO_P2P_CONNECT_SELFID_IDX) = self_id;

	int n = KING_PROTO_P2P_CONNECT_SELFID_IDX + KING_NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	
	return n;

}

static int king_send_p2pconnectack(int sockfd, int self_id, struct sockaddr_in *paddr) {

	U8 buffer[KING_BUFFER_LENGTH] = {0};
	
	buffer[KING_PROTO_BUFFER_STATUS_IDX] = NTY_PROTO_P2P_CONNECT_ACK;
	*(int *)(buffer+KING_PROTO_P2P_CONNECT_ACK_SELFID_IDX) = self_id;

	int n = KING_PROTO_P2P_CONNECT_ACK_SELFID_IDX + KING_NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	
	return n;

}


static int king_client_send_message(int sockfd, int self_id, int other_id, struct sockaddr_in *paddr, U8 *msg, int length) {
	U8 buffer[KING_BUFFER_LENGTH] = {0};

	buffer[KING_PROTO_BUFFER_STATUS_IDX] = NTY_RPORO_MESSAGE_REQ; 
	*(int *)(buffer+KING_RPORO_MESSAGE_SELFID_IDX) = self_id;
	*(int *)(buffer+KING_PROTO_MESSAGE_OTHERID_IDX) = other_id;

	memcpy(buffer+KING_RPORO_MESSAGE_CONTENT_IDX, msg, length);

	int n = KING_RPORO_MESSAGE_CONTENT_IDX + length;
	*(U16*)(buffer+KING_PROTO_BUFFER_LENGTH_IDX) = (U16) n;
	
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	return n;
}

static int king_send_messageack(int sockfd, int self_id, struct sockaddr_in *paddr) {

	U8 buffer[KING_BUFFER_LENGTH] = {0};
	
	buffer[KING_PROTO_BUFFER_STATUS_IDX] = NTY_RPORO_MESSAGE_ACK;
	*(int *)(buffer+KING_RPORO_MESSAGE_ACK_SELFID_IDX) = self_id;

	int n = KING_RPORO_MESSAGE_ACK_SELFID_IDX + KING_NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	
	return n;

}




client_table table[KING_CLIENT_MAX] = {0};
int client_count = 0;


static int get_index_by_clientid(int client_id) {
	int i = 0;

	int now_count = client_count;
	for (i = 0;i < now_count;i ++) {
		if (table[i].client_id == client_id) return i;
	}
}


static int king_send_message(int sockfd, int client_id, U8 *buffer, int length) {

	int index = get_index_by_clientid(client_id);
	struct sockaddr_in c_addr;
	c_addr.sin_family = AF_INET;
	array_to_addr(table[index].addr, &c_addr);

	int n = sendto(sockfd, buffer, length, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));
	if (n < 0) {
		perror("sendto");
	}
	return n;
}


static int king_send_notify(int sockfd, int client_id, int self_id) {

	U8 buffer[KING_BUFFER_LENGTH] = {0};

	int index = get_index_by_clientid(self_id);

	buffer[KING_PROTO_BUFFER_STATUS_IDX] = NTY_PROTO_NOTIFY_REQ;
	*(int*)(buffer+KING_PROTO_NOTIFY_SELFID_IDX) = self_id;
	memcpy(buffer+KING_PROTO_NOTIFY_ADDR_IDX, table[index].addr, KING_CLIENT_ADDR_LENGTH);

	index = get_index_by_clientid(client_id);
	struct sockaddr_in c_addr;
	c_addr.sin_family = AF_INET;
	array_to_addr(table[index].addr, &c_addr);

	int n = KING_PROTO_NOTIFY_ADDR_IDX + KING_CLIENT_ADDR_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));
	if (n < 0) {
		perror("sendto");
	}

	return n;
}





#endif





