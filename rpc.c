/*
 * rpc.c
 *
 *  Created on: Jul 3, 2014
 *      Author: Aaron
 */


#include "rpc.h"
#include "err.h"
#include "message.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <cstring>
#include <map>
using namespace std;
#define MAX_CONNECTIONS 100
#define HOST_NAME_SIZE 256
int MAX_SIZE = 2^32-1;

int binderSocket, clientSocket, serverPort;
char address[HOST_NAME_SIZE];
map <string, skeleton> localDb ;


string convertToString(char* c){
	stringstream ss;
	string s;
	ss<c;
	ss>>s;
	return s;
}
string getKey(string name, string argTypeStr){
	stringstream ss;
	stringstream ssOut;
	ss.str(argTypeStr);
	ssOut <<name<<",";
	string stemp;
	int withOutLen;
	int removeLen = 65535 << 16; //16 1s followed by 16 0s
	while(getline(ss, stemp, '#')){
		int withLen = atoi(stemp.c_str());
			//elemenate lower 16 bits
			withOutLen = withLen & removeLen;
		if (withLen != withOutLen){
			//mark as array
			withOutLen = withOutLen | 1;
		}
			ssOut << withLen << "#";
	}
	return ssOut.str();
}

//Server Function
int rpcInit() {
	char * address = getenv("BINDER_ADDRESS");
	char * port = getenv("BINDER_PORT");
	int retVal;

	if (address == NULL || port == NULL) {
		return -1;
	}


	//Connecting to binder
	binderSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (binderSocket == -1) {
		return -1;
	}

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(address, port, &hints, &res);
	retVal = connect(binderSocket, res->ai_addr, res->ai_addrlen);
	if (retVal == -1) {
		return -1;
	}

	//Opening connection for clients
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket == -1) {
		return -1;
	}

	struct sockaddr_in sockAddress;
	sockAddress.sin_family = AF_INET;
	sockAddress.sin_addr.s_addr = INADDR_ANY;
	sockAddress.sin_port = htons(0);
	retVal = bind(clientSocket , (struct sockaddr *)&sockAddress, sizeof(sockAddress));
	if (retVal == -1) {
		return -1;
	}
	retVal = listen (clientSocket, MAX_CONNECTIONS);
	if(retVal == -1) {
		return -1;
	}

	struct sockaddr_in sin;
	int addrlen = sizeof(sin);
	getsockname(clientSocket, (struct sockaddr *)&sin, (socklen_t*)&addrlen);
	int serverPort = ntohs(sin.sin_port);
	if (serverPort <= 0) {
		return -1;
	}
	gethostname(address, HOST_NAME_SIZE);
	if (address == NULL) {
		return -1;
	}

	return SUCCESS;
}

int rpcRegister(char *name, int *argTypes, skeleton f){
	//insert into local db
	stringstream ss;
	ss<<name;

	//create register msg
	string msg = createRegisterMsg(serverPort, name, argTypes);
	//send register msg
	int len = strlen(msg.c_str())+1;
	send(binderSocket, msg.c_str(), len, 0);

	//listen for acknlowedgment
	char reply[10];
	recv(binderSocket, reply, len,0);
	ss << reply;
	int returnCode;
	ss >> returnCode;
	if(returnCode == 0){
		//success insert into local db
		string argTypeStr;
		ss.str('');
		ss.str(msg);
		getline(ss, argTypeStr, ',');//length
		getline(ss, argTypeStr, ',');//type
		getline(ss, argTypeStr, ',');//add
		getline(ss, argTypeStr, ',');//port
		getline(ss, argTypeStr, ',');//name
		getline(ss, argTypeStr, ',');//argType
		string key = getKey(name, argTypeStr);
		localDb[key] = f;
	}
	else{
		//error?
	}

}
