#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

typedef struct Client{
		int socketTCP;

		// adresse ip + port udp
		struct sockaddr_in from;
		uint16_t portInterneUDP;

		void (*Free)(struct Client *This);
		void (*Clear)(struct Client *This);
}Client;

Client* New_Client();
void Client_Init(struct Client* This);

//Destructructeurs
void Client_New_Free(Client* This);

//Others
void Client_Clear(Client* This);


#endif // CLIENT_H
