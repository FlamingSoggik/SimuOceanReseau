#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void Client_Init(Client *This){
	This->casesTo=New_ListeCase();
	This->Clear = Client_Clear;
	This->posX=0;
	This->posY=0;
}

void Client_Free(Client *This){
	This->Clear(This);
	puts("Destruction de Client statique");
}

void Client_Clear(Client *This){
	close(This->socketTCP);
	This->casesTo->Free(This->casesTo);
}

Client* New_Client(){
	Client *This = malloc(sizeof(Client));
	if(!This) return NULL;
	Client_Init(This);
	This->Free = Client_New_Free;
	return This;
}

void Client_New_Free(Client* This){
	if(This) This->Clear(This);
	free(This);
}
