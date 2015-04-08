#ifndef LISTECLIENT_H
#define LISTECLIENT_H

#include "client.h"
#include <stdint.h>
#include "Bool.h"

#define ERROR_MALLOC_ITEM -1
#define ERROR_LISTE_VIDE -2

typedef struct MaillonListeClient{
		struct Client* c;
		struct MaillonListeClient* next;
}MaillonListeClient;

typedef struct ListeClient {
	uint16_t taille;
	pthread_mutex_t mutexListeClient;
	struct MaillonListeClient *Top;
	void(*Free)(struct ListeClient*, char freeClients);
	int16_t(*Push)(struct ListeClient*, struct Client*);
	struct Client*(*Pop)(struct ListeClient*);
	void(*Clear)(struct ListeClient*, char freeClient);
	uint16_t (*Taille)(struct ListeClient*);
	struct Client*(*getNieme)(struct ListeClient*, uint16_t);
	struct Client*(*getFromFrom)(struct ListeClient*, struct sockaddr_in);
	struct Client*(*getFromSockNo)(struct ListeClient*, int sockNo);
	Bool (*remove)(struct ListeClient *This, int sockToFind);
	Bool (*removeAll)(struct ListeClient *This);
}ListeClient;

uint16_t ListeClient_Taille(ListeClient* This);
void ListeClient_Clear(ListeClient *This, char freeClients);
void ListeClient_New_Free(ListeClient *This, char freeClients);
int16_t ListeClient_Push(ListeClient* This, struct Client *c);
struct Client* ListeClient_getNieme(ListeClient* This, uint16_t number);
struct Client* ListeClient_Pop(ListeClient* This);
ListeClient* New_ListeClient();
void ListeClient_Init(ListeClient* This);
struct Client* ListeClient_getFromFrom(ListeClient *This, struct sockaddr_in from);
struct Client* ListeClient_getFromSockNo(ListeClient *This, int sockNo);
Bool ListeClient_remove(ListeClient *This, int sockToFind);
Bool ListeClient_removeAll(ListeClient *This);

#endif // LISTECLIENT_H
