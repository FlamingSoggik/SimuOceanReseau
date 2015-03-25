#include "elementpont.h"

#include <stdlib.h>
#include <stdio.h>
#include "listetype.h"

#define max(a,b) (a>=b?a:b)

ElementPont* New_ElementPont(Case *c){
	ElementPont* This = malloc(sizeof(ElementPont));
	if (!This) return NULL;
	if (ElementPont_Init(c, This) < 0){
		free(This);
		return NULL;
	}
	This->Free = Element_New_Free;
	return This;
}

char ElementPont_Init(Case *c, ElementPont* This){
	if (c == NULL){
		puts("Erreur creation animal case parent vide");
	}
	This->caseParent=c;
	This->type=PONT;
	This->Clear = Element_Clear;
    This->serialize=Element_serialize;
	return 0;
}

char* ElementPont_serialize(ElementPont *This)
{
	/* Format de la chaine retournée :
	 * <Type>\n
	 */
	char* SerializedThis;

	// 4 : nombre de uint16, 5: nombre de caractère pour un uint16 +1: comptage du retour à la ligne +1 : \0
	SerializedThis=malloc(((5+1)+1)*sizeof(char));
	sprintf(SerializedThis, "%d\n", This->type);
	return SerializedThis;
}
