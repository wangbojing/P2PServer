
/*
 * Author: WangBoJing
 * email: 1989wangbojing@gmail.com 
 * github: https://github.com/wangbojing
 */

#include "udp.h"
#include <pthread.h>


static int status_machine = KING_STATUS_LOGIN;
static int client_selfid = 0x0;

struct sockaddr_in server_addr;

client_table p2p_clients[KING_CLIENT_MAX] = {0};
static int p2p_count = 0;

static int king_client_buffer_parser(int sockfd, U8 *buffer, U32 length, struct sockaddr_in *addr) {

	U8 status = buffer[KING_PROTO_BUFFER_STATUS_IDX];

	switch (status) {
		case NTY_PROTO_NOTIFY_REQ: {
			struct sockaddr_in other_addr;
			other_addr.sin_family = AF_INET;
			array_to_addr(buffer+KING_PROTO_NOTIFY_ADDR_IDX, &other_addr);

			king_send_p2pconnect(sockfd, client_selfid, &other_addr);
			break;
		}
		case NTY_PROTO_P2P_CONNECT_REQ: {
			int now_count = p2p_count++;

			p2p_clients[now_count].stamp = time_genrator();
			p2p_clients[now_count].client_id = *(int*)(buffer+KING_PROTO_P2P_CONNECT_SELFID_IDX);
			addr_to_array(p2p_clients[now_count].addr, addr);
		
			king_send_p2pconnectack(sockfd, client_selfid, addr);
			printf("Enter P2P Model\n");
			status_machine = KING_STATUS_P2P_MESSAGE;

			break;
		}
		case NTY_PROTO_P2P_CONNECT_ACK: {
			int now_count = p2p_count++;
			
			p2p_clients[now_count].stamp = time_genrator();
			p2p_clients[now_count].client_id = *(int*)(buffer+KING_PROTO_P2P_CONNECT_SELFID_IDX);
			addr_to_array(p2p_clients[now_count].addr, addr);
			
			printf("Enter P2P Model\n");
			status_machine = KING_STATUS_P2P_MESSAGE;
			break;
		}
		case NTY_RPORO_MESSAGE_REQ: {

			U8 *msg = buffer+KING_RPORO_MESSAGE_CONTENT_IDX;
			U32 other_id = *(U32*)(buffer+KING_RPORO_MESSAGE_SELFID_IDX);
			
			printf(" from client:%d --> %s\n", other_id, msg);

			king_send_messageack(sockfd, client_selfid, addr);
			//status_machine = KING_STATUS_P2P_MESSAGE;
			
			break;
		}
		case KING_PROTO_LOGIN_ACK: {
			printf(" Connect Server Success\nPlease Enter Message : ");
			status_machine = KING_STATUS_MESSAGE;
			break;
		}
		case KING_PROTO_HEARTBEAT_ACK:
		case KING_PROTO_CONNECT_ACK:
		case NTY_PROTO_NOTIFY_ACK:
			break;
		case NTY_RPORO_MESSAGE_ACK:
			break;
	}
	
}


void* king_recv_callback(void *arg) {

	int sockfd = *(int *)arg;
	struct sockaddr_in addr;
	int length = sizeof(struct sockaddr_in);
	U8 buffer[KING_BUFFER_LENGTH] = {0};

	//printf("king_recv_callback --> enter\n");
	
	while (1) {
		int n = recvfrom(sockfd, buffer, KING_BUFFER_LENGTH, 0, (struct sockaddr*)&addr, &length);
		if (n > 0) {
			buffer[n] = 0;

			king_client_buffer_parser(sockfd, buffer, n, &addr);
		} else if (n == 0) {
			printf("server closed\n");
			close(sockfd);
			break;
		} else if (n == -1) {
			perror("recvfrom");
			close(sockfd);
			break;
		}
	}
}


void *king_send_callback(void *arg) {

	int sockfd = *(int *)arg;
	char buffer[KING_BUFFER_LENGTH] = {0};

	//printf("king_send_callback --> enter\n");
	
	while (1) {

		bzero(buffer, KING_BUFFER_LENGTH);
		
		scanf("%s", buffer);
		//getchar();

		if (status_machine == KING_STATUS_MESSAGE) {

			
			printf(" --> please enter bt : ");
			
			int other_id = buffer[1]-0x30;

			if (buffer[0] == 'C') {
				//printf("king_send_connect");
				king_send_connect(sockfd, client_selfid, other_id, &server_addr);
			} else {
				int length = strlen(buffer);
				king_client_send_message(sockfd, client_selfid, other_id, &server_addr, buffer, length);
			}
		
		} else if (status_machine == KING_STATUS_P2P_MESSAGE) {

			printf(" --> please enter message to send : ");
			
			int now_count = p2p_count;
			struct sockaddr_in c_addr;
			c_addr.sin_family = AF_INET;
			array_to_addr(p2p_clients[now_count-1].addr, &c_addr);

 			int length = strlen(buffer);

			king_client_send_message(sockfd, client_selfid, 0, &c_addr, buffer, length);
		}

	}
}


int main(int argc, char *argv[]) {

	printf(" This is a UDP Client\n");

	if (argc != 4) {
		printf("Usage: %s ip port\n", argv[0]);
		exit(1);
	}

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(1);
	}


	pthread_t thread_id[2] = {0};
	KING_CALLBACK cb[2] = {king_send_callback, king_recv_callback};
	int i = 0;

	for (i = 0;i < 2;i ++) {
		int ret = pthread_create(&thread_id[i], NULL, cb[i], &sockfd);
		if (ret) {
			perror("pthread_create");
			exit(1);
		}
		sleep(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	
	client_selfid = atoi(argv[3]);

	king_send_login(sockfd, client_selfid, &server_addr);

	for (i = 0;i < 2;i ++) {
		pthread_join(thread_id[i], NULL);
	}
	
	return 0;
}




