#ifndef RESEAU_H
#define RESEAU_H

#include <pthread.h>
#include <sys/select.h>

typedef struct Reseau
{
	pthread_t th_ThreadIncommingPlayer;
	pthread_t th_ThreadInternalMessages;
	pthread_t th_ThreadTcp;
	pthread_mutex_t mutexMatricePropriete;
	int sockEcouteIncommingClients;
	int sockEcouteInternalMessages;
	int sockEcouteTcp;
	int portEcouteIncommingClients;
	int portEcouteInternalMessages;
	int portEcouteTcp;
	int nbConnectes;
	fd_set untouchableSet;
	fd_set degradableSet;
	int maxFd;
	void (*Free)(struct Reseau *This);
	void (*Clear)(struct Reseau *This);
} Reseau;

// Constructeur/Destructeur dynamique
Reseau* New_Reseau();
void Reseau_New_Free(struct Reseau*);

//Constructeur/Destructeur statique
Reseau Reseau_Create();
void Reseau_Free(struct Reseau*);

void Reseau_Init(struct Reseau*);
void Reseau_Clear(struct Reseau*This);

void tenterConnection(struct Reseau* This);
int creatIncommingClients(Reseau *This);
void creatEcouteInternalMessages(Reseau *This);
int creatEcouteTcp(Reseau *This);

#endif // RESEAU_H
