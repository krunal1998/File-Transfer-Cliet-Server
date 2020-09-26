#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h> 
#include <fcntl.h>

int findFile(char *);
void recieveFileFromServer(int, char *);
void sendFileToServer(int, char *);

int main(int argc, char *argv[]){
	char userCommand[100]={'\0'};
	int serverSD, portNumber;
	struct sockaddr_in servaddr;     // server socket address
	int sizeOfInput;
	char *operation, *fileName;
				
	
	if(argc != 3){
		printf("Call model: %s <IP Address> <Port Number>\n", argv[0]);
		exit(0);
	}

	//create socket 
	if ((serverSD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Cannot create socket\n");
		exit(-1);
	}

	//intialize server address structure
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	sscanf(argv[2], "%d", &portNumber);
	servaddr.sin_port = htons((uint16_t)portNumber);

	//assign server address in socket structore
	if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0){
		fprintf(stderr, " inet_pton() has failed\n");
		exit(-1);
	}
	
	//connect with server
	if(connect(serverSD, (struct sockaddr *) &servaddr, sizeof(servaddr))<0){
		fprintf(stderr, "connect() has failed, exiting\n");
		exit(-1);
	}

	printf("Got connected, Welcome to FTP Server.\n");
	while(1){
		printf("\nPlease Enter command:");
		fgets(userCommand, sizeof(userCommand), stdin);
		
		sizeOfInput = strlen(userCommand);
		//Remove trailing new line character from user input
		userCommand[sizeOfInput-1]='\0';
		
		//updated size of input
		sizeOfInput = strlen(userCommand);
		write(serverSD, userCommand ,strlen(userCommand) );
		
		if(strcmp(userCommand, "quit") == 0)
		{
			printf("Client process quits..\n");
			close(serverSD);
			exit(0);
		}
		else{
			//Tokenize user command to seperate filename and operation
			operation= strtok(userCommand, " ");
			fileName = strtok(NULL, " ");
			
			//Change directory
			chdir("ClientData");
			
			if ((strcmp(operation, "get") == 0) && fileName != NULL)
				recieveFileFromServer(serverSD, fileName);
			
			else if ((strcmp(operation, "put") == 0) && fileName != NULL)
				sendFileToServer(serverSD, fileName);

			else
				printf("Invalid command. \nPlease enter any command from following: get <FileName>, put <FileName> or quit \n");
			
			//change working directory to  the parent directory.
			chdir("..");
		}
		
		//Reset User Input Array and message array 
		memset(&userCommand[0], 0, sizeof(userCommand));
	} 
}

void recieveFileFromServer(int serverSD, char *fileName){
	int ack, fileDesc;
	char ch;
	//read acknowledgement from server and store it in ack variable
	read(serverSD, &ack, sizeof(ack));
	if(ack == 1){	//File exist on server side
		if ( (fileDesc = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0700))  == -1 ){
			printf("Error: can't create file\n");
			return;
		}
					
		printf("Recieving data from server of %s file.\n", fileName);	
		while ( read(serverSD, &ch, 1) >0 ){
			if(ch == 4)
				break;
			write(fileDesc, &ch, 1);
		}
		close(fileDesc);
		printf("Successfully downloaded...\n");
	}
	else	//File doesn't exist on server side
		printf("Sorry, %s file does not exist on server side\n", fileName);
}

void sendFileToServer(int serverSD, char *fileName){
	int ack, fileDesc;
	char ch;
	//Check specific file is present in the storage or not. it will return 0 for unsuccessful and 1 for successful.
	ack = findFile(fileName);
	//send acknowledgement of the file found to the server
	write(serverSD, &ack, sizeof(ack));
	
	if(ack == 1){	//file exists on client side.
		fileDesc = open(fileName, O_RDONLY);
		printf("Transfering data of %s file to the server.\n", fileName);

		//read one by one characters from file and write it on socket.
		while(read(fileDesc, &ch, 1) > 0)
			write(serverSD,&ch , 1);
		
		//After reading all content of file, Store ASCII code 4 in ch and send it as an end of transaction.
		ch = 4;
		write(serverSD, &ch, 1);
					
		close(fileDesc);
					
		printf("Successfully Uploaded...\n");
				
	}
	else	//File doesn't exist on client side
		printf("%s file doesn't exists on client side.\n", fileName);
				
}

//This function will find file from client storage and return 0 if file not found and 1 for file found.
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
