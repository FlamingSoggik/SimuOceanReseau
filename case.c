#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "case.h"

Case Case_Create(Grille *g, uint16_t posX, uint16_t posY)
{
	Case This;
    This.g = g;
    This.posX=posX;
    This.posY=posY;
    This.proprietaire=NULL;
	This.liste = New_ListeElem();
	This.Free=Case_Free;
	This.Clear=Case_Clear;
	This.Print=Case_Print;
    This.serialize=Case_serialize;
	return This;
}

void Case_Free(Case *This){
	This->Clear(This);
}

void Case_Clear(Case *This){
	This->liste->Free(This->liste);
}

void Case_Print(Case *This){
	This->liste->Print(This->liste);
}

char* Case_serialize(Case *This)
{
//printf("Case_serialized ");
	char* SerializedThis;
	char* ListeElemSerialized = This->liste->serialize(This->liste);
	// 4 : nombre de uint16, 5: nombre de caractère pour un uint16 +1: comptage du retour à la ligne +1 : \0
	SerializedThis=malloc((2*(5+1)+strlen(ListeElemSerialized)+1)*sizeof(char));
//printf("J'envoie : posX:%d posY:%d\nMon contenu est \n%s", This->posX, This->posY, ListeElemSerialized);
	sprintf(SerializedThis, "%d\n%d\n%s", This->posX, This->posY, ListeElemSerialized);
	free(ListeElemSerialized);
	return SerializedThis;
}
