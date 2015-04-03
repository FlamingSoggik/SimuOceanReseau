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
	struct MaillonListeClient *Top;
	void(*Free)();
	int16_t(*Push)(struct ListeClient*, struct Client*);
	struct Client*(*Pop)(struct ListeClient*);
	void(*Clear)(struct ListeClient*);
	uint16_t (*Taille)(struct ListeClient*);
	struct Client*(*getNieme)(struct ListeClient*, uint16_t);
	struct Client*(*getFrom)(struct ListeClient*, struct sockaddr_in);
	Bool (*remove)(struct ListeClient *This, int sockToFind);
}ListeClient;

uint16_t ListeClient_Taille(ListeClient* This);
void ListeClient_Clear(ListeClient *This);
void ListeClient_New_Free(ListeClient *This);
int16_t ListeClient_Push(ListeClient* This, struct Client *c);
struct Client* ListeClient_getNieme(ListeClient* This, uint16_t number);
struct Client* ListeClient_Pop(ListeClient* This);
ListeClient* New_ListeClient();
void ListeClient_Init(ListeClient* This);
struct Client* ListeClient_getFrom(ListeClient *This, struct sockaddr_in from);
Bool ListeClient_remove(ListeClient *This, int sockToFind);

#endif // LISTECLIENT_H
