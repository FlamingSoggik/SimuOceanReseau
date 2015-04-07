#ifndef RESEAU_H
#define RESEAU_H

#include <pthread.h>
#include <sys/select.h>
#include "client.h"
#include "listeclient.h"
#include "listecase.h"
#include "Bool.h"

struct Client;
struct ListeCase;
struct ElementPecheur;

typedef struct Reseau
{
	pthread_t th_ThreadIncommingPlayer;
    pthread_t th_ThreadInternalMessages;
    pthread_t th_ThreadTcp;
    pthread_t th_AnswerTimeout4;
	pthread_mutex_t mutexMatricePropriete;
	pthread_mutex_t mutexNbrReponseAttendue;
	pthread_cond_t condEverythingRecieved;
	int sockEcouteIncommingClients;
	int sockEcouteInternalMessages;
	int sockEcouteTcp;
	int portEcouteIncommingClients;
	int portEcouteInternalMessages;
	int portEcouteTcp;
	fd_set untouchableSet;
	fd_set degradableSet;
	int maxFd;
	int flag;
	Bool carteInitialised;
    int selfPipe[2];
	unsigned char nbrReponseAttendue;
    struct ListeClient* clients;
	void (*Free)(struct Reseau *This);
	void (*Clear)(struct Reseau *This);
    struct Grille *g;

	void (*askForProperty)(struct Reseau *This, struct ListeCase* lcas);
	char* (*giveProperty)(struct Reseau *This, char* str, struct Client *cli);
	Bool (*recupProperty)(struct Reseau* This, char* str, struct Client*cli);

	void (*askForVisibility)(struct Reseau *This, struct ListeCase* lcas);
	char* (*giveVisibility)(struct Reseau *This, char* str);
	Bool (*recupVisibility)(struct Reseau* This, char* str, struct Client* cli);

	void (*sendWin)(struct Reseau *This);
	void (*sendPos)(struct Reseau *This, struct ElementPecheur*);


} Reseau;

// Constructeur/Destructeur dynamique
Reseau* New_Reseau(struct Grille *g);
void Reseau_New_Free(struct Reseau*);

//Constructeur/Destructeur statique
Reseau Reseau_Create();
void Reseau_Free(struct Reseau*);

void Reseau_Init(struct Reseau*, struct Grille *g);
void Reseau_Clear(struct Reseau*This);

void tenterConnection(struct Reseau* This);
int creatIncommingClients(Reseau *This);
int creatEcouteInternalMessages(Reseau *This);
int creatEcouteTcp(Reseau *This);

void unSerialize(Reseau* This, char* str, struct Client *cli);
void askForCarte(Reseau *This);

void Reseau_askForProperty(Reseau *This, struct ListeCase* lcas);
char* Reseau_giveProperty(Reseau *This, char* str, struct Client *cli);
Bool Reseau_recupProperty(Reseau* This, char* str, struct Client*cli);

void Reseau_askForVisibility(Reseau *This, struct ListeCase* lcas);
char* Reseau_giveVisibility(Reseau *This, char* str);
Bool Reseau_recupVisibility(Reseau* This, char* str, struct Client* cli);

void Reseau_sendWin(struct Reseau *This);
Bool Reseau_recupCoordinatesEnnemy(Reseau* This, char* str, struct Client* cli);
void Reseau_sendPos(Reseau* This, struct ElementPecheur* p);

#endif // RESEAU_H
