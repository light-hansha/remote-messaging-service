# remote-messaging-service
## Introduction

The following project was initially realized in late 2014 for the Operating Systems course, held by professor Francesco Quaglia at La Sapienza, University of Rome.

__Language used:__ C

#Project specification
Create a messaging service that allows users on different machines to exchange messages. The messaging service is characterized by a server program which resides  on a specific machine and accepts/stores messages sent from one or more client processes. A client process must provide the following operations:
* To read all the messages currently stored in the inbox.
* To send a new message to another user
* To delete all the messages currently stored in the inbox.

A message must contain  a recipient field, an object field and a text field.
The messaging system is restricted to authorized users only.

## Implementation choices
The application is based on the client/server paradigm and relies on a TCP connection. For simplicity purposes I adopted a static memory allocation model, defining 2 new data stractures:

    typedef struct _user{
       char username[16];
      char password[16];
    }user;

    typedef struct _message{
       char dest[16];
       char subject[32];
       char text[512];
    }message;

__Important assumptions:__ Usernames and passwords cannot contain spaces nor can be empty strings. Each username is unique and passwords are case sensitive. A valid message necessarily has a subject and a text and neither can be an empty string. The last character of each string is reserved for the NUL terminator, implying that the maximum length of a username is 15 characters (the same logic is applied to passwords). Subject and text strings that exceeds the maximum length of 31 characters and 511 character, respectively, will be truncated.
The _Data_ folder and its contents are essential for the proper execution of the application.

__Last notes:__ the user registration procedure was not required, but new users can be easily "registered" by editing the file _users.list_ inside the _Data_ folder, which contains a list of usernames and passwords separated by a space character.

#Server execution
Open the terminal and type in the following commands:

`% gcc server.c -o server`

`% ./server`
 
where % is the Unix prompt.

#Client execution
Invoke the compiler as usual:

`% gcc client.c -o client`
  

and then run the executable file using the following syntax: _client (username) (password)_

For example, if you want to log in as _user0_ (see _users.list_ file) you would type the following command:

`% ./client user0 0000`



