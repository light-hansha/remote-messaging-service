//=============================================================//
//   File:  server.c   ||   Author:  Francesco Cipolla         //
//=============================================================//

//   Version: 1.2   
//   Last changed: 2015/11/03

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>

#define BACKLOG 10 

#define DISPLAY 1   
#define SEND 2     
#define DELETE 3  
#define EXIT 4

#define USER_SIZE 16
#define PASS_SIZE 16
#define SUBJECT_SIZE 32
#define TEXT_SIZE 512

#define SHORT_TIMEOUT 10
#define TYPING_TIMEOUT 60
#define RESET_TIMEOUT 0

#define DATA_PATH "./Data/"
#define DATA_PATH_LENGTH 7


//NEW TYPE DEFINITION
typedef struct _user{
   char username[16];
   char password[16];
}user;

typedef struct _message{
	char dest[16]; 
	char subject[32];
	char text[512];
}message;

//GLOBAL VARIABLES
FILE *users; 
FILE *my_inbox;
int ds_sock;
int ds_sock_a;
struct sockaddr client_addr;
int addrlen;
user login_data;


//SIGNAL-HANDLING FUNCTIONS
void shutdown_server(){
	printf("\nService interrupted.\n");
	close(ds_sock_a);
	close(ds_sock);
	exit(0);
}
	
void is_inactive(){ 
	printf("Client is inactive.\n");
	close(ds_sock_a);
}

void closure(){
	printf("Client desynchronized.\n");
	close(ds_sock_a);
}

//AUXILIARY FUNCTIONS
void listening();
void check_recv();
void check_send();

int compare_username(char* us1, char* us2){
	int l1 = strlen(us1);
	if(l1 != strlen(us2)) return -1;
	int i;
	for(i=0 ; i<l1 ; i++){
		if(us1[i] != us2[i] && us1[i] != tolower(us2[i])) return -1;
	}
	return 0;
}

void display_messages(){
	printf("Option 1 selected\n");
	int i, ch;
	char sender[USER_SIZE];
	char subject[SUBJECT_SIZE];
	char text[TEXT_SIZE];
	message msg;
	
	char path[DATA_PATH_LENGTH +strlen(login_data.username)];
	strcpy(path, DATA_PATH);
	strcat(path, login_data.username);
	
	my_inbox = fopen(path, "a+");
	while(EOF != fscanf(my_inbox,"%15s%*c",sender)){
		i=0;
		ch=fgetc(my_inbox);
		while (i <= SUBJECT_SIZE-1 && EOF != ch){
			if(ch == '\n') break;
        subject[i++] = ch;
        ch=fgetc(my_inbox);
		}
		subject[i]='\0';
		i=0;
		ch=fgetc(my_inbox);
		while (i <= TEXT_SIZE-1 && EOF != ch){
			if(ch == '\n') break;
        text[i++] = ch;
        ch=fgetc(my_inbox);
		}
		text[i]='\0';
		
		strcpy(msg.dest, sender);
		strcpy(msg.subject, subject);
		strcpy(msg.text, text);
			
		int send_res = send(ds_sock_a, &msg, sizeof(message), 0);
		check_send();
		}
	fclose(my_inbox);
	strcpy(msg.dest,"");
	int send_res = send(ds_sock_a, &msg, sizeof(message), 0);
	check_send();
		
	printf("Done.\n");
	return;
}


void send_new_message(){
	printf("Option 2 selected.\n");
	
	alarm(SHORT_TIMEOUT);
	char us[USER_SIZE];
	int recv_res = recv(ds_sock_a, us, USER_SIZE*sizeof(char), 0);
	alarm(RESET_TIMEOUT);
	check_recv(recv_res);
	
	int registered = search_user(us); 
	
	int send_res = send(ds_sock_a, &registered, sizeof(int), 0);
	check_send(send_res);
	
	if(registered == -1){
		closure();
		listening();
	}
	
	message msg;
	alarm(2*TYPING_TIMEOUT);
	recv_res = recv(ds_sock_a, &msg, sizeof(msg), 0);
	alarm(RESET_TIMEOUT);
	check_recv(recv_res);
	
	char path[DATA_PATH_LENGTH +strlen(us)];
	strcpy(path, DATA_PATH);
	strcat(path, us);
	
	FILE* user_dest = fopen(path,"a");
	int bytes = fprintf(user_dest, "%s\n%s\n%s\n\n",login_data.username,msg.subject,msg.text);
	fclose(user_dest);
	
	int outcome=0;
	if (bytes<0){
		printf("Writing error occurred!\n");
		outcome = -1;
	}
	else printf("%s received a new message.\n", us);
	
	send_res = send(ds_sock_a, &outcome, sizeof(int), 0);
	check_send(send_res);
	
	printf("Done.\n");
	return;	
}

int search_user(char* username){
	users = fopen("./Data/users.list", "r");
	char us[USER_SIZE];
	while(EOF != fscanf(users,"%s %*s\n",us)){
		if(compare_username(username, us)==0){
			fclose(users);
			return 0;
		}
	}
	fclose(users);
	return -1;
}

void delete_inbox(){
	printf("Option 3 selected.\n");
	int outcome = 0;
	
	char path[DATA_PATH_LENGTH +strlen(login_data.username)];
	strcpy(path, DATA_PATH);
	strcat(path, login_data.username);
	
	my_inbox = fopen(path, "w");
	if (my_inbox==NULL) outcome=-1;
	fclose(my_inbox);
	int send_res = send(ds_sock_a, &outcome, sizeof(int), 0);
	check_send(send_res);
	printf("Done.\n");
	return;
}

int authentication(char* username, char* password){
		users = fopen("./Data/users.list", "r");
		char us[USER_SIZE];
	    char pass[PASS_SIZE];
	    while(EOF != fscanf(users,"%s %s\n",us,pass)){
			if(compare_username(username, us)==0){
				if(strncmp(password,pass,PASS_SIZE)==0){
					printf("user %s logged in.\n",username);
					fclose(users);
					return 0;
				}
				else{
					fclose(users);
					return -1; //incorrect password
				}	
			}
		}
		fclose(users);
		return -2; //user not recognized
}

//LISTENING FUNCTION
void listening(){
	while(1){
		printf("\nServer is ready to accept new client connections...\n");
		
		memset(&ds_sock_a, 0, sizeof(ds_sock_a));
		memset(&client_addr, 0, sizeof(client_addr));
		memset(&addrlen, 0, sizeof(addrlen));
		ds_sock_a=accept(ds_sock, &client_addr, &addrlen);
		if(ds_sock_a==-1){
			printf("Accept failed!\n");
			listening();
		}
		
		//RECEIVE AUTHENTICATION DATA
		memset((void*)&login_data, 0, sizeof(user)); 
		alarm(SHORT_TIMEOUT);
		int recv_res = recv(ds_sock_a, &login_data, sizeof(user), 0); 
		alarm(RESET_TIMEOUT);
		
		check_recv(recv_res);
		
		//AUTHENTICATION PROCEDURE
		int login_result = authentication(login_data.username,login_data.password);
		
		int send_res = send(ds_sock_a, &login_result, sizeof(int), 0);
		check_send(send_res);
		
		if(login_result<0){
			printf("Login attempt failed.\n");
			closure();
			listening();
		}
	
		//SELECT OPERATION PROCEDURE
		int command = 0;
		
		alarm(SHORT_TIMEOUT);
		recv_res = recv(ds_sock_a, &command, sizeof(int), 0);
		alarm(RESET_TIMEOUT);
		check_recv(recv_res);		
		
		if(command == DISPLAY) display_messages();
		else if(command == SEND) send_new_message();
		else if(command == DELETE) delete_inbox();
		else if(command == EXIT){
			closure();
			listening();
		}
		else printf("Unknown operation selected.\n");
	}			
}

void check_recv(int recv_res){
	if(recv_res==-1){
		printf("Receive error!\n");
		listening();
	}
	if(recv_res== 0){
		printf("No data received!\n");
		listening();
	}
	return;
}

void check_send(int send_res){
	if(send_res==-1){
		printf("Send error!\n");
		listening();
	}
	if(send_res== 0){
		printf("No data sent!\n");
		listening();
	}
	return;
}

//MAIN FUNCTION
int main(){
	//SIGNAL HANDLING
	signal(SIGINT, shutdown_server);
	signal(SIGHUP, shutdown_server);
	signal(SIGKILL, shutdown_server);
	signal(SIGTERM, shutdown_server);
	signal(SIGPIPE, closure);
	signal(SIGALRM, is_inactive);
	signal(SIGSEGV, shutdown_server);
	
	//ESTABLISH CONNECTION
	struct sockaddr_in server_addr;
	 
	ds_sock=socket(AF_INET, SOCK_STREAM, 0);
	if(ds_sock==-1){
		printf("Socket creation error\n.");
		exit(0);
	}
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(7777);
	server_addr.sin_addr.s_addr=INADDR_ANY;
	
	if(bind(ds_sock, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){
		printf("Bind error.\n");
		close(ds_sock);
		exit(0);
	}
	
	if(listen(ds_sock, BACKLOG)==-1){
	   printf("Listen error.\n");
	   close(ds_sock);
	   exit(0);
	}
	
	printf("=== Server is up and running ===");

	listening();
}
