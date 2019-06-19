#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#define SIZE	1024
   
void socket_connect_port (int port);
void thread_process (int socket);
void cmd(int sid, char *str);
void get (int sid, char *str);
void rm (int sid, char *str);
void ls (int sid, char *str);
void cd (int sid, char *str);
void s_mkdir (int sid, char *str);
void pwd (int sid);
void mv(int sid, char *str);
void cp(int sid, char *str);
void quit ();
int file_curr(char **mdict, char *fname);
pthread_t n_thread[SIZE];

int t_id = 0;
char *fdict[SIZE];
int sdict[SIZE] = {0};
int g_id[SIZE] = {0};
char *mutex_dict[SIZE]={NULL};

int main (int argc, char **argv) {
	int port;
	pthread_t port_thread;
	
	
	// specify port number
	if (argc != 2) {
		printf("Please enter a port number.\n");
	} else {
		port = atoi(argv[1]);
		
		if (port > 1000 && port < 10000 ) {
			pthread_create(&port_thread, NULL, (void *) &socket_connect_port, (int *) port);
			pthread_join(port_thread, NULL);

			
		} else {
			printf("Please enter the port number between 1000 and 10000.\n");
		}
	}
	
	return 0;
}

void socket_connect_port (int p) {
	int sockid, s;
	char buf[SIZE];
	struct sockaddr_in server;
	
	printf("Server port no#: %d\n", p);
        printf("Connection made");
        printf("binding....");
		
	// set up socket
	sockid = socket(AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		printf("socket creation failed.\n");
		exit(errno);
	}
	
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(p);
	
	if (bind(sockid, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("binding failed.\n");
		exit(errno);
	}
	
	listen(sockid, SIZE);
	
	while ((s = accept(sockid, (struct sockaddr *) NULL, NULL)) > 0) {
		sdict[s] = t_id;
		pthread_create(&n_thread[t_id], NULL, (void *) &thread_process, (int *) s);
		t_id++;
	}
}


void thread_process (int socket) {
	char buf[SIZE];
	bzero(buf, SIZE);
	read(socket, buf, SIZE);		
	cmd(socket, buf);	
	
}

void cmd (int sid, char *str) {
	int select;
	char *e = "Invalid Command.\n\n";
	
	if (strncmp(str, "get", 3) == 0)
		select = 1;
	else if (strncmp(str, "rm", 2) == 0)
		select = 2;
	else if (strncmp(str, "ls", 2) == 0)
		select = 3;
	else if (strncmp(str, "cd", 2) == 0)
		select = 4;
	else if (strncmp(str, "mkdir", 5) == 0)
		select = 5;
	else if (strncmp(str, "pwd", 3) == 0)
		select = 6;
	else if (strncmp(str, "quit", 4) == 0)
		select = 7;
        else if (strncmp(str,"mv",  2) == 0)
                select=8;
        else if (strncmp(str,"cp", 2) == 0)
                select=9;
	else
		select = 0;

	switch(select) {
		case 1:
			get(sid, str);
			break;
		case 2:
			rm(sid, str);
			break;
		case 3:
			ls(sid, str);
			break;
		case 4:
			cd(sid, str);
			break;
		case 5:
			s_mkdir(sid, str);
			break;
		case 6:
			pwd(sid);
			break;
		case 7:
			quit();
			break;
                case 8:
                        mv(sid,str); 
                        break;
                case 9:
                       cp(sid,str);
                       break;
              
		default:
			printf("Invalid Command.\n\n");
			write(sid, e, strlen(e));
	}
}

void ls (int sid, char *str) {
	FILE * fp;
	char buffer[SIZE];
	char ret[SIZE] = "";

	fp = popen(str, "r");
	while(fgets(buffer,sizeof(buffer),fp) != NULL){
		strcat(ret, buffer);
	}
	write(sid, ret, sizeof(ret));
	pclose(fp);
}

void cd (int sid, char *str) {
	char f[100];
	int i;

	char *fail_cd = "Failed to change directory.\n";
	char *success_cd = "Directory changed successfully.\n";
	
	bzero(f, sizeof(f));
	for (i = 3; i < strlen(str)-1 ; i++) {
		f[i-3] = str[i];
	}
	if(chdir(f) != 0){
		printf("%s: %s\n\n", f, fail_cd);
		write(sid, fail_cd, strlen(fail_cd));
	}
	else{
		printf("%s: %s\n\n", f, success_cd);
		write(sid, success_cd, strlen(success_cd));
	}
}

void s_mkdir (int sid, char *str) {
	char f[100];
	int i;
	char *fail_mkdir = "Failed to create dir.\n";
	char *success_mkdir = "Dir successfully created.\n";
	
	bzero(f, sizeof(f));
	for (i = 6; i < strlen(str) ; i++) {
		f[i-6] = str[i];
	}
	if(mkdir(f, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH) < 0){
		printf("%s: %s\n\n", f, fail_mkdir);
		write(sid, fail_mkdir, strlen(fail_mkdir));
	}
	else{
		printf("%s: %s\n\n", f, success_mkdir);
		write(sid, success_mkdir, strlen(success_mkdir));
	}
}
void pwd (int sid) {
	FILE * fp;
	char buffer[SIZE];
	system("pwd");
	fp = popen("pwd", "r");
	fgets(buffer,sizeof(buffer),fp);
	write(sid, buffer, sizeof(buffer));
	pclose(fp);
}

void get (int sid, char *str) {
	char *ferror = "No such file";	// error message
	char buf[SIZE];
	char f[100];
	int i;
	char t_id_string[10];

	bzero(f, sizeof(f));
	bzero(buf, sizeof(buf));
	
	int tid = sdict[sid];
	sprintf(t_id_string, "%d", tid);
	g_id[tid] = 1;
	
	// get the file name
	for (i = 4; i < strlen(str)-1; i++) {
		f[i-4] = str[i];	
	}
	
	FILE *file = fopen(f, "rb");
	if (file == NULL) {
		printf("No such file: %s\n\n", f);
		write(sid, ferror, strlen(ferror));
		g_id[tid] = 0;
		return;
	}

	write(sid, t_id_string, strlen(t_id_string));
	
	int ret = file_curr(mutex_dict, f);
	while(ret == 1){
		ret = file_curr(mutex_dict, f);
	}

	read(sid, buf, sizeof(buf)); // read the signal
	printf("Sending file %s.\n", buf);
	bzero(buf, sizeof(buf));

	while(fread(buf, 1, 1000, file) > 0) {	// read file content to the buffer
		write(sid, buf, strlen(buf));		// written to client buffer
		if(strlen(buf) < 1000) {
			printf("All bytes of %s has been sent.");
		} else {
			printf("1000 bytes of %s has been sent.");
			}
		bzero(buf, sizeof(buf));
		sleep(2);
	}
	
	fclose(file);
	close(sid);
	g_id[tid] = 0;
}

void cp(int sid,char *str){
char *ferror = "No such file";	// error msg
	char buf[SIZE];
	char f[100];
	int i;
	char t_id_string[10];
        char f2[100];	// file name 2
	char buf1[SIZE];
	int j;
	char t_id_string1[10];
        char *str2;

	bzero(f, sizeof(f));
	bzero(buf, sizeof(buf));
	
	// get the file name 1
	for (i = 3; i < strlen(str)-1; i++) {
		f[i-3] = str[i];	
	}
        int tid = sdict[sid];
	sprintf(t_id_string, "%d", tid);
	g_id[tid] = 1;

	FILE *file = fopen(f, "rb");
	if (file == NULL) {
		printf("No such file: %s\n\n", f);
		write(sid, ferror, strlen(ferror));
		g_id[tid] = 0;
		return;
	}
        write(sid, t_id_string, strlen(t_id_string));
	
	int ret = file_curr(mutex_dict, f);
	while(ret == 1){
		ret = file_curr(mutex_dict, f);
	}

	read(sid, buf, sizeof(buf)); // read the sending signal
	printf("Sending file %s.\n", buf);
	bzero(buf, sizeof(buf));


        int tid1 = sdict[sid];
	sprintf(t_id_string1, "%d", tid1);
	
	
	write(sid, t_id_string1, strlen(t_id_string1));
	
	bzero(f2, sizeof(f2));
	bzero(buf1, sizeof(buf1));
	while(fread(buf, 1, 1000, file) > 0) {	// read file content to the buffer
		write(sid, buf, strlen(buf));		// write the buffer to the client
		if(strlen(buf) < 1000) {
                 fclose(file);

          // get the file name 2
	for (i = 8; i < strlen(str2) - 1; i++) {
		f2[i-8] = str2[i];
	}
	
	fdict[tid1] = f2;
	int ret = file_curr(mutex_dict, f2);
	while(ret == 1){
		ret = file_curr(mutex_dict, f2);
	}
	
	mutex_dict[tid1] = f2;
	//printf("fdict[%d]: %s is locked.\n",tid, fdict[tid1]);
	
	FILE *file2 = fopen(f2, "wb");
	if (file2 == NULL) {
		printf("Cannot write the file %s\n", f2);
		
	}
	
	while (read(sid, buf1, 1000) > 0) {
		fwrite(buf1, sizeof(char), strlen(buf1), file2);
                printf("All bytes of %s has been sent.");
		} 
            fclose(file2);
}
}
              
              close(sid);

}
void rm (int sid, char *str) {
	char f[100];
	int i;
        char *success_rm = "File successfully deleted.\n";
	char *fail_rm = "No such file.\n";
	
	
	bzero(f, sizeof(f));
	
	// get the file name
	for (i = 3; i < strlen(str) - 1; i++) {
		f[i-3] = str[i];
	}
	
	if (remove(f) != 0) {
		printf("%s: %s\n\n", f, fail_rm);
		write(sid, fail_rm, strlen(fail_rm));	
	} else {
       printf("%s: %s\n\n", f, success_rm);
		write(sid, success_rm, strlen(success_rm));
	}

}


void mv(int sid,char *str){
char *str2;
char f[1000];
char f2[1000];
char *ch1;
char *ch2;
	int i;
	char *fail_mv = "No such file found.\n";
	char *success_mv = "successful.\n";
	
	bzero(f, sizeof(f));
	
	// get the file name
	for (i = 3; i < strlen(str) - 1; i++) {
		f[i-3] = str[i];
	}
        // get new file name
	for (i = (strlen(str)+1); i < strlen(str2) - 1; i++) {
		f[i-(strlen(str)+1)] = str2[i];
         }
FILE *file = fopen(f, "r");
if (file == NULL) {
printf("No such file: %s\n\n", f);
write(sid, fail_mv, strlen(fail_mv));
}
 else{
   rename(f,f2);
   write(sid,success_mv,strlen(success_mv));
  }
      
}


   
void quit () {
	printf("Client is  disconnected.\n\n");
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


 