#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h> 
#include <fcntl.h>

int listenSD, clientSD;
struct sockaddr_in servaddr;	//server socket address
int portNumber;	

void serviceClient();
int findFile(char *);
void sendFileToClient(int , char *);
void recieveFileFromClient(int, char *);

int main(int argc, char *argv[]){  
  
	
	if(argc != 2){
		printf("Call model: %s <Port Number>\n", argv[0]);
		exit(0);
	}

	//Create Socket
	listenSD = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSD <0){
		fprintf(stderr, "Can't create socket\n");							//remove this
		exit(-1);
	}
	//Initialize socket address structure
	bzero(&servaddr, sizeof(servaddr));										//remove this 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sscanf(argv[1], "%d", &portNumber);
	
	servaddr.sin_port = htons((uint16_t)portNumber);
  
	//bind socket address to the socket
	if((bind(listenSD, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0){
		fprintf(stderr, "Can't bind socket with socket address\n");				//this also
		exit(-1);
	}

	//Make the socket listening
	if((listen(listenSD, 5))<0){
		fprintf(stderr, "Error in listen\n");								//Remove this
		exit(-1);
	}

	printf("Waiting for a client...\n");
	while(1){
		
		//wait for client connection
		clientSD = accept(listenSD, NULL, NULL);
		if(!fork()){
			//Close listening socket in child. So that reference count reamains one. 
			//This child serves the client it does not need listening socket to do this.
			close(listenSD);												//remove this	
			serviceClient();
		}
		close(clientSD);
	}
}

void serviceClient(){
	char command[100] = {'\0'};
	char *operation, *fileName;

	printf("Server: Got a client- %d\n", getpid());
	while(1){
		read(clientSD, command, 100);
		
		printf("\nGot command from client-%d:: %s \n",getpid(), command);
		
		if(strcmp(command, "quit") == 0){
			printf("Client-%d left.\n", getpid());
			close(clientSD);
			exit(0);
		}
		else{
			//Tokenize user command to seperate filename and operation
			operation= strtok(command, " ");
			fileName = strtok(NULL, " ");
				
			//change directory
			chdir("ServerData");
			if ((strcmp(operation, "get") == 0) && fileName != NULL)
				sendFileToClient(clientSD, fileName);
			
			else if ((strcmp(operation, "put") == 0) && fileName != NULL)
				recieveFileFromClient(clientSD, fileName);

			else
				printf("Invalid command entered by client-%d\n", getpid());
			
			//change working directory to  the parent directory.
			chdir("..");
//			printf("\nWaiting for next command from client-%d...\n", getpid());
		}
		//reset command array.
		memset(&command[0], 0, sizeof(command));
	}
}

void sendFileToClient(int clientSD, char *fileName){
	int ack, fileDesc;
	char ch;
	//Check specific file is present in the storage or not. it will return 0 for unsuccessful and 1 for successful.
	ack = findFile(fileName);
				
	//send acknowledgement of the file found to the client
	write(clientSD, &ack, sizeof(ack));
				
	if(ack == 1){	//means file is present 
		if((fileDesc = open(fileName, O_RDONLY)) <0){
			printf(">>>> Error in opening\n");
			return;
		}
		
		printf("Tranfering content of %s file from server to client-%d.\n ", fileName, getpid());
		//read one by one characters from file and write it on socket.
		while(read(fileDesc, &ch, 1) > 0)
			write(clientSD,&ch , 1);

		//After reading all content of file, Store ASCII code 4 in ch and send it as an end of transaction.
		ch = 4;
		write(clientSD, &ch, 1);
						
		printf("%s file transferred successfully to the client-%d\n", fileName, getpid());
		close(fileDesc);
	}
	else	//means file isn't present 
		printf("%s File is not present on our storage.\n", fileName);
}

void recieveFileFromClient(int clientSD, char *fileName){
	int ack, fileDesc;
	char ch;
	//read acknowledgement from client and store in ack variable
	read(clientSD, &ack, sizeof(ack));
	if(ack == 1){	//File exists on client side
		if ((fileDesc = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0700))  == -1 ){
			printf("Error: can't create file\n");
			return;
		}

		printf("Receiving data from client-%d for %s file.\n", getpid(), fileName);
		while ( read(clientSD, &ch, 1) >0 ){
			if(ch == 4)
				break;
			write(fileDesc, &ch, 1);
		}
		printf("%s file successfully stored on server side. \n", fileName);
		close(fileDesc);
	}
	else
		printf("Not recieved any data for %s file from client-%d\n", fileName, getpid());	
}

//This function will find file from server storage and return 0 if file not found and 1 for file found.
int findFile(char * fileName){
	DIR *dp;
	struct dirent *dirp;
	
	//open current directory
	if ((dp= opendir(".")) == NULL)
	{
		printf("can't open directory\n");
		return 0;
	}
	
	//read every file in directory
	while ((dirp = readdir(dp)) != NULL){
		//compare current file's name with fileName that we want to search. 
		if (!strcmp(dirp->d_name,fileName)){
			//File found. Close current directory and Return 1 
			closedir(dp);
			return 1;
		}
	}
	//File not found. Close current directory and Return 0 	
	closedir(dp);
	return 0;
}
