#include "element.h"
#include "elementanimal.h"
#include "elementpecheur.h"
#include "elementpont.h"
#include "elementterre.h"
#include <stdio.h>
#include <stdlib.h>

void Element_Init(Case *c, Element *This){
    This->caseParent=c;
	This->Clear = Element_Clear;
    This->serialize=Element_serialize;
	This->type=VOID;
}

void Element_Free(Element *This){
	This->Clear(This);
	puts("Destruction de Element statique");
}

void Element_Clear(Element *This){
	//Ne fais rien mais fera quelque chose si on a dans notre classe des éléments malloc
    This=This;
}

Element* New_Element(Case *c){
	Element *This = malloc(sizeof(Element));
	if(!This) return NULL;
    Element_Init(c, This);
	This->Free = Element_New_Free;
	return This;
}

void Element_New_Free(Element* This){
	if(This) This->Clear(This);
	free(This);
}

char* Element_serialize(Element *This)
{
    if (This->type >= TYPEMINANIMAL && This->type <= TYPEMAXANIMAL){
        ElementAnimal *ea = (ElementAnimal*)This;
        return ea->serialize((Element*)ea);
    }
    else if (This->type == PECHEUR){
        ElementPecheur *ep = (ElementPecheur*)This;
        return ep->serialize((Element*)ep);
    }
    else{
        char* toFill;
        toFill=malloc(((5+1)+1)*sizeof(char));
        sprintf(toFill, "%d\n", This->type);
        return toFill;
    }

	// switch type pour choisir ce que l'on fait

}
