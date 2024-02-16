#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct data_base_with_port
{
	int port;
	char info[2048];
	struct data_base_with_port *next;
};

typedef struct data_base_with_port database;

database *d1=NULL;


// reading from database

void read_from(int client_port, char *response)
{
	printf("[+] ENTERED read_from function\n");
	if (d1==NULL)
	{
		strcpy(response,"FIN");
		return NULL;
	}
	while(d1->next!=NULL)
	{
		if(d1->port==client_port)
		{
			strcpy(response,d1->info);
			return NULL;
		}
		d1=d1->next;
	}
	if (d1==NULL)
	{
		strcpy(response,"FIN");
	}
	else if(d1->port==client_port)
	{
		strcpy(response,d1->info);
	}
	else
	{
		strcpy(response,"FIN");
	}
}

void write_port(int client_port, char *content,char *response)
{
	database *temp=(database*)malloc(sizeof(database));
	temp->port=client_port;
	strcpy(temp->info,content);
	temp->next=NULL;
	if(d1==NULL)
	{
		d1=temp;
	}
	else
	{
		while(d1->next!=NULL && d1->port!=client_port)
			d1=d1->next;
		if(d1->next==NULL)
			d1->next=temp;
		else if(d1->port==client_port)
		{
			// over writing the content in the port info 
			strcpy(d1->port,content);
		}
	}
	strcpy(response,"[200 OK]");
}

void get_count(char *response)
{
	int n=0;
	while(d1!=NULL)
	{
		n++;
		d1=d1->next;
	}
	itoa(n,response,1024);
}

void remove_port(int client_port,char *response)
{
	// if database is empty
	if (d1==NULL)
	{
		strcpy(response,"[database EMPTY]");
		return NULL;
	}
	
	// if the first port in the database is the port which is to be cleared
	if (d1->port==client_port)
	{
		d1=d1->next;
		strcpy(response,"[200 OK]");
		return NULL;
	}

	// if the database has only one port
	if(d1->next==NULL)
	{
		if(d1->port==client_port)
		{
			d1=NULL;
			strcpy(response,"[200 OK]");
		}
		else
		{
			strcpy(response,"[NOT FOUND]");
		}
		return NULL;
	}
	
	// general case
	else
	{
		database *prev=d1;
		database *cur=d1->next;
		while(cur->port!=client_port && cur!=NULL)
		{
			prev=prev->next;
			cur=cur->next;
		}
		if(cur->port==client_port)
		{
			database *temp=cur;
			prev->next=cur->next;
			temp->next=NULL;
			free(temp);
			strcpy(response,"[200 OK]");
			return NULL;
		}
		else
		{
			strcpy(response,"[NOT FOUND]");
			return NULL;
		}
	}
}

void get_rep(int port,int client_sock,char *request,char *response,char *content)
{
	if (strncmp("READ",request,4)==0)
	{
		// read from port as provided 
		read_from(port,response);
	}
	else if(strncmp("WRITE",request,5)==0)
	{
		write_port(port,content,response);
	}
	else if (strncmp("COUNT",request,5)==0)
	{
		get_count(response);
	}
	else if (strncmp("DELETE",request,6)==0)
	{
		remove_port(port,response);
	}
	else if (strncmp("END",request,3)==0)
	{
		strcpy(response,"END");
		close(client_sock);
	    printf("[+]Client disconnected.\n\n");
	}
	else
	{
		strcpy(response,"INVALID");
		close(client_sock);
	    printf("[+]Client disconnected.\n\n");
	}
	// return response
}



int main()
{

  char *ip = "192.168.1.10";
  int port = 5566;

  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  char buffer[1024];
  char response[1024];
  int n;

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0){
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket created.\n");

  memset(&server_addr, '\0', sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  n = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (n < 0){
    perror("[-]Bind error");
    exit(1);
  }
  printf("[+]Bind to the port number: %d\n", port);

  listen(server_sock, 5);
  printf("Listening...\n");

  while(1){
    addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    printf("[+]Client connected.\n");

    bzero(buffer, 1024);
    recv(client_sock, buffer, sizeof(buffer), 0);
    printf("Client: %s\n", buffer);
    get_rep(port,client_sock,&buffer,&response);
    bzero(buffer, 1024);
    // strcpy(buffer, "HI, THIS IS SERVER. HAVE A NICE DAY!!!");
    printf("Server: %s\n", response);
    send(client_sock, response, strlen(response), 0); 
  }

  return 0;
}
