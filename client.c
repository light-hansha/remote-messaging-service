//=============================================================//
//   File:  client.c   ||   Author:  Francesco Cipolla         //
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

#define DISPLAY 1   
#define SEND 2      
#define DELETE 3   
#define EXIT 4

#define ARG_NUM 3
#define USER_SIZE 16
#define PASS_SIZE 16
#define SUBJECT_SIZE 32
#define TEXT_SIZE 512

#define CHECK_SUBJECT 0
#define CHECK_TEXT 1

#define SHORT_TIMEOUT 10
#define TYPING_TIMEOUT 60
#define RESET_TIMEOUT 0


//NEW TYPE DEFINITION
typedef struct _user{ //32 bytes
   char username[16];
   char password[16];
}user;

typedef struct _message{ //560 bytes
	char dest[16]; 
	char subject[32];
	char text[512];
}message;

//GLOBAL VARIABLES
int ds_sock;

//SIGNAL-HANDLING FUNCTIONS
void shutdown_client(){ 
	printf("Service interrupted.\n========================\n");
	close(ds_sock);
	exit(0);
}

void is_inactive(){ 
	printf("Timeout exceeded.\n========================\n");
	close(ds_sock);
	exit(0);
}

//AUXILIARY FUNCTIONS
void check_send();
void check_recv();

void to_lowercase(char** s){ 
	int i;
	for(i=0 ; (*s)[i]!='\0' ; i++){
		(*s)[i]=tolower((*s)[i]);
	}
}

void display_messages(){
	printf("\n=== Display messages ===\n");
	int count_msg = 0;
	message msg;
	while(1){
		
		alarm(SHORT_TIMEOUT);
		int recv_res = recv(ds_sock, &msg, sizeof(msg), 0);
		alarm(RESET_TIMEOUT);
		
		check_recv(recv_res);
		
		if(strcmp(msg.dest,"")==0) break;
		else{
			count_msg++;
			printf("Message %d\n   From: %s\n   Subject: %s\n   Text: %s\n",count_msg, msg.dest, msg.subject, msg.text);
		}
	}
	if(count_msg==0)printf("Your inbox is empty.\n");
	return;
}

int check_input_username(char* us){
	int total_length = strlen(us);
	if(total_length == 1) return -1; //invalid username ""
	int i;
	if(total_length==USER_SIZE-1 && (us[USER_SIZE-2] != '\n')){
		 int count_stream_char=1;
		 while(getc(stdin)!='\n')count_stream_char++;
		 if (count_stream_char!=1) return -1; //username exceeeds max length
	}
	
	if(us[total_length-1]=='\n')us[total_length-1]='\0'; //replace '\n' with '\0'
	
	//total_length = strlen(us);
	total_length--;
	
	for(i=0 ; i < total_length ; i++){
		if(us[i] == ' ')return -1; //invalid username (contains spaces)
	}
	
	to_lowercase(&us);
	
	int send_res = send(ds_sock, us, USER_SIZE*sizeof(char), 0);
	check_send(send_res);
	
	int outcome;
	alarm(SHORT_TIMEOUT);
	int recv_res = recv(ds_sock, &outcome, sizeof(int), 0);
	alarm(RESET_TIMEOUT);
	check_recv(recv_res);
	
	if(outcome==-1) return -2; //username doesn't exists
	return 0; //ok		
}

int check_input_string(char* string, int type){
	int ref_length;
	if(type==CHECK_SUBJECT) ref_length = SUBJECT_SIZE;
	else if(type==CHECK_TEXT)ref_length = TEXT_SIZE;
	
	int total_length = strlen(string);
	
	if(total_length == 1) return -1; //invalid string ""
	if(total_length == ref_length-1 && (string[ref_length-2] != '\n')){
		 int count_stream_char=1;
		 while(getc(stdin)!='\n')count_stream_char++;
		 if (count_stream_char!=1)printf("WARNING: Input string exceeds maximum length. The message was cut.\n");
	}
	
	if(string[total_length-1]=='\n')string[total_length-1]='\0';
	
	return 0;
	
}

void send_new_message(){
	printf("\n=== Send new message ===\n");
	char user_dest[USER_SIZE];
	char subject[SUBJECT_SIZE];
	char text[TEXT_SIZE];
	
	//TYPE USERNAME 
	printf("Insert username (15 characters max):\n");
	alarm(SHORT_TIMEOUT);
	if(fgets(user_dest, USER_SIZE, stdin)==NULL){
		printf("Read Error\n");
		raise(SIGTERM);
	}
	alarm(RESET_TIMEOUT);
	int outcome = check_input_username(user_dest);
	if(outcome==-1){
		printf("Invalid username!\n");
		raise(SIGTERM);
	}
	else if(outcome==-2){
		printf("The user %s doesn't exists\n",user_dest);
		raise(SIGTERM);
	}
	
	//TYPE SUBJECT
	printf("Type the subject of your message (31 characters max):\n");
	alarm(TYPING_TIMEOUT);
	if(fgets(subject, SUBJECT_SIZE, stdin)==NULL){
		printf("Read Error\n");
		raise(SIGTERM);
	}
	alarm(RESET_TIMEOUT);
	outcome = check_input_string(subject, CHECK_SUBJECT);
	if(outcome==-1){
		printf("Invalid subject!\n");
		raise(SIGTERM);
	}
	
	//TYPE TEXT
	printf("Type the text of your message (511 characters max):\n");
	alarm(TYPING_TIMEOUT);
	if(fgets(text, TEXT_SIZE, stdin)==NULL){
		printf("Read Error\n");
		raise(SIGTERM);
	}
	alarm(SHORT_TIMEOUT);
	outcome = check_input_string(text, CHECK_TEXT);
	if(outcome==-1){
		printf("Invalid text\n");
		raise(SIGTERM);
	}

	//SEND INFO
	message msg;
	strcpy(msg.subject,subject);
	strcpy(msg.text,text);
	
	int send_res = send(ds_sock, &msg, sizeof(msg), 0);
	check_send(send_res);
	
	alarm(SHORT_TIMEOUT);
	int recv_res = recv(ds_sock, &outcome, sizeof(int), 0);
	alarm(RESET_TIMEOUT);
	check_recv(recv_res);
	
	if(outcome==0) printf("\nOK! Message sent!\n");
	else printf("\nAn error occurred! Please try again.\n");
	return;
}

void delete_inbox(){
	printf("\n===== Delete inbox =====\n");
	int outcome;
	alarm(SHORT_TIMEOUT);
	int recv_res = recv(ds_sock, &outcome, sizeof(int), 0);
	alarm(RESET_TIMEOUT);
	
	check_recv(recv_res);
	
	if(outcome == 0)printf("Inbox messages successfully deleted.\n");
	else printf("Cancellation failed!\n");
	return;
}

void check_send(int send_res){
	if(send_res==0){
		printf("No data sent!\n");
		raise(SIGTERM);
	}
	if(send_res==-1){
		printf("Send failed!\n");
		raise(SIGTERM);
	}
}

void check_recv(int recv_res){
	if(recv_res==0){
		printf("No data received!\n");
		raise(SIGTERM);
	}
	if(recv_res==-1){
		printf("Receive failed!\n");
		raise(SIGTERM);
	}
}

//MAIN FUNCTION
int main(int argc, char** argv){
	//SIGNAL HANDLING
	signal(SIGINT, shutdown_client);
	signal(SIGTERM, shutdown_client);
	signal(SIGPIPE, shutdown_client);
	signal(SIGKILL, shutdown_client);
	signal(SIGHUP, shutdown_client);
	signal(SIGSEGV, shutdown_client);
	signal(SIGALRM, is_inactive);
	
	int command;
	
	//CHECK ARGUMENTS
	if(argc < ARG_NUM){ 
		printf("Too few arguments.\nPlease use the correct sintax: client (username) (password)\n");
		exit(0);
	}
	if(argc > ARG_NUM){
		printf("Too many arguments.\nPlease use the correct sintax: client (username) (password)\n");
		exit(0);
	}
	
	int l_us = strlen(argv[1]); 
	int l_pa = strlen(argv[2]);
	if(l_us > USER_SIZE-1){ 
		printf("Username exceeds maximum length, please retry.\n");
		exit(0);
	}
	else if(l_pa > PASS_SIZE-1){
		printf("Password exceeds maximum length, please retry.\n");
		exit(0);	
	}
	
	to_lowercase(&argv[1]);
	user login_data;
	memset(&login_data, 0, sizeof(login_data));
	strcpy(login_data.username, argv[1]); 
	strcpy(login_data.password, argv[2]); 
	
	//ESTABLISH CONNECTION
	struct sockaddr_in client_addr;
	struct hostent* hp;
	
	ds_sock=socket(AF_INET, SOCK_STREAM, 0);
	if(ds_sock==-1){
	   printf("Cannot create socket.\n");
	   exit(0);
	}

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family=AF_INET; 
	client_addr.sin_port=htons(7777);
	//client_addr.sin_addr.s_addr=inet_addr("192.168.1.62");    STATIC IP TEST
	//memcpy(&client_addr.sin_addr, &client_addr.sin_addr.s_addr, 4); 
	hp=gethostbyname("localhost");
	memcpy(&client_addr.sin_addr, hp->h_addr, 4);
	
	int c=connect(ds_sock, (struct sockaddr*)&client_addr, sizeof(client_addr));
	if(c==-1){
		printf("A connection failure has occurred.\n");
		raise(SIGTERM);
	}
	
	//SEND AUTENTICATION DATA
	printf("\n==== Authentication ====\nLogging in...\n");
	int send_res = send(ds_sock, &login_data, sizeof(user), 0);
	check_send(send_res);
	
	//RECEIVE RESULT
	int outcome;
	alarm(SHORT_TIMEOUT);
	int recv_res = recv(ds_sock, &outcome, sizeof(int), 0);
	alarm(RESET_TIMEOUT);
	
	check_recv(recv_res);
	
	//CHECK RESULT
	if(outcome==-2){
		printf("The user %s doesn't exists, please try again.\n", login_data.username);
		shutdown_client();
	}
	else if(outcome==-1){
		printf("Invalid password, please try again.\n");
		shutdown_client();
	}
	else printf("Login succeded, connection with server successfully established.\n");
	
	//SELECT OPERATION
	printf("\n\n=== Select operation ===\nInsert command:\n[1] - View your received messages\n"
	"[2] - Send a new message\n[3] - Delete messages from your inbox\n[4] - Exit\n");
	
	command=0;
	alarm(SHORT_TIMEOUT);
	scanf("%d",&command);
	alarm(RESET_TIMEOUT);
	
	char newline = getc(stdin);
	send_res = send(ds_sock, &command, sizeof(int), 0);
	
	check_send(send_res);
	
	if(command == DISPLAY) display_messages();
	else if(command == SEND)send_new_message();
	else if(command == DELETE)delete_inbox();
	else if(command == EXIT){
		printf("Logged out.\n");
		printf("========================\n");
		exit(0);
	}
	else{
		printf("Unknown command. ");	
		shutdown_client();
	}
	
	printf("========================\n");
	close(ds_sock);
	exit(0);	
	
}
