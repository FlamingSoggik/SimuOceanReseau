#include <stdlib.h>
#include "listeclient.h"

void ListeClient_Init(ListeClient* This){
	This->taille=0;
	This->Top=NULL;
	This->Clear=ListeClient_Clear;
	This->Taille=ListeClient_Taille;
	This->getNieme=ListeClient_getNieme;
	This->Push=ListeClient_Push;
	This->Pop=ListeClient_Pop;
	This->getFromFrom=ListeClient_getFromFrom;
	This->getFromSockNo=ListeClient_getFromSockNo;
	This->remove=ListeClient_remove;
	This->removeAll=ListeClient_removeAll;
	pthread_mutex_init(&This->mutexListeClient, NULL);
}

ListeClient* New_ListeClient(){
	ListeClient* This = malloc(sizeof(ListeClient));
	if (!This) return NULL;
	ListeClient_Init(This);
	This->Free=ListeClient_New_Free;
	return This;
}

void ListeClient_New_Free(ListeClient *This, char freeClients){
	This->Clear(This, freeClients);
	free(This);
}

void ListeClient_Clear(ListeClient *This, char freeClients){
	MaillonListeClient *tmp;
	while(This->Top)
	{
		tmp = This->Top->next;
		if (freeClients == 1){
			This->Top->c->Free(This->Top->c);
		}
		free(This->Top);
		--This->taille;
		This->Top = tmp;
	}
	pthread_mutex_destroy(&This->mutexListeClient);
}

int16_t ListeClient_Push(ListeClient* This, Client *c){
	MaillonListeClient *il = malloc(sizeof(MaillonListeClient));
	if (!il) return ERROR_MALLOC_ITEM;
	il->next=This->Top;
	il->c=c;
	This->Top=il;
	++This->taille;
	return 0;
}

Client* ListeClient_Pop(ListeClient* This){
	if (This->taille == 0)
		return NULL;
	MaillonListeClient *tmp = This->Top->next;
	Client *c = This->Top->c;
	free(This->Top);
	This->Top=tmp;
	--This->taille;
	return c;
}

uint16_t ListeClient_Taille(ListeClient* This){
	return This->taille;
}


Client *ListeClient_getNieme(ListeClient *This, uint16_t number)
{
	if (number > This->taille-1){
		return NULL;
	}
	MaillonListeClient *tmp = This->Top;
	while(number != 0){
		tmp=tmp->next;
		--number;
	}
	return tmp->c;
}


Client* ListeClient_getFromFrom(ListeClient* This, struct sockaddr_in from)
{
	MaillonListeClient *tmp = This->Top;
	while(tmp != NULL){
		if (tmp->c->from.sin_addr.s_addr == from.sin_addr.s_addr && tmp->c->from.sin_port == from.sin_port){
			return tmp->c;
		}
		tmp=tmp->next;
	}
	return NULL;
}

Bool ListeClient_remove(ListeClient *This, int sockToFind)
{
	MaillonListeClient *tmp = This->Top;
	MaillonListeClient *prec = NULL;
	while(tmp != NULL){
		if (tmp->c->socketTCP == sockToFind){
			tmp->c->Free(tmp->c);
			if (prec == NULL){
				This->Top=tmp->next;
			}
			else {
				prec->next=tmp->next;
			}
			--This->taille;
			free(tmp);
			return True;
		}
		prec=tmp;
		tmp=tmp->next;
	}
	return False;
}

Bool ListeClient_removeAll(ListeClient *This)
{
	MaillonListeClient *tmp;
	while(This->Top)
	{
		tmp = This->Top->next;
		This->Top->c->Free(This->Top->c);
		free(This->Top);
		--This->taille;
		This->Top = tmp;
	}
	return True;
}


Client* ListeClient_getFromSockNo(ListeClient* This, int sockNo)
{
	MaillonListeClient *tmp = This->Top;
	while(tmp != NULL){
		if (tmp->c->socketTCP == sockNo){
			return tmp->c;
		}
		tmp=tmp->next;
	}
	return NULL;
}
