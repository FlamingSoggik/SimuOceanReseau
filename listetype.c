#include <stdlib.h>
#include <stdio.h>
#include "listetype.h"

void ListeType_Init(ListeType* This){
	This->taille=0;
	This->Top=NULL;
	This->Push=ListeType_Push;
	This->Pop=ListeType_Pop;
	This->Clear=ListeType_Clear;
	This->Taille=ListeType_Taille;
	This->Contain=ListeType_Contain;
}

ListeType* New_ListeType(){
	ListeType* This = malloc(sizeof(ListeType));
	if (!This) return NULL;
	ListeType_Init(This);
	This->Free=ListeType_New_Free;
	return This;
}

void ListeType_New_Free(ListeType *This){
	This->Clear(This);
	free(This);
}

void ListeType_Clear(ListeType *This){
	MaillonListeType *tmp;
	while(This->Top)
	{
		tmp = This->Top->next;
		free(This->Top);
		--This->taille;
		This->Top = tmp;
	}
}

int16_t ListeType_Push(ListeType* This, Type t){
	MaillonListeType *il = malloc(sizeof(MaillonListeType));
	if (!il) return ERROR_MALLOC_ITEM;
	il->next=This->Top;
	il->t=t;
	This->Top=il;
	++This->taille;
	return 0;
}

Type ListeType_Pop(ListeType* This){
	if (This->taille == 0)
		return VOID;
	MaillonListeType *tmp = This->Top->next;
	Type t = This->Top->t;
	free(This->Top);
	This->Top=tmp;
	--This->taille;
	return t;
}

int16_t ListeType_Taille(ListeType* This){
	return This->taille;
}


Bool ListeType_Contain(ListeType * This, Type t)
{
	if (This->taille == 0)
		return False;
	MaillonListeType *tmp=This->Top;
	while(tmp != NULL)
	{
		if (tmp->t == t)
			return True;
		tmp=tmp->next;
	}
	return False;
}

char* ListeType_serialize(ListeType *This)
{
	/* Format de la chaine retournée :
	 * <nombre d'elements>\n<Type>\n<Type>
	 */
	int offset;
	char* SerializedThis;
	// tableau à double entrée de chaine de caractère contenant les fifférents éléments
	SerializedThis=malloc(((This->taille+1)*(5+1)+1)*sizeof(char));
	if (!SerializedThis) return NULL;
	offset = sprintf(SerializedThis, "%d\n", This->taille);

	MaillonListeType *tmp = This->Top;
	while(tmp != NULL){
		offset += sprintf(SerializedThis+offset, "%d\n", tmp->t);
		tmp=tmp->next;
	}
	return SerializedThis;
}
