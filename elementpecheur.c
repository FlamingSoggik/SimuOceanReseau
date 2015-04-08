#include "elementpecheur.h"
#include "stringreplace.h"
#include <stdlib.h>
#include <stdio.h>
#include "math.h"
#include "Bool.h"
#include "elementanimal.h"
#include "elementpont.h"
#include "listetype.h"
#include <time.h>
#include <string.h>

#define TAILLE_CANNE_A_PECHE 2
#define TAILLE_FILET 2
#define COUT_POSE_PONT 3
#define DISTANCE_DEPLACEMENT 2
#define DISTANCE_LANCE_FILET 3
#define max(a,b) (a>=b?a:b)
#define min(a,b) (a>=b?b:a)

ElementPecheur* New_ElementPecheur(Case *c){
	ElementPecheur* This = malloc(sizeof(ElementPecheur));
	if (!This) return NULL;
	if (ElementPecheur_Init(c, This) < 0){
		free(This);
		return NULL;
	}
	This->Free = ElementPecheur_New_Free;
	return This;
}

char ElementPecheur_Init(Case *c, ElementPecheur* This){
	if (c == NULL){
		puts("Erreur creation animal case parent vide");
	}
	This->listeDePeche=New_ListeType();
	This->listeDePeche->Push(This->listeDePeche, BAR);
	This->listeDePeche->Push(This->listeDePeche, THON);
	This->listeDePeche->Push(This->listeDePeche, PYRANHA);
	This->listeDePeche->Push(This->listeDePeche, REQUIN);
	This->listeDePeche->Push(This->listeDePeche, ORQUE);
	This->listeDePeche->Push(This->listeDePeche, BALEINE);
	This->caseParent=c;
//	c->isLocked=True;
	This->PositionInitialeX=This->caseParent->posX;
	This->PositionInitialeY=This->caseParent->posY;
	This->pecheParCanne=ElementPecheur_pecheParCanne;
	This->pecheParCanneSDL=ElementPecheur_pecheParCanneSDL;
	This->pecheParFilet=ElementPecheur_pecheParFilet;
	This->pecheParFiletSDL=ElementPecheur_pecheParFiletSDL;
	This->peutPecher=ElementPecheur_peutPecher;
	This->deplacement=ElementPecheur_deplacement;
	This->construirePont=ElementPecheur_construirePont;
	This->mourir=ElementPecheur_mourir;
	This->reinitSac=ElementPecheur_reinitSac;
	This->type=PECHEUR;
	This->Clear = ElementPecheur_Clear;
	This->sac = COUT_POSE_PONT*5;
	This->longueurCanne=TAILLE_CANNE_A_PECHE;
	This->tailleFilet=TAILLE_FILET;
	This->distanceDeplacement=DISTANCE_DEPLACEMENT;
	This->estSelectionne=0;

	This->GetSac=ElementPecheur_getSac;
	This->SetSac=ElementPecheur_setSac;

	This->GetLongueurCanne=ElementPecheur_getLongueurCanne;
	This->SetLongueurCanne=ElementPecheur_setLongueurCanne;

	This->GetTailleFilet=ElementPecheur_getTailleFilet;
	This->SetTailleFilet=ElementPecheur_setTailleFilet;

	This->GetDistanceDeplacement=ElementPecheur_getDistanceDeplacement;
	This->SetDistanceDeplacement=ElementPecheur_setDistanceDeplacement;

	This->GetPositionInitialeX=ElementPecheur_getPositionInitialex;
	This->SetPositionInitialeX=ElementPecheur_setPositionInitialex;

	This->GetPositionInitialeY=ElementPecheur_getPositionInitialey;
	This->SetPositionInitialeY=ElementPecheur_setPositionInitialey;
	This->testVictory=ElementPecheur_testVictory;
	This->serialize=ElementPecheur_serialize;

	return 0;
}

void ElementPecheur_Clear(Element *This){
	ElementPecheur *p = (ElementPecheur*)This;
	p->listeDePeche->Free(p->listeDePeche);
//	This->caseParent->isLocked=False;
}

void ElementPecheur_New_Free(Element* This){
	ElementPecheur *p = (ElementPecheur*)This;
	if(p) This->Clear(This);
	free(p);
}

void ElementPecheur_pecheParCanne(ElementPecheur *This, char *buffer)
{
	int16_t k=0;
	char *buf1, *buf2;

	buf1 = str_replace(buffer, "68", "9");
	buf2 = str_replace(buf1, "86", "9");
	free(buf1);
	buf1 = str_replace(buf2, "86", "9");
	free(buf2);
	buf2 = str_replace(buf1, "48", "7");
	free(buf1);
	buf1= str_replace(buf2, "84", "7");
	free(buf2);
	buf2 = str_replace(buf1, "42", "1");
	free(buf1);
	buf1 = str_replace(buf2, "24", "1");
	free(buf2);
	buf2 = str_replace(buf1, "62", "3");
	free(buf1);
	buf1 = str_replace(buf2, "26", "3");
	free(buf2);

	int16_t deplX = 0, deplY = 0;
	int16_t nbrDirrectionDonne = strlen(buf1);

	while (k < nbrDirrectionDonne && k < TAILLE_CANNE_A_PECHE){
		switch (buf1[k]) {
			case '1':
				deplX+=1;
				deplY-=1;
				break;
			case '2':
				deplX+=1;
				break;
			case '3':
				deplX+=1;
				deplY+=1;
				break;
			case '4':
				deplY-=1;
				break;
			case '5':
				break;
			case '6':
				deplY+=1;
				break;
			case '7':
				deplX-=1;
				deplY-=1;
				break;
			case '8':
				deplX-=1;
				break;
			case '9':
				deplX=1;
				deplY+=1;
				break;
			default:
				printf("Error, la direction du pecheur doit être un nombre entre 1 et 9 compris\n");
				free(buf1);
				return;
		}
		++k;
	}
	free(buf1);


	ElementAnimal* e;

	if ((double)This->caseParent->posX+deplX < 0 || This->caseParent->posX+deplX > This->caseParent->g->Taille-1 || (double)This->caseParent->posY+deplY < 0 || This->caseParent->posY+deplY > This->caseParent->g->Taille-1){
		return;
	}
	Case *casePeche;
	casePeche=&This->caseParent->g->tab[This->caseParent->posX+deplX][This->caseParent->posY+deplY];

	ListeCase * lc = New_ListeCase();
	/************** Demande de mise à jour du contenu de la case en question **************/
	lc->Push(lc, casePeche);
	This->caseParent->g->r->askForVisibility(This->caseParent->g->r, lc);
	lc->Vider(lc);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	while(This->caseParent->g->r->nbrReponseAttendue > 0){
		pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
	}
	if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_unlock");
			exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	/***************************************************************************************/

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	if (casePeche->liste->HasAnAnimal(casePeche->liste)){
		/************** Demande de mise à jour du contenu de la case en question **************/
		lc->Push(lc, casePeche);
		This->caseParent->g->r->askForProperty(This->caseParent->g->r, lc);
		lc->Vider(lc);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
			perror("pthread_mutex_unlock");
			exit(1);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		while(This->caseParent->g->r->nbrReponseAttendue > 0){
			pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
		}
		if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		/***************************************************************************************/

		if (casePeche->proprietaire == NULL && casePeche->liste->HasAnAnimal(casePeche->liste)){
			e=(ElementAnimal*)casePeche->liste->getAnimal(casePeche->liste);
			if (This->peutPecher(This, e->type) == True){
				This->sac+=e->constantes->taille;
				e->caseParent->liste->deleteElement(e->caseParent->liste, (Element*)e);
			}
		}
	}
//	casePeche->isLocked=False;
	if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	lc->Free(lc);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

Type ElementPecheur_pecheParCanneSDL(ElementPecheur *This, int16_t x, int16_t y)
{
	Type t=VOID;
	if (x < 0 || x > (double)This->caseParent->g->Taille-1 || y < 0 || y > (double)This->caseParent->g->Taille-1)
		return t;
	int16_t deplX = (double)This->caseParent->posX-x;
	int16_t deplY = (double)This->caseParent->posY-y;
	if (deplX < 0)
		deplX*=-1;
	if (deplY < 0)
		deplY*=-1;
	if (deplX > TAILLE_CANNE_A_PECHE || deplY > TAILLE_CANNE_A_PECHE){
		return t;
	}
	ElementAnimal *e;
	Case *casePeche;
	ListeCase *lc = New_ListeCase();

	casePeche=&This->caseParent->g->tab[x][y];

	/************** Demande de mise à jour du contenu de la case en question **************/
	lc->Push(lc, casePeche);
	This->caseParent->g->r->askForVisibility(This->caseParent->g->r, lc);
	lc->Vider(lc);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	while(This->caseParent->g->r->nbrReponseAttendue > 0){
		pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
	}
	if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_unlock");
			exit(-10);
	}
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	/***************************************************************************************/
	//pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_lock");
		exit(-10);
	}

	if (casePeche->proprietaire == NULL && casePeche->liste->HasAnAnimal(casePeche->liste)){
		/************** Demande de mise à jour du contenu de la case en question **************/
		lc->Push(lc, casePeche);
		This->caseParent->g->r->askForProperty(This->caseParent->g->r, lc);
		lc->Vider(lc);
		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
			perror("pthread_mutex_unlock");
			exit(1);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		while(This->caseParent->g->r->nbrReponseAttendue > 0){
			pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
		}
		if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		/***************************************************************************************/
		if (casePeche->liste->HasAnAnimal(casePeche->liste)){
			e=(ElementAnimal*)casePeche->liste->getAnimal(casePeche->liste);
			if (This->peutPecher(This, e->type) == True){
				This->sac+=e->constantes->taille;
                t=e->type;
				e->caseParent->liste->deleteElement(e->caseParent->liste, (Element*)e);

			}
		}
	}

//	casePeche->isLocked=False;
	if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(1);
	}
	lc->Free(lc);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);


    return t;
}

Bool ElementPecheur_peutPecher(ElementPecheur *This, Type t)
{
	if (This->listeDePeche->Contain(This->listeDePeche, t) == True){
		return True;
	}
	return False;
}

void ElementPecheur_pecheParFilet(ElementPecheur *This, char *buffer)
{
	int16_t  k = 0;
	int16_t i, j;
	char *buf1, *buf2;

	buf1 = str_replace(buffer, "68", "9");
	buf2 = str_replace(buf1, "86", "9");
	free(buf1);
	buf1 = str_replace(buf2, "86", "9");
	free(buf2);
	buf2 = str_replace(buf1, "48", "7");
	free(buf1);
	buf1= str_replace(buf2, "84", "7");
	free(buf2);
	buf2 = str_replace(buf1, "42", "1");
	free(buf1);
	buf1 = str_replace(buf2, "24", "1");
	free(buf2);
	buf2 = str_replace(buf1, "62", "3");
	free(buf1);
	buf1 = str_replace(buf2, "26", "3");
	free(buf2);

	int16_t deplX = 0, deplY = 0, compt = 0, noessai = 0;
	int16_t nbrDirrectionDonne = strlen(buf1);

	while (k < nbrDirrectionDonne && k < DISTANCE_LANCE_FILET){
		switch (buf1[k]) {
			case '1':
				deplX+=1;
				deplY-=1;
				break;
			case '2':
				deplX+=1;
				break;
			case '3':
				deplX+=1;
				deplY+=1;
				break;
			case '4':
				deplY-=1;
				break;
			case '5':
				break;
			case '6':
				deplY+=1;
				break;
			case '7':
				deplX-=1;
				deplY-=1;
				break;
			case '8':
				deplX-=1;
				break;
			case '9':
				deplX=1;
				deplY+=1;
				break;
			default:
				printf("Error, la direction du pecheur doit être un nombre entre 1 et 9 compris\n");
				free(buf1);
				return;
		}
		++k;
	}
	free(buf1);


	ListeCase *lc = New_ListeCase();
	ElementAnimal* e;
	Case*** MatriceAccessiblePeche= NULL;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_lock");
			exit(-10);
	}
	MatriceAccessiblePeche=This->caseParent->g->getMatriceVoisins(This->caseParent->g, This->caseParent->posX+deplX, This->caseParent->posY+deplY, This->tailleFilet);
	MatriceAccessiblePeche[This->tailleFilet][This->tailleFilet]=&(This->caseParent->g->tab[This->caseParent->posX+deplX][This->caseParent->posY+deplX]);

	for(i=0;i<2*This->tailleFilet+1.0;++i){
		for(j=0;j<2*This->tailleFilet+1.0;++j){
			if (MatriceAccessiblePeche[i][j] != NULL){
//				if (MatriceAccessiblePeche[i][j]->proprietaire == NULL)
//					MatriceAccessiblePeche[i][j]->isLocked = True;
//				else
					lc->Push(lc, MatriceAccessiblePeche[i][j]);
			}
		}
	}
	while (compt != ( 2*This->tailleFilet+1.0)*(2*This->tailleFilet+1.0) && noessai < 2){
		/************** Demande de mise à jour du contenu de la case en question **************/
		This->caseParent->g->r->askForProperty(This->caseParent->g->r, lc);

		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete)){
				perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		while(This->caseParent->g->r->nbrReponseAttendue > 0){
			pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
		}
		if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
				perror("pthread_mutex_lock");
				exit(-10);
		}
		/***************************************************************************************/
		compt=0;
		for(i=0; i < 2*This->tailleFilet+1.0; ++i){
			for(j=0; j < 2*This->tailleFilet+1.0; ++j){
				if (MatriceAccessiblePeche[i][j] == NULL){
					++compt;
				}
				else if (MatriceAccessiblePeche[i][j]->proprietaire == NULL){
					++compt;
				}
			}
		}
		++noessai;
	}
	lc->Vider(lc);


	for(i=0;i<2*This->tailleFilet+1.0;++i){
		for(j=0;j<2*This->tailleFilet+1.0;++j){
			if (MatriceAccessiblePeche[i][j] != NULL && MatriceAccessiblePeche[i][j]->proprietaire == NULL) {
				if (MatriceAccessiblePeche[i][j]->liste->HasAnAnimal(MatriceAccessiblePeche[i][j]->liste)){
					e=(ElementAnimal*)MatriceAccessiblePeche[i][j]->liste->getAnimal(MatriceAccessiblePeche[i][j]->liste);
					if (This->peutPecher(This, e->type) == True){
						This->sac+=e->constantes->taille;
						e->caseParent->liste->deleteElement(e->caseParent->liste, (Element*)e);
					}
				}
			}
		}
	}
	if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_unlock");
			exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	for (i=0; i<2*This->tailleFilet+1.0;++i){
		free(MatriceAccessiblePeche[i]);
	}
	lc->Free(lc);
	free(MatriceAccessiblePeche);
}

int ElementPecheur_pecheParFiletSDL(ElementPecheur *This, int16_t x, int16_t y)
{
	if (x < 0 || x > (double)This->caseParent->g->Taille-1 || y < 0 || y > (double)This->caseParent->g->Taille-1)
        return 0;
	int16_t deplX = (double)This->caseParent->posX-x;
	int16_t deplY = (double)This->caseParent->posY-y;
	if (deplX < 0)
		deplX*=-1;
	if (deplY < 0)
		deplY*=-1;
	if (deplX > TAILLE_CANNE_A_PECHE || deplY > TAILLE_CANNE_A_PECHE){
        return 0;
	}

    int16_t i, j, compt = 0, noessai = 0, incrementation_sac=0;
	ListeCase *lc = New_ListeCase();
	ElementAnimal* e;
	Case*** MatriceAccessiblePeche= NULL;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_lock");
			exit(-10);
	}
	MatriceAccessiblePeche=This->caseParent->g->getMatriceVoisins(This->caseParent->g, x, y, This->tailleFilet);
	MatriceAccessiblePeche[This->tailleFilet][This->tailleFilet]=&(This->caseParent->g->tab[x][y]);

	for(i=0;i<2*This->tailleFilet+1.0;++i){
		for(j=0;j<2*This->tailleFilet+1.0;++j){
			if (MatriceAccessiblePeche[i][j] != NULL){
//				if (MatriceAccessiblePeche[i][j]->proprietaire == NULL)
//					MatriceAccessiblePeche[i][j]->isLocked = True;
//				else
					lc->Push(lc, MatriceAccessiblePeche[i][j]);
			}
		}
	}

	while (compt != ( 2*This->tailleFilet+1.0)*(2*This->tailleFilet+1.0) && noessai < 2){
		/************** Demande de mise à jour du contenu de la case en question **************/
		This->caseParent->g->r->askForProperty(This->caseParent->g->r, lc);

		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete)){
				perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		while(This->caseParent->g->r->nbrReponseAttendue > 0){
			pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
		}
		if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
				perror("pthread_mutex_lock");
				exit(-10);
		}
		/***************************************************************************************/
		compt=0;
		for(i=0; i < 2*This->tailleFilet+1.0; ++i){
			for(j=0; j < 2*This->tailleFilet+1.0; ++j){
				if (MatriceAccessiblePeche[i][j] == NULL){
					++compt;
				}
				else if (MatriceAccessiblePeche[i][j]->proprietaire == NULL){
					++compt;
				}
			}
		}
		++noessai;
	}
	lc->Vider(lc);


	for(deplX=0;deplX<2*This->tailleFilet+1.0;++deplX){
		for(deplY=0;deplY<2*This->tailleFilet+1.0;++deplY){
			if (MatriceAccessiblePeche[deplX][deplY] != NULL && MatriceAccessiblePeche[deplX][deplY]->proprietaire == NULL) {
				if (MatriceAccessiblePeche[deplX][deplY]->liste->HasAnAnimal(MatriceAccessiblePeche[deplX][deplY]->liste)){
					e=(ElementAnimal*)MatriceAccessiblePeche[deplX][deplY]->liste->getAnimal(MatriceAccessiblePeche[deplX][deplY]->liste);
					if (This->peutPecher(This, e->type) == True){
                        incrementation_sac+=e->constantes->taille;
                        This->sac+=e->constantes->taille;
						e->caseParent->liste->deleteElement(e->caseParent->liste, (Element*)e);
					}
				}
			}
		}
	}
//	for(i=0; i<2*This->tailleFilet+1.0; ++i){
//		for(j=0; j<2*This->tailleFilet+1.0; ++j){
//			if (MatriceAccessiblePeche[i][j] != NULL && MatriceAccessiblePeche[i][j]->proprietaire == NULL)
//				MatriceAccessiblePeche[i][j]->isLocked = False;
//		}
//	}
	if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete)){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	lc->Free(lc);
	for (deplX=0; deplX<2*This->tailleFilet+1.0;++deplX){
		free(MatriceAccessiblePeche[deplX]);
	}
	free(MatriceAccessiblePeche);

    printf("Incre sac : %d\n", incrementation_sac);
    return incrementation_sac;

}

Bool ElementPecheur_deplacement(ElementPecheur *This, char direction)
{
	int16_t deplX = 0, deplY = 0;
	switch (direction) {
		case '1':
			deplX=1;
			deplY=-1;
			break;
		case '2':
			deplX=1;
			break;
		case '3':
			deplX=1;
			deplY=1;
			break;
		case '4':
			deplY=-1;
			break;
		case '5':
			break;
		case '6':
			deplY=1;
			break;
		case '7':
			deplX=-1;
			deplY=-1;
			break;
		case '8':
			deplX=-1;
			break;
		case '9':
			deplX=-1;
			deplY=1;
			break;
		default:
			printf("Error, la direction du pecheur doit être un nombre entre 1 et 9 compris\n");
			return False;
	}
	if ((double)This->caseParent->posX+deplX < 0 || This->caseParent->posX+deplX > This->caseParent->g->Taille-1 || (double)This->caseParent->posY+deplY < 0 || This->caseParent->posY+deplY > This->caseParent->g->Taille-1){
		// Le joueur essaie de sortir de la grille
		// lancer cri de whilem
		return False;
	}
	Case *caseDeplacement;
	caseDeplacement = &This->caseParent->g->tab[This->caseParent->posX+deplX][This->caseParent->posY+deplY];
	ListeCase *lc = New_ListeCase();
	/************** Demande de mise à jour du contenu de la case en question **************/
	lc->Push(lc, caseDeplacement);
	This->caseParent->g->r->askForVisibility(This->caseParent->g->r, lc);
	lc->Vider(lc);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	while(This->caseParent->g->r->nbrReponseAttendue > 0){
		pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
	}
	if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_unlock");
			exit(-10);
	}
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	/***************************************************************************************/
	//pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	/*********************** Si on peux se déplacer sur cette case ***********************/
	if ((caseDeplacement->liste->HasAPont(caseDeplacement->liste) == 1 || caseDeplacement->liste->HasDirt(caseDeplacement->liste) == 1 || (This->caseParent->liste->HasAPont(This->caseParent->liste) == 0 && This->caseParent->liste->HasDirt(This->caseParent->liste) == 0)) && caseDeplacement->liste->HasAPecheur(caseDeplacement->liste) == 0){
		//Il y a un pond et pas de pecheur sur ce pont, on peut s'y déplacer
		// ou alors on est déja dans l'eau donc on peux se déplacer de partout

		lc->Push(lc, caseDeplacement);
		/************** On peut s'y déplacer, on demande donc la propriété **************/
		This->caseParent->g->r->askForProperty(This->caseParent->g->r, lc);
		lc->Vider(lc);
		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_unlock");
			exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		while(This->caseParent->g->r->nbrReponseAttendue > 0){
			pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
		}
		if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_unlock");
			exit(-10);
		}
		/********************************************************************************/

		/*********************** Si on peux se déplacer sur cette case ***********************/
		if (caseDeplacement->proprietaire == NULL && ((caseDeplacement->liste->HasAPont(caseDeplacement->liste) == 1 || caseDeplacement->liste->HasDirt(caseDeplacement->liste) == 1 || (This->caseParent->liste->HasAPont(This->caseParent->liste) == 0 && This->caseParent->liste->HasDirt(This->caseParent->liste) == 0)) && caseDeplacement->liste->HasAPecheur(caseDeplacement->liste) == 0)){
//			This->caseParent->isLocked=False;
			This->caseParent->g->moveFromTo(This->caseParent->g, (Element*)This, This->caseParent->posX+deplX, This->caseParent->posY+deplY);
//			This->caseParent->isLocked=True;
			if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
				perror("pthread_mutex_unlock");
				exit(-10);
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			lc->Free(lc);
			return True;
		}
		/********************* Si on peux PAS se déplacer sur cette case *********************/
		else {
//			This->caseParent->isLocked=False;
			if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
				perror("pthread_mutex_unlock");
				exit(-10);
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			lc->Free(lc);
			printf("%s : ERR1\n", __FUNCTION__);
			return False;
		}
	}
	else {
		//Deplacement impossible
		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
			perror("pthread_mutex_unlock");
			exit(-10);
		}
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//		This->caseParent->isLocked=False;
		lc->Free(lc);
		printf("%s : ERR2\n", __FUNCTION__);
		return False;
	}
}

Bool ElementPecheur_construirePont(ElementPecheur *This, char direction)
{
	if (This->sac < COUT_POSE_PONT){
		printf("Err cout pont\n");
		return False;
	}
	int16_t deplX = 0, deplY = 0;
	switch (direction) {
		case '1':
			deplX=1;
			deplY=-1;
			break;
		case '2':
			deplX=1;
			break;
		case '3':
			deplX=1;
			deplY=1;
			break;
		case '4':
			deplY=-1;
			break;
		case '5':
			break;
		case '6':
			deplY=1;
			break;
		case '7':
			deplX=-1;
			deplY=-1;
			break;
		case '8':
			deplX=-1;
			break;
		case '9':
			deplX=-1;
			deplY=1;
			break;
		default:
			printf("Error, la direction du pecheur doit être un nombre entre 1 et 9 compris\n");
			return False;
	}
	if ((double)This->caseParent->posX+deplX < 0 || This->caseParent->posX+deplX > This->caseParent->g->Taille-1 || (double)This->caseParent->posY+deplY < 0 || This->caseParent->posY+deplY > This->caseParent->g->Taille-1){
		// Le joueur essaie de construire en dehors de la ligne
		// lancer cri de whilem
		return False;
	}
	Case *caseCreationDuPont;
	caseCreationDuPont = &This->caseParent->g->tab[This->caseParent->posX+deplX][This->caseParent->posY+deplY];
	ListeCase *lc = New_ListeCase();
	/************** Demande de mise à jour du contenu de la case en question **************/
	lc->Push(lc, caseCreationDuPont);
	This->caseParent->g->r->askForVisibility(This->caseParent->g->r, lc);
	lc->Vider(lc);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	while(This->caseParent->g->r->nbrReponseAttendue > 0){
		pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
	}
	if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_unlock");
			exit(-10);
	}
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	/***************************************************************************************/
	//pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	if (caseCreationDuPont->liste->HasAPont(caseCreationDuPont->liste) == 0
		&& caseCreationDuPont->liste->HasDirt(caseCreationDuPont->liste) == 0
		&& (caseCreationDuPont->liste->HasAnAnimal(caseCreationDuPont->liste) == 0
			|| (caseCreationDuPont->liste->HasAnAnimal(caseCreationDuPont->liste) == 1
				&& ((ElementAnimal*)caseCreationDuPont->liste->getAnimal(caseCreationDuPont->liste))->constantes->taille < This->caseParent->g->TailleMaxSousPont)
			)
		){
		//Il y a un pond et pas de pecheur sur ce pont, on peut s'y déplacer

		/************** On peut s'y déplacer, on demande donc la propriété **************/
		lc->Push(lc, caseCreationDuPont);
		This->caseParent->g->r->askForProperty(This->caseParent->g->r, lc);
		lc->Vider(lc);
		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_lock");
			exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		while(This->caseParent->g->r->nbrReponseAttendue > 0){
			pthread_cond_wait(&This->caseParent->g->r->condEverythingRecieved, &This->caseParent->g->r->mutexNbrReponseAttendue);
		}
		if ( pthread_mutex_unlock(&This->caseParent->g->r->mutexNbrReponseAttendue) < 0){
			perror("pthread_mutex_unlock");
				exit(-10);
		}
		if (pthread_mutex_lock(&This->caseParent->g->r->mutexMatricePropriete)){
			perror("pthread_mutex_lock");
			exit(-10);
		}
		/********************************************************************************/

		if (caseCreationDuPont->proprietaire == NULL
			&& (caseCreationDuPont->liste->HasAPont(caseCreationDuPont->liste) == 0
				&& caseCreationDuPont->liste->HasDirt(caseCreationDuPont->liste) == 0
				&& (caseCreationDuPont->liste->HasAnAnimal(caseCreationDuPont->liste) == 0
					|| (caseCreationDuPont->liste->HasAnAnimal(caseCreationDuPont->liste) == 1
						&& ((ElementAnimal*)caseCreationDuPont->liste->getAnimal(caseCreationDuPont->liste))->constantes->taille < This->caseParent->g->TailleMaxSousPont)
					)
				)
			){

			caseCreationDuPont->liste->Push(caseCreationDuPont->liste, (Element*)New_ElementPont(caseCreationDuPont));
//			This->caseParent->isLocked=False;
			This->sac-=COUT_POSE_PONT;
			if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
				perror("pthread_mutex_unlock");
				exit(-10);
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			printf("Avant Free %d\n", lc->taille);
			lc->Free(lc);
			printf("%s : Normalement tout s'est bien passé\n", __FUNCTION__);
			return True;
		}
		else {
//			caseCreationDuPont->isLocked=False;
			if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
				perror("pthread_mutex_unlock");
				exit(-10);
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			lc->Free(lc);
			printf("%s : ERR1\n", __FUNCTION__);
			return False;
		}
	}
	else {
//		caseCreationDuPont->isLocked=False;
		printf("%s : ERR2\n", __FUNCTION__);
		if (pthread_mutex_unlock(&This->caseParent->g->r->mutexMatricePropriete) < 0){
			perror("pthread_mutex_unlock");
			exit(-10);
		}
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		lc->Free(lc);
		return False;
	}
}

void ElementPecheur_mourir(ElementPecheur *This)
{
	printf("%s\n", __FUNCTION__);
	This->caseParent->g->reinitPecheur(This->caseParent->g, (Element*)This);
}

void ElementPecheur_reinitSac(ElementPecheur *This)
{
	This->sac=COUT_POSE_PONT*5;
}

char* ElementPecheur_serialize(Element *This)
{
	ElementPecheur* me = (ElementPecheur*) This;
	/* Format de la chaine retournée :
	 * <Type>\n<Variable>\n<Variable>
	 */
	char* SerializedThis;

	// 4 : nombre de uint16, 5: nombre de caractère pour un uint16 +1: comptage du retour à la ligne +1 : \0
	SerializedThis=malloc((6*(5+1)+1)*sizeof(char));
	sprintf(SerializedThis, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n", me->type, me->sac, me->longueurCanne, me->tailleFilet, me->distanceDeplacement, me->PositionInitialeX, me->PositionInitialeY);
	return SerializedThis;
}

uint16_t ElementPecheur_getSac(struct ElementPecheur *This){
	return This->sac;
}

void ElementPecheur_setSac(struct ElementPecheur *This, uint16_t toset){
	This->sac=toset;
}

uint16_t ElementPecheur_getLongueurCanne(struct ElementPecheur *This){
	return This->longueurCanne;
}

void ElementPecheur_setLongueurCanne(struct ElementPecheur *This, uint16_t toset){
	This->longueurCanne=toset;
}

uint16_t ElementPecheur_getTailleFilet(struct ElementPecheur *This){
	return This->tailleFilet;
}

void ElementPecheur_setTailleFilet(struct ElementPecheur *This, uint16_t toset){
	This->tailleFilet=toset;
}

uint16_t ElementPecheur_getDistanceDeplacement(struct ElementPecheur *This){
	return This->distanceDeplacement;
}

void ElementPecheur_setDistanceDeplacement(struct ElementPecheur *This, uint16_t toset){
	This->distanceDeplacement=toset;
}

uint16_t ElementPecheur_getPositionInitialex(struct ElementPecheur *This){
	return This->PositionInitialeX;
}

void ElementPecheur_setPositionInitialex(struct ElementPecheur *This, uint16_t toset){
	This->PositionInitialeX=toset;
}

uint16_t ElementPecheur_getPositionInitialey(struct ElementPecheur *This){
	return This->PositionInitialeY;
}

void ElementPecheur_setPositionInitialey(struct ElementPecheur *This, uint16_t toset){
	This->PositionInitialeY=toset;
}

void ElementPecheur_testVictory(struct ElementPecheur *This, int16_t joueur)
{
	if ((This->PositionInitialeX == This->caseParent->g->Taille-1) && (This->caseParent->posX == 0))
	{
		//Celui la start en bas\n");
		This->caseParent->g->victoire=joueur;
	}

	if ((This->PositionInitialeX == 0) && (This->caseParent->posX == This->caseParent->g->Taille-1))
	{
		//Celui la start en haut\n");
		This->caseParent->g->victoire=joueur;
	}

	if ((This->PositionInitialeY == 0) && (This->caseParent->posY == This->caseParent->g->Taille-1))
	{
		//Celui la start  à gauche \n");
		This->caseParent->g->victoire=joueur;
	}
	if ((This->PositionInitialeY == This->caseParent->g->Taille-1) && (This->caseParent->posY == 0))
	{
		//Celui la start à droite \n");
		This->caseParent->g->victoire=joueur;
	}
}
