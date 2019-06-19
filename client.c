#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>


#define SIZE	1024

pthread_t main_thread;
pthread_t get_thread[SIZE];

typedef struct str_msg
{
	char message[SIZE];
} msg;

int port;
char *serverIP;

void socket_connect ();
void socket_connect_port (void *ptr);
void cmd(int sid, char *str, char *r_str);
void get (int sid, char *str, char *r_str);
void get_extend (int sid, char *str, char *r_str);
void quit (int sid, char *str);
int file_curr(char **mdict, char *fname);
pthread_t main_thread;
pthread_t get_thread[SIZE];
int get_id = 0;
char *get_fdict[SIZE];
int get_sdict[SIZE] = {0};
int g_id[SIZE] = {0};
char *get_mutex_dict[SIZE]={NULL};

int main (int argc, char **argv) {

	//  specify port number 
	if (argc != 3) {
		printf("Please enter the server address and a port numbers.\n");
	} else {
		serverIP = argv[1];
		port = atoi(argv[2]);
		pthread_create(&main_thread, NULL, (void *) &socket_connect, NULL);
		pthread_join(main_thread, NULL);	
	}
	return 0;
}

void socket_connect () {
	int sockid, select;
	char reply[SIZE];
	char buf[SIZE];
	char id_string[10];
	struct sockaddr_in server;
	msg data;
	
	while (1) {
		bzero(buf, sizeof(buf));
		bzero(data.message, sizeof(data.message));
		bzero(id_string, sizeof(id_string));
		
		printf("\nmishaalserver> ");
		fgets(buf, sizeof(buf), stdin);
		
		strcpy(data.message, buf);
		bzero(reply, sizeof(reply));
	
		if (strncmp(buf, "&get", 4) == 0) {
			pthread_create(&get_thread[get_id], NULL, (void *) &socket_connect_port, (void *) &data);
			sleep(1);
		} 			
	         else {
			// set up socket
			sockid = socket(AF_INET, SOCK_STREAM, 0);
			if (sockid < 0) {
				printf("Cannot create socket.\n");
				exit(errno);
			}
			
			bzero(&server, sizeof(server));
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = inet_addr(serverIP);
			server.sin_port = htons(port);
			
			if (connect(sockid, (struct sockaddr *)&server, sizeof(server)) < 0) {
				printf("Cannot connect to the server.\n");
				exit(errno);
			}
		
			if (strncmp(buf, "get", 3) == 0)
				select = 1;	// get command

			else if (strncmp(buf, "quit", 4) == 0)
				select = 2;	// quit command
			else 
				select = 0;	// rm, ls, cd, mkdir, pwd and other invalid command	




			switch(select) {
				case 1:
					get (sockid, buf, reply);
					break;
				case 2:
					quit (sockid, buf);
					break;
				default:
					cmd (sockid, buf, reply);
			}
		}
	}
}

void socket_connect_port (void *ptr) {
	msg *data;
	data = (msg *) ptr;
	
	int sockid;
	int select;
	char reply[SIZE];
	struct sockaddr_in server;
	
	// set up socket
	sockid = socket(AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		printf("Cannot create socket.\n");
		exit(errno);
	}
	
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(serverIP);
	server.sin_port = htons(port);
	
	if (connect(sockid, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("server connection failed.\n");
		exit(errno);
	}

	if (strncmp(data->message, "&get", 4) == 0)
		select = 1;	// &get command
	
	bzero(reply, sizeof(reply));
			
	switch(select) {
		case 1:
			get_extend (sockid, data->message, reply);
			break;
	}
	
	pthread_exit(NULL);
}


void cmd (int sid, char *str, char *r_str) {
	write(sid, str, strlen(str));
	read(sid, r_str, SIZE);
	printf("%s", r_str);	
	close(sid);
}

void get (int sid, char *str, char *r_str) {
	char *nofile = "No such file";
	char *ferror = "Cannot write the file.\n";
	char *signal = "signal";
	char f[100];
	int i;
	
	bzero(f, sizeof(f));
	
	// get the file name
	for (i = 4; i < strlen(str) - 1; i++) {
		f[i-4] = str[i];
	}
	
	write(sid, str, strlen(str));		// write the command to the server
	read(sid, r_str, SIZE);				
	
	if (strncmp(r_str, nofile, 12) != 0) {	
		int c_id = atoi(r_str);
		bzero(r_str, strlen(r_str));
		write(sid, signal, strlen(signal));	// send a signal to server to start getting file
		
		FILE *file = fopen(f, "wb");	
		
		if (file == NULL) {
			printf("%s", ferror);
			close(sid);
			return;
		}
		
		while (read(sid, r_str, 1000) > 0) {
			fwrite(r_str, sizeof(char), strlen(r_str), file);
			if(strlen(r_str) < 1000) {
				printf("All bytes of %s has been received.");
			} else {
				printf("1000 bytes of %s has been received.");
			}
			bzero(r_str, strlen(r_str));
		}
		
		fclose(file);
		printf("Get file %s from server!\n", f);
	} else {
		printf("%s: %s", r_str, f);
	}
	close(sid);
}

void get_extend (int sid, char *str, char *r_str) {
	int get_id_tmp = get_id;
	g_id[get_id_tmp] = 1;
	get_id++;
	
	char *nofile = "No such file found";
	char *ferror = "Cannot write the file.\n";
	char *signal = "signal";
	char buf[100];
	char f[100];
	int i;
	
	bzero(buf, sizeof(buf));
	bzero(f, sizeof(f));
	
	// get the file name
	for (i = 5; i < strlen(str) - 1; i++) {
		f[i-5] = str[i];
	}
	
	// set get command without & sign to send
	for (i = 1; i < strlen(str); i++) {
		buf[i-1] = str[i];
	}
	
	write(sid, buf, strlen(buf));		// write the command to the server
	read(sid, r_str, SIZE);				
	
	if (strncmp(r_str, nofile, 12) != 0) {
		
		int c_id = atoi(r_str);
		get_sdict[get_id_tmp] = c_id;
		
		bzero(r_str, strlen(r_str));
				
		get_fdict[get_id_tmp] = f;
		int ret = file_curr(get_mutex_dict, f);
		
		while(ret == 1){
			ret = file_curr(get_mutex_dict, f);
		}
		
		get_mutex_dict[get_id_tmp] = f;
		
		FILE *file = fopen(f, "wb");	
		if (file == NULL) {
			printf("%s", ferror);
			close(sid);
			g_id[get_id_tmp] = 0;
			return;
		}
		
		write(sid, signal, strlen(signal));	// send a signal to server to start getting file
		while (read(sid, r_str, 1000) > 0) {
			fwrite(r_str, sizeof(char), strlen(r_str), file);
			bzero(r_str, strlen(r_str));
		}
		
		fclose(file);
		get_mutex_dict[get_id_tmp] = NULL;	
		
	} else {
		printf("%s: %s", r_str, f);
	}
	close(sid);
	g_id[get_id_tmp] = 0;
}


void quit (int sid, char *str) {
	write(sid, str, strlen(str));
	printf("session ended.\n");
	exit(1);
}


int file_curr(char **mdict, char *fname){
	int i = 0;
	for(i = 0; i < SIZE-1; i++){
		if((mdict[i] != NULL) && (strcmp(mdict[i], fname) == 0)){
			return 1;
		}
	}
	return 0;
}