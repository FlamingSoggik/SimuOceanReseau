﻿#include "grille.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "errno.h"
#include "elementanimal.h"
#include "elementpecheur.h"
#include "elementpont.h"
#include "elementterre.h"
#include "element.h"
#include "changermodeterminal.h"
#include "stringreplace.h"

#define EAU "\033[00m"				//NOIR
#define PVOID "\033[48;5;1m"		//ROUGE FONCE
#define PPLANCTON "\033[48;5;2m"	//VERT CLAIR
#define PCORAIL "\033[48;5;9m"		//ROUGE CLAIR
#define PBAR "\033[48;5;184m"		//MARON-JAUNE
#define PTHON "\033[48;5;8m"		//GRIS
#define PPOLLUTION "\033[48;5;7m"	//GRIS TRES CLAIR
#define PPYRANHA "\033[48;5;90m"	//VIOLET
#define PREQUIN "\033[48;5;18m"		//BLEU FONCE
#define PORQUE "\033[48;5;27m"		//BLEU CLAIR
#define PBALEINE "\033[48;5;22m"	//VERT FONCE
#define PPONT "\033[48;5;136m"		//MARRON CLAIR
#define PTERRE "\033[48;5;130m"		//MARRON
#define PPECHEUR "\033[48;5;48m"	//VERT-BLEU
#define NORMAL "\033[00m"			//NOIR
#define SELECTIONE "\033[48;5;231m"	//BLANC
#define PGRAS "\033[01m"

Grille Grille_Create(int16_t Taille, unsigned char nbPecheur){
	Grille g;
	Grille_Init(&g, Taille, nbPecheur);
	g.Free=Grille_Free;
	return g;
}

void Grille_Init(Grille *This, uint16_t Taille, unsigned char nbPecheurs){
	if (nbPecheurs > 2* Taille)
		nbPecheurs = 0;
	static int16_t premierpassage = 1;
	if (premierpassage == 1){
		srand(time(NULL));
		defineConstant();
		premierpassage = 0;
	}
	uint16_t i,j, parcours;
	This->Clear = Grille_Clear;
	This->Print = Grille_Print;
	This->Taille=Taille;
	This->TailleMaxSousPont=10;
	This->TourCourant = 0;
	This->listePredationInitialise = 0;
	This->moveFromTo=Grille_moveFromTo;
	This->getMatriceVoisins=Grille_getMatriceVoisins;
	This->faireTour=Grille_faireTour;
	This->reinitPecheur = Grille_reinitPecheur;
	This->detruirePont=Grille_detruirePont;
	This->tab=malloc(sizeof(Case*)*Taille);
	This->nbPecheur = nbPecheurs;
	This->victoire=-1;
	This->NbrCasesToMe=Taille*Taille;
	This->serializeMesCases=Grille_serializeMesCases;
	for (i=0;i<Taille;++i){
		This->tab[i]=malloc(sizeof(Case)*Taille);
	}
	for(i=0;i<Taille;++i){
		for(j=0; j<Taille; ++j){
			This->tab[i][j]= Case_Create(This, i, j);
			if (j == 0 || j == Taille-1 || i == 0 || i == Taille-1){
				This->tab[i][j].liste->Push(This->tab[i][j].liste, (Element*)New_ElementTerre(&This->tab[i][j]));
			}
		}
	}


	remplirListePredation(This);

	This->r=New_Reseau(This);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->r->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	if (This->r->carteInitialised == False){
		//Cas ou on a pas d'autre joueurs en jeu
		This->tabPecheur=malloc(This->nbPecheur*sizeof(ElementPecheur*));
		int16_t nbrCase = (Taille-2)*(Taille-2);
		int16_t nbrEspece = 0;
		int16_t comptelemespece = 0;
		Type compteurTypeEspece = TYPEMIN;


		while (compteurTypeEspece < TYPEMAX){
			comptelemespece = 0;
			switch(compteurTypeEspece){
				case PLANCTON:
					nbrEspece = 40*nbrCase/100;
					break;
				case CORAIL:
					nbrEspece = 2*nbrCase/100;
					break;
				case BAR:
					nbrEspece = 5*nbrCase/100;
					break;
				case THON:
					nbrEspece = 5*nbrCase/100;
					break;
				case PYRANHA:
					nbrEspece = 2*nbrCase/100;
					break;
				case REQUIN:
					nbrEspece = 4*nbrCase/100;
					break;
				case ORQUE:
					nbrEspece = 2*nbrCase/100;
					break;
				case BALEINE:
					nbrEspece = 5*nbrCase/100;
					break;
				default:
					nbrEspece = 0;
					break;
			}
			while (comptelemespece != nbrEspece){
				i = rand()%Taille;
				j = rand()%Taille;
				if (This->tab[i][j].liste->HasAnAnimal(This->tab[i][j].liste) == 0 && This->tab[i][j].liste->HasDirt(This->tab[i][j].liste) == 0){
					This->tab[i][j].liste->Push(This->tab[i][j].liste, (Element*)New_ElementAnimal(&This->tab[i][j], compteurTypeEspece));
					++comptelemespece;
				}
			}
			++compteurTypeEspece;
		}
	}

	Bool flag;
	int ret;
//	struct timespec ts;
//	ts.tv_sec = 2;
//	ts.tv_nsec = 0;
	This->tabPecheur=malloc(This->nbPecheur*sizeof(ElementPecheur*));
	for(parcours=0; parcours<This->nbPecheur; ++parcours){
		flag = False;
		while (flag == False){
			if (rand()%2){
				if (rand()%2){
					j=This->Taille-1;
				}
				else {
					j=0;
				}
				i=rand()%This->Taille;
			}
			else {
				if (rand()%2){
					i=This->Taille-1;
				}
				else {
					i=0;
				}
				j=rand()%This->Taille;
			}
			if(This->tab[i][j].liste->HasAPecheur(This->tab[i][j].liste) == True)
				continue;

			ListeCase* lc = New_ListeCase();
			lc->Push(lc, &This->tab[i][j]);
			This->r->askForProperty(This->r, lc);
			lc->Free(lc);
			if (pthread_mutex_unlock(&This->r->mutexMatricePropriete) < 0){
				perror("pthread_mutex_unlock");
				exit(-10);
			}
			if (pthread_mutex_lock(&This->r->mutexNbrReponseAttendue) < 0){
				perror("pthread_mutex_lock");
				exit(1);
			}
			while(This->r->nbrReponseAttendue != 0){
				pthread_cond_wait(&This->r->condEverythingRecieved, &This->r->mutexNbrReponseAttendue);
			}
			if ( (ret = pthread_mutex_unlock(&This->r->mutexNbrReponseAttendue)) < 0){
				perror("pthread_mutex_unlock");
				exit(-10);
			}
			if (pthread_mutex_lock(&This->r->mutexMatricePropriete)){
				perror("pthread_mutex_lock");
				exit(-10);
			}
			if (This->tab[i][j].proprietaire != NULL){
				printf("Je n'ai pas eu la propriété :(\n");
				continue;
			}
			else {
				This->tab[i][j].liste->Push(This->tab[i][j].liste, (Element*)New_ElementPecheur(&This->tab[i][j]));
				ElementPecheur* p =(ElementPecheur*)This->tab[i][j].liste->getPecheur(This->tab[i][j].liste);
				This->tabPecheur[parcours]=p;
				flag=True;
			}
		}
	}
	if (pthread_mutex_unlock(&This->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (This->nbPecheur != 0)
		This->r->sendPos(This->r, This->tabPecheur[0]);
}

void Grille_Clear(struct Grille *This){
	uint16_t i, j;

	free(This->tabPecheur);
	for(i=0;i<This->Taille;++i){
		for(j=0; j<This->Taille; ++j){
			This->tab[i][j].Free(&(This->tab[i][j]));
		}
	}
	for(i=0; i<This->Taille; ++i){
		free(This->tab[i]);
	}
	free(This->tab);
	viderListePredation(This);
	This->r->Free(This->r);
}

void Grille_Print(struct Grille *This){
	printf(EAU "EAU" NORMAL " " PVOID "RIEN(erreur)" NORMAL " " PPLANCTON "PLANCTON" NORMAL " " PCORAIL "CORAIL" NORMAL " " PBAR "BAR" NORMAL " " PTHON "THON" NORMAL " " PPOLLUTION "POLLUTION" NORMAL " " PPYRANHA "PYRANHA" NORMAL " " PREQUIN "PREQUIN" NORMAL " " PORQUE "ORQUE" NORMAL " " PBALEINE "BALEINE" NORMAL "\n");
	uint16_t i, j;
	for(i=0;i<This->Taille;++i)
		printf("=====" NORMAL);
	printf("=\n");
	for(i=0;i<This->Taille;++i){
		printf("|");
		for(j=0; j<This->Taille; ++j){
			if (This->tab[i][j].liste->HasAPecheur(This->tab[i][j].liste)) {
				if (This->tab[i][j].liste->HasAPont(This->tab[i][j].liste)){
					if (((ElementPecheur*)This->tab[i][j].liste->getPecheur(This->tab[i][j].liste))->estSelectionne == 1)
						printf(SELECTIONE " " PPONT " " PPECHEUR " " SELECTIONE " " NORMAL "|");
					else
						printf(" " PPONT " " PPECHEUR " " NORMAL " |");
				}
				else if(This->tab[i][j].liste->HasDirt(This->tab[i][j].liste)){
					if (((ElementPecheur*)This->tab[i][j].liste->getPecheur(This->tab[i][j].liste))->estSelectionne == 1)
						printf(SELECTIONE " " PTERRE " " PPECHEUR " " SELECTIONE " " NORMAL "|");
					else
						printf(" " PTERRE " " PPECHEUR " " NORMAL " |");
				}
				else {
					if (((ElementPecheur*)This->tab[i][j].liste->getPecheur(This->tab[i][j].liste))->estSelectionne == 1)
						printf(SELECTIONE " " NORMAL " " PPECHEUR " " SELECTIONE " " NORMAL "|");
					else printf("  " PPECHEUR " " NORMAL " |");
				}
			}
			else if (This->tab[i][j].liste->HasAPont(This->tab[i][j].liste)){
				printf(" " PPONT "  " NORMAL " |");
			}
			else if (This->tab[i][j].liste->HasDirt(This->tab[i][j].liste)){
				printf(" " PTERRE "  " NORMAL " |");
			}
			else if (This->tab[i][j].liste->HasAnAnimal(This->tab[i][j].liste)){
				switch(This->tab[i][j].liste->getAnimal(This->tab[i][j].liste)->type){
					case VOID:
						printf(" " PVOID "  " NORMAL " |");
						break;
					case PLANCTON:
						printf(" " PPLANCTON "  " NORMAL " |");
						break;
					case CORAIL:
						printf(" " PCORAIL "  " NORMAL " |");
						break;
					case BAR:
						printf(" " PBAR "  " NORMAL " |");
						break;
					case THON:
						printf(" " PTHON "  " NORMAL " |");
						break;
					case POLLUTION:
						printf(" " PPOLLUTION "  " NORMAL " |");
						break;
					case PYRANHA:
						printf(" " PPYRANHA "  " NORMAL " |");
						break;
					case REQUIN:
						printf(" " PREQUIN "  " NORMAL " |");
						break;
					case ORQUE:
						printf(" " PORQUE "  " NORMAL " |");
						break;
					case BALEINE:
						printf(" " PBALEINE "  " NORMAL " |");
						break;
					default:
						printf(" " NORMAL "ER" NORMAL " |");
						break;
				}
			}
			else {
				printf(" " EAU "  " NORMAL " |");
			}
		}
		printf("\n");
		if (i == This->Taille-1){
			for (j=0; j<This->Taille; j++)
				printf("=====");
			printf("=\n");
		}
		else {
			for (j=0; j<This->Taille; j++)
				printf("—————");
			printf("—\n");
		}
	}



	printf("\n");
	printf("\n");
	printf("\n");
	for(i=0;i<This->Taille;++i)
		printf("=====" NORMAL);
	printf("=\n");
	for(i=0;i<This->Taille;++i){
		printf("|");
		for(j=0; j<This->Taille; ++j){
			if (This->tab[i][j].proprietaire == NULL){
				printf(" " PREQUIN "  " NORMAL " |");
			}
			else {
				printf(" " PPONT "  " NORMAL " |");
			}
		}
		printf("\n");
		if (i == This->Taille-1){
			for (j=0; j<This->Taille; j++)
				printf("=====");
			printf("=\n");
		}
		else {
			for (j=0; j<This->Taille; j++)
				printf("—————");
			printf("—\n");
		}
	}
}

void Grille_Free(struct Grille *This){
	This->Clear(This);
}

void Grille_New_Free(struct Grille *This){
	if(This != NULL) This->Clear(This);
	free(This);
}

Case*** Grille_getMatriceVoisins(Grille *This, uint16_t posX, uint16_t posY, uint16_t nbSauts)
{
	//printf("Taille de la grille : %d\n", This->Taille);
	uint16_t taille=2*nbSauts+1;
	//printf("Taille du nouveau tableau : %d\n", taille);
	//Coordonnées de la case du milieu du nouveau tableau
	uint16_t cMNT = nbSauts;
	//printf("Centre de la Matrice à renvoyer : %d\n", cMNT);
	Case* **tableau = malloc(sizeof(Case**)*taille);
	if (tableau == NULL){
		puts("ERROR creating matrice in detMatriceVoicins");
		return NULL;
	}
	uint16_t i;
	for(i=0;i<taille;++i){
		tableau[i] = malloc(sizeof(Case*)*taille);
	}
	tableau[cMNT][cMNT]=NULL;
	int16_t j, k;
	for(i=nbSauts;i>0;--i){
		//printf("Nous somme dans la boucle pour le saut de %d cases\n", i);
		for(j=posX-i,k=0;j<(double)posX+i+1;++j,++k){
			//printf("J, qui correspond à la valeur de x qui varie afin de completer la première et dernière ligne vaut %d\n", j);
			if (j < 0 || j > (double)This->Taille-1 || (double)posY-i < 0 || (double)posY-i > This->Taille-1){
				//printf("La position (ligne superieure) évaluée est en dehors de la Grille ([%d][%d]), la valeur null est placée dans la case [%d][%d]\n", j, posY-i, cMNT-i+k, cMNT-i);
				tableau[cMNT-i+k][cMNT-i]= NULL;
			} else {
				//printf("La position (ligne superieure) évaluée est dans la Grille, l'adresse de la case correspondante ([%d][%d]) est placée dans la case [%d][%d]\n", j, posY-i, cMNT-i+k, cMNT-i);
				tableau[cMNT-i+k][cMNT-i]=&This->tab[j][posY-i];
			}
			//printf("\n\n");
			if (j < 0 || j > (double)This->Taille-1 || (double)posY+i < 0 || (double)posY+i > This->Taille-1){
				//printf("La position (ligne inferieure) évaluée est en dehors de la Grille ([%d][%d]), la valeur null est placée dans la case [%d][%d]\n", j, posY+i, cMNT-i+k, cMNT+i);
				tableau[cMNT-i+k][cMNT+i]= NULL;
			} else {
				//printf("La position (ligne inferieure) évaluée est dans la Grille, l'adresse de la case correspondante ([%d][%d]) est placée dans la case [%d][%d]\n\n\n\n", j, posY+i, cMNT-i+k, cMNT+i);
				tableau[cMNT-i+k][cMNT+i]=&This->tab[j][posY+i];
			}
		}

		for(j=posY-i+1,k=1;j<(double)posY+i-1+1;++j,++k){
			//printf("J, qui correspond à la valeur de y qui varie afin de completer la première et dernière colonne vaut %d\n", j);
			if (j < 0 || j > (double)This->Taille-1 || (double)posX-i < 0 || (double)posX-i > This->Taille-1){
				//printf("La position (ligne superieure) évaluée est en dehors de la Grille ([%d][%d]), la valeur null est placée dans la case [%d][%d]\n", posX-i, j, cMNT-i, cMNT-i+k);
				tableau[cMNT-i][cMNT-i+k]= NULL;
			} else {
				//printf("La position (ligne superieure) évaluée est dans la Grille, l'adresse de la case correspondante ([%d][%d]) est placée dans la case [%d][%d]\n", posX-i, j, cMNT-i, cMNT-i+k);
				tableau[cMNT-i][cMNT-i+k]=&This->tab[posX-i][j];
			}
			//printf("\n\n");
			if (j < 0 || j > (double)This->Taille-1 || (double)posX+i < 0 || (double)posX+i > This->Taille-1){
				//printf("La position (ligne inferieure) évaluée est en dehors de la Grille ([%d][%d]), la valeur null est placée dans la case [%d][%d]\n", posX+i, j, cMNT+i, cMNT-i+k);
				tableau[cMNT+i][cMNT-i+k]= NULL;
			} else {
				//printf("La position (ligne inferieure) évaluée est dans la Grille, l'adresse de la case correspondante ([%d][%d]) est placée dans la case [%d][%d]\n\n\n\n", posX+i, j, cMNT+i, cMNT-i+k);
				tableau[cMNT+i][cMNT-i+k]=&This->tab[posX+i][j];
			}
		}


	}
	return tableau;
}

void Grille_moveFromTo(Grille *This, Element *elem, uint16_t posX, uint16_t posY)
{
	This->tab[elem->caseParent->posX][elem->caseParent->posY].liste->remove(This->tab[elem->caseParent->posX][elem->caseParent->posY].liste, elem);
	This->tab[posX][posY].liste->Push(This->tab[posX][posY].liste, elem);
	elem->caseParent=&(This->tab[posX][posY]);
}

void Grille_reinitPecheur(Grille *This, Element *elem)
{
	ElementPecheur* pecheur = (ElementPecheur*)elem;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	This->tab[pecheur->caseParent->posX][pecheur->caseParent->posY].liste->remove(This->tab[pecheur->caseParent->posX][pecheur->caseParent->posY].liste, (Element*)pecheur);
	char sensDeTest = 'b';
	int16_t depl = 1;
	Case *caseInitiale = &This->tab[pecheur->PositionInitialeX][pecheur->PositionInitialeY];
	ListeCase *lc = New_ListeCase();

recommencer:
	/************** Demande de mise à jour du contenu de la case en question **************/
	lc->Push(lc, caseInitiale);
	This->r->askForProperty(This->r, lc);
	lc->Vider(lc);
	if (pthread_mutex_unlock(&This->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(1);
	}
	if (pthread_mutex_lock(&This->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}
	while(This->r->nbrReponseAttendue > 0){
		pthread_cond_wait(&This->r->condEverythingRecieved, &This->r->mutexNbrReponseAttendue);
	}
	if ( pthread_mutex_unlock(&This->r->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_unlock");
			exit(-10);
	}
	if (pthread_mutex_lock(&This->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_lock");
		exit(1);
	}

	/***************************************************************************************/
	if (caseInitiale->proprietaire != NULL || caseInitiale->liste->HasAPecheur(caseInitiale->liste) == 1){
		if (sensDeTest == 'b'){
			if (pecheur->PositionInitialeX+depl > This->Taille-1){
				sensDeTest='h';
				depl=1;
			}
			else {
				caseInitiale=&This->tab[pecheur->PositionInitialeX+1][pecheur->PositionInitialeY];
				++depl;
			}
		}
		else {
			if ((double)pecheur->PositionInitialeX-depl < 0){
				printf("Impossible de placer le pecheur --> perdu !!\n");
				return;
			}
			else {
				caseInitiale=&This->tab[pecheur->PositionInitialeX-1][pecheur->PositionInitialeY];
				++depl;
			}
		}
		goto recommencer;
	}
	This->moveFromTo(This, (Element*)pecheur, caseInitiale->posX, caseInitiale->posY);
	//caseInitiale->liste->Push(caseInitiale->liste, (Element*)pecheur);
	pecheur->reinitSac(pecheur);
	if (pthread_mutex_unlock(&This->r->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(1);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

void Grille_faireTour(Grille *This, char isSdl){
	uint16_t i, j;
	ElementAnimal *e;
	ElementPecheur *p;
	for (i=0; i<This->Taille; ++i){
		for (j=0; j<This->Taille; ++j){
			if (This->tab[i][j].proprietaire == NULL && This->tab[i][j].liste->HasAnAnimal(This->tab[i][j].liste)){
				e=(ElementAnimal*)This->tab[i][j].liste->getAnimal(This->tab[i][j].liste);
				if (e->doitJouerCeTour(e) == False)
					continue;
				if (e->survie(e) == False){
					This->tab[i][j].liste->deleteElement(This->tab[i][j].liste, (Element*)e);;
				}
				else {
					e->reproduction(e);
					e->predation(e);
					e->deplacement(e);
					e->tour(e);
					e->aFaitSonTour(e);
				}
			}
			else if (This->tab[i][j].proprietaire == NULL){
			}
			e=NULL;
		}
	}
	if (isSdl == 0){
		if (This->TourCourant != 0 && This->TourCourant%5 == 0){
			for (i=0;i<This->nbPecheur; ++i){
				char buff[20]={'\0'};
				p=This->tabPecheur[i];
				p->estSelectionne=1;
				system("clear");
				This->Print(This);
				printf("Choisisez une action construire (c), deplacement (d) ou pecher(p)\n");
				mode_raw(1);
				char c = getchar();
				mode_raw(0);
				if (c == 'c'){
					printf("Choisisez la dirrection (123456789)\n");
					mode_raw(1);
					c = getchar();
					mode_raw(0);
					p->construirePont(p, c);
				}
				else if (c == 'd'){
					printf("Choisisez la dirrection (123456789)\n");
					mode_raw(1);
					c = getchar();
					mode_raw(0);
					p->deplacement(p, c);
					p->testVictory(p, i);
					p->caseParent->g->r->sendPos(p->caseParent->g->r, p);
				}
				else if (c == 'p'){
					printf("Choisisez le type de peche : Peche par canne (c) ou par filet(f)\n");
					mode_raw(1);
					c = getchar();
					mode_raw(0);
					if (c == 'f'){
						printf("Entrez le chemin pour la case ou lancer le filet (123456789) et validez avec la touche entrée\n");
						if (fgets(buff, sizeof(buff), stdin) == NULL){
							break;
						}
						uint16_t longueurChaine=strlen(buff);
						if (longueurChaine == 0)
							break;
						buff[longueurChaine-1]='\0';
						p->pecheParFilet(p, buff);
					}
					else if (c == 'c'){
						printf("Entrez le chemin pour acceder à l'element à pecher (123456789) et validez avec la touche entrée\n");
						if (fgets(buff, sizeof(buff), stdin) == NULL){
							break;
						}
						uint16_t longueurChaine=strlen(buff);
						if (longueurChaine == 0)
							break;
						buff[longueurChaine-1]='\0';
						p->pecheParCanne(p, buff);
					}
				}
				p->estSelectionne=0;
				system("clear");
				This->Print(This);
			}
		}
	}
	for (i=0; i<This->Taille; ++i){
		for (j=0; j<This->Taille; ++j){
			if (This->tab[i][j].liste->HasAnAnimal(This->tab[i][j].liste)){
				e=(ElementAnimal*)This->tab[i][j].liste->getAnimal(This->tab[i][j].liste);
				e->finDuTour(e);
			}
		}
	}
	++This->TourCourant;
}

Grille *New_Grille(int16_t Taille, unsigned char nbPecheurs)
{
	Grille* This = malloc(sizeof(Grille));
	if(!This) return NULL;
	Grille_Init(This, Taille, nbPecheurs);
	This->Free=Grille_New_Free;
	return This;
}


void Grille_detruirePont(Grille *This, struct Case *c)
{
	if (This->tab[c->posX][c->posY].liste->HasAPont(This->tab[c->posX][c->posY].liste) == False){
		return;
	}
	Element* p = This->tab[c->posX][c->posY].liste->getPont(This->tab[c->posX][c->posY].liste);
	This->tab[c->posX][c->posY].liste->deleteElement(This->tab[c->posX][c->posY].liste, p);
}


char* Grille_serializeMesCases(Grille *This)
{
	/* Format de la chaine retournée :
	 * <nombre de Cases>\n<Case>\n<Case>
	 */
	int i, j, k, offset;
	char* SerializedThis;
	// tableau à double entrée de chaine de caractère contenant les différents éléments
	char **leschaines = malloc(This->NbrCasesToMe*sizeof(char*));
	if (!leschaines) return NULL;
	unsigned int nbrCaract=0;

	k=0;
	for(i=0;i<This->Taille;++i){
		for(j=0;j<This->Taille;++j){
			if (This->tab[i][j].proprietaire == NULL){
				leschaines[k]=This->tab[i][j].serialize(&This->tab[i][j]);
				nbrCaract+=strlen(leschaines[k]);
				++k;
			}
		}
	}

	SerializedThis=malloc((nbrCaract+(5+1)+1+1+3)*sizeof(char));
	offset = sprintf(SerializedThis, "#3a%d\n", This->NbrCasesToMe);

	for(i=0; i<This->NbrCasesToMe; ++i){
		offset += sprintf(SerializedThis+offset, "%s", leschaines[i]);
		free(leschaines[i]);
	}
	offset += sprintf(SerializedThis+offset, "#");
	free(leschaines);
	return SerializedThis;
}
