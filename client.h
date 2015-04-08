#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include "listecase.h"

typedef struct Client{
		int socketTCP;
		// adresse ip + port udp
        struct sockaddr_in from;
        struct ListeCase* casesTo;
		void (*Free)(struct Client *This);
		void (*Clear)(struct Client *This);
		int16_t posX;
		int16_t posY;
		char ipPortString[21];
}Client;

Client* New_Client();
void Client_Init(struct Client* This);

//Destructructeurs
void Client_New_Free(Client* This);

//Others
void Client_Clear(Client* This);


#endif // CLIENT_H
