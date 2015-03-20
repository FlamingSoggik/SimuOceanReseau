#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void Client_Init(Client *This){
	This->Clear = Client_Clear;
}

void Client_Free(Client *This){
	This->Clear(This);
	puts("Destruction de Client statique");
}

void Client_Clear(Client *This){
	close(This->socketTCP);
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
