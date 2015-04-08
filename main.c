#include <stdio.h>
#include "element.h"
#include "elementanimal.h"
#include "elementpecheur.h"
#include "elementpont.h"
#include "grille.h"
#include <stdlib.h>
#include "affichage.h"
#include <unistd.h>
#include "stringreplace.h"

int main(int argc, char **argv)
{
	Grille *g;
	int nbpecheurs = 0, tailleGrille=0;
	char interface = -1;
	if (argc < 4){
		printf("Taille d'un coté de la grille carré (30 conseillé): ");
		scanf("%d%*c", &tailleGrille);
		printf("Nombre de pecheurs (0 .. 10) : ");		scanf("%d%*c", &nbpecheurs);
		printf("Interface Graphique avec/sans (a/s) : ");
		scanf("%c%*c", &interface);
	} else {
		tailleGrille=atoi(argv[1]);
		nbpecheurs=atoi(argv[2]);
		interface=argv[3][0];
	}
	if (tailleGrille > 80){
		tailleGrille = 80;
		printf("La taille maximum de la grille est 80\n");
	}
	switch (interface){
        case 'a' :
            g = New_Grille(tailleGrille, nbpecheurs);
			g = SDL_Print(g);
//			g->Print(g);
//			if (g->r->carteInitialised == True){
//				pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
//				if (pthread_mutex_lock(&g->r->mutexMatricePropriete)){
//						perror("pthread_mutex_lock");
//						exit(-10);
//				}
//				ListeCase *lc = New_ListeCase();
//				for (i=0; i < g->Taille; ++i){
//					for (j=0; j < g->Taille/2; ++j){
//						lc->Push(lc, &g->tab[i][j]);
//					}
//				}
//				g->r->askForProperty(g->r, lc);
//				lc->Vider(lc);
//				lc->Free(lc);
//				if (pthread_mutex_unlock(&g->r->mutexMatricePropriete)){
//						perror("pthread_mutex_lock");
//						exit(-10);
//				}
//				if (pthread_mutex_lock(&g->r->mutexNbrReponseAttendue) < 0){
//					perror("pthread_mutex_lock");
//					exit(1);
//				}
//				while(g->r->nbrReponseAttendue != 0){
//					pthread_cond_wait(&g->r->condEverythingRecieved, &g->r->mutexNbrReponseAttendue);
//				}
//				g->r->flag=1;
//				if ( pthread_mutex_unlock(&g->r->mutexNbrReponseAttendue) < 0){
//					perror("pthread_mutex_unlock");
//						exit(-10);
//				}
//				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//			}
//			else sleep (1);
//			while(g->TourCourant < 1000){
//				g->faireTour(g, 0);
//				system("clear");
//				g->Print(g);
//				usleep(1000000);
//			}
			g->Free(g);
			break;
		case 's':
			g = New_Grille(tailleGrille, nbpecheurs);
			g->Print(g);
			printf("Lancement du jeu dans 5 secondes. Si la grille ne s'affiche pas correctement, utilisez ctrl+- pour diminuer la taille ou ctrl+maj++ \n");
			sleep(5);
			while(g->TourCourant < 1000){
				g->faireTour(g, 0);
				system("clear");
				g->Print(g);
				usleep(100000);
			}
			g->Free(g);
			break;
		default:printf("Err %c\n", interface);
			return -1;
	}

	return 0;
}

