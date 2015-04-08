#include <sys/types.h> //--> Non nécessaire sous linux > 2001 mais utile sur vieilles impémentations BSD
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <strings.h>
#include <net/if.h>
#include <sys/errno.h>
#include <inttypes.h>

#include "reseau.h"
#include "element.h"
#include "elementanimal.h"
#include "elementpecheur.h"
#include "elementterre.h"
#include "elementpont.h"

void* HandleIncommingPlayer(void *arg){
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	Reseau* This = (Reseau*)arg;
	// Le thread pourra être interrompu à tout moment (notamment dans le recvfrom bloquant)
	struct sockaddr_in from;
	struct sockaddr_in dst;
	struct hostent *hp;

	int sizeRecieved;
	char recvBuffer[20], sendbuffer[20];
	unsigned int fromlen=sizeof(from);

	for(;;){
		sizeRecieved=recvfrom(This->sockEcouteIncommingClients, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&from, &fromlen);
		if (sizeRecieved <= 0){
			if (errno == EINTR){
				continue;
			}
			perror("recvfrom");
			exit(1);
		}
		else if (sizeRecieved < 3){
			printf("Continue car ordre trop court\n");
			continue;
		}
		if (strncmp(recvBuffer, "#1q", 3) == 0){
			hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
			//printf("Demande de connection recue de la part de %s:%d\n",inet_ntoa(from.sin_addr),htons(from.sin_port));

			sprintf(sendbuffer, "#1a\n%d\n%d\n", This->portEcouteTcp, This->portEcouteInternalMessages);

			dst.sin_addr=from.sin_addr;
			dst.sin_port=from.sin_port;
			dst.sin_family=AF_INET;

			sendto(This->sockEcouteIncommingClients, sendbuffer, strlen(sendbuffer)+1, 0, (struct sockaddr *) &dst, sizeof(dst));
		}
		else {
			printf("Unknown message:\n");

			recvBuffer[sizeRecieved]='\0';
			printf("*%s*\n",recvBuffer);
			hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
			printf("...de la part de %s(%d)\n",inet_ntoa(from.sin_addr),htons(from.sin_port));

			dst.sin_addr=from.sin_addr;
			dst.sin_port=from.sin_port;
			dst.sin_family=AF_INET;

			//sendto(This->sockEcouteIncommingClients, "#1#", 3, 0, (struct sockaddr *) &dst, sizeof(dst));
		}
	}
	//Anti warning trick
	hp=hp;
	pthread_exit(NULL);
}

void* HandleInternalMessage(void *arg){
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	Reseau* This = (Reseau*)arg;
	// Le thread pourra être interrompu à tout moment (notamment dans le recvfrom bloquant)
	struct sockaddr_in from;
	struct sockaddr_in dst;
	struct hostent *hp;

	char* toSend;
	int sizeRecieved;
	char recvBuffer[65535];
	unsigned int fromlen=sizeof(from);

	for(;;){
		sizeRecieved=recvfrom(This->sockEcouteInternalMessages, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&from, &fromlen);
		if (sizeRecieved <= 0){
			if (errno == EINTR){
				continue;
			}
			perror("recvfrom");
			exit(1);
		}
		else if (sizeRecieved < 3){
			printf("Order too small\n");
			continue;
		}
		hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
		//Anti warning trick
		hp=hp;
		dst.sin_addr=from.sin_addr;
		dst.sin_port=from.sin_port;
		dst.sin_family=AF_INET;
		// On recoit une demande de toute notre carte : #3q (q comme question)
		if (strncmp(recvBuffer, "#3q", 3) == 0){
			printf("Demande de don de carte de la part de %s:%d\n",inet_ntoa(from.sin_addr),htons(from.sin_port));
			toSend=This->g->serializeMesCases(This->g);
			sendto(This->sockEcouteInternalMessages, toSend, strlen(toSend)+1, 0, (struct sockaddr *) &dst, sizeof(dst));
			free(toSend);
		}
		else if (strncmp(recvBuffer, "#3a", 3) == 0){
			printf("Reception de la carte la part de %s:%d\n",inet_ntoa(from.sin_addr),htons(from.sin_port));
			Client *c;
			c = This->clients->getFromFrom(This->clients, from);
			if (c == NULL){
				printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			}
			unSerialize(This, recvBuffer+3, c);
			This->carteInitialised=True;
			printf("FIN UNSERIALIZED\n");
		}
		else if (strncmp(recvBuffer, "#5q", 3) == 0){
			//Donner la visibilité
			Client *c;
			c = This->clients->getFromFrom(This->clients, from);
			if (c == NULL){
				printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			}
			toSend=Reseau_giveVisibility(This, recvBuffer+3);
			sendto(This->sockEcouteInternalMessages, toSend, strlen(toSend)+1, 0, (struct sockaddr *) &dst, sizeof(dst));
			free(toSend);
		}
		else if (strncmp(recvBuffer, "#5a", 3) == 0){
			//recevoir la visibilité
			Client *c;
			c = This->clients->getFromFrom(This->clients, from);
			if (c == NULL){
				printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			}
			Reseau_recupVisibility(This, recvBuffer+3, c);

			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			if (pthread_mutex_lock(&This->mutexNbrReponseAttendue) < 0){
				perror("pthread_mutex_lock");
				exit(1);
			}
			This->nbrReponseAttendue--;
			if (This->nbrReponseAttendue <= 0){
				pthread_cond_signal(&This->condEverythingRecieved);
			}

			if (pthread_mutex_unlock(&This->mutexNbrReponseAttendue) < 0){
				perror("pthread_mutex_unlock");
				exit(1);
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		}
		else if (strncmp(recvBuffer, "#6#", 3) == 0){
			//recevoir message de défaite
			This->g->victoire=-2;
		}
		else if (strncmp(recvBuffer, "#7q", 3) == 0){
			//recevoir message de défaite
			Client *c;
			c = This->clients->getFromFrom(This->clients, from);
			if (c == NULL){
				printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			}
			printf("Chaine recue : %s", recvBuffer);
			Reseau_recupCoordinatesEnnemy(This, recvBuffer+3, c);
			toSend=Reseau_strAskVisibilitySurrounding(This, c);
			sendto(This->sockEcouteInternalMessages, toSend, strlen(toSend)+1, 0, (struct sockaddr *) &dst, sizeof(dst));
			free(toSend);
		}
		else if (strncmp(recvBuffer, "#7a", 3) == 0){
			//recevoir la visibilité suite à un de nos déplacement
			Client *c;
			c = This->clients->getFromFrom(This->clients, from);
			if (c == NULL){
				printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			}
			Reseau_recupVisibility(This, recvBuffer+3, c);
		}
		else {
			printf("Unwnown message:\n");
			recvBuffer[sizeRecieved]='\0';
			printf("*%s*\n",recvBuffer);
			hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
			printf("...de la part de %s(%d)\n",inet_ntoa(from.sin_addr),htons(from.sin_port));

			dst.sin_addr=from.sin_addr;
			dst.sin_port=from.sin_port;
			dst.sin_family=AF_INET;
		}
	}
	pthread_exit(NULL);
}

void updateMax(Reseau* This, unsigned int oldmax){
	unsigned int i=0;
	unsigned int max=0;
	for(; i < oldmax; ++i){
		if (FD_ISSET(i, &This->untouchableSet))
			max=i;
	}
	This->maxFd=max;
}

void* HandleTcpPlayer(void *arg){
	// Le thread pourra être interrompu à tout moment (notamment dans le recvfrom bloquant)
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	Reseau* This = (Reseau*)arg;
	struct sockaddr_in newClientInfos;
	unsigned int newClientInfosSize = sizeof(struct sockaddr_in);
	int newClientSocket, i, done, sizeRecieved, retSelect;
	char recvBuffer[4096], *toSend;
	for(;;){
		memcpy(&This->degradableSet, &This->untouchableSet, sizeof(fd_set));
		retSelect=select(This->maxFd+1, &This->degradableSet, NULL, NULL, NULL);
		if (retSelect <= 0){
			perror("Select");
			exit(1);
		}
		for(i=0, done = 0 ; i < This->maxFd+1 && done != retSelect; ++i){
			if (FD_ISSET(i, &This->degradableSet)){
				++done;
				if (i == This->sockEcouteTcp){
					if ( (newClientSocket=accept(This->sockEcouteTcp, (struct sockaddr*)&newClientInfos, &newClientInfosSize)) == -1){
						perror("accept");
						exit(1);
					}
					Client* c = New_Client();
					c->socketTCP=newClientSocket;
					unsigned int tailleSockaddrIn=sizeof(struct sockaddr_in);
					if (getpeername(newClientSocket, (struct sockaddr*)&c->from, &tailleSockaddrIn) < 0){
						perror("getpeername");
						c->Free(c);
						continue;
					}
					pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
					if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
						perror("pthread_mutex_lock");
						exit(-10);
					}
					if (This->clients->Push(This->clients, c) == ERROR_MALLOC_ITEM){
						exit(-1);
					}
					if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
						perror("pthread_mutex_unlock");
						exit(-10);
					}
					pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
					FD_SET(newClientSocket, &This->untouchableSet);
					if (newClientSocket > This->maxFd){
						This->maxFd=newClientSocket;
					}
					printf("Nouveau Client ajoutté au jeu, adresse ip: %s portTCP: %d socket no : %d\n", inet_ntoa(c->from.sin_addr),htons(c->from.sin_port), i);
				}
				else if (i == This->selfPipe[0]){
					read(This->selfPipe[0], recvBuffer, sizeof(recvBuffer));
				}
				else {
					if ( (sizeRecieved=read(i, recvBuffer, sizeof(recvBuffer))) <= 0){
						printf("Un client quitte la partie.\n");
						//						pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
						//						if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
						//							perror("pthread_mutex_lock");
						//							exit(-10);
						//						}
						This->clients->remove(This->clients, i);
						//						if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
						//							perror("pthread_mutex_unlock");
						//							exit(-10);
						//						}
						pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
						FD_CLR(i, &This->untouchableSet);
						if (This->maxFd == i){
							updateMax(This, i);
						}
					}
					else {
						unsigned int tailleSockaddrIn=sizeof(struct sockaddr_in);
						//Quel est ton port udpInterne
						if (strncmp(recvBuffer, "#2#", 3) == 0){
							if (getpeername(newClientSocket, (struct sockaddr*)&newClientInfos, &tailleSockaddrIn) < 0){
								perror("getpeername");
								continue;
							}
							Client *c;
							uint16_t val=0;
							sscanf(recvBuffer+3, "%" SCNd16, &val);
							pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
							//							if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
							//								perror("pthread_mutex_lock");
							//								exit(-10);
							//							}
							c = This->clients->getFromFrom(This->clients, newClientInfos);
							if (c == NULL){
								printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
								//								if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
								//									perror("pthread_mutex_unlock");
								//									exit(-10);
								//								}
								//								pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
								continue;
							}
							c->from.sin_port=htons(val);
							sprintf(c->ipPortString, "%s:%d", inet_ntoa(c->from.sin_addr),htons(c->from.sin_port));
							//							if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
							//								perror("pthread_mutex_unlock");
							//								exit(-10);
							//							}
							//							pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
						}
						else if (strncmp(recvBuffer, "#4q", 3) == 0){
							//Donner la propriétée
							Client *c;
							//							pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
							//							if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
							//								perror("pthread_mutex_lock");
							//								exit(-10);
							//							}
							c = This->clients->getFromSockNo(This->clients, i);
							if (c == NULL){
								printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
								//								if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
								//									perror("pthread_mutex_unlock");
								//									exit(-10);
								//								}
								//								pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
								continue;
							}
							toSend=Reseau_giveProperty(This, recvBuffer+3, c);
							write(c->socketTCP, toSend, strlen(toSend));
							//							if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
							//								perror("pthread_mutex_unlock");
							//								exit(-10);
							//							}
							//							pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
							free(toSend);
						}
						else if (strncmp(recvBuffer, "#4a", 3) == 0){
							//mettre a jour la matrice de propriété
							Client *c;
							//							pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
							//							if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
							//								perror("pthread_mutex_lock");
							//								exit(-10);
							//							}
							c = This->clients->getFromSockNo(This->clients, i);
							if (c == NULL){
								printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
								//								if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
								//									perror("pthread_mutex_unlock");
								//									exit(-10);
								//								}
								//								pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
								continue;
							}
							Reseau_recupProperty(This, recvBuffer+3, c);

							pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
							if (pthread_mutex_lock(&This->mutexNbrReponseAttendue) < 0){
								perror("pthread_mutex_lock");
								exit(1);
							}
							This->nbrReponseAttendue--;
							if (This->nbrReponseAttendue <= 0){
								pthread_cond_signal(&This->condEverythingRecieved);
							}

							if (pthread_mutex_unlock(&This->mutexNbrReponseAttendue) < 0){
								perror("pthread_mutex_unlock");
								exit(1);
							}
							pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
						}
						else {
							printf("Should never append\n");
						}
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}

Reseau *New_Reseau(Grille *g)
{
	Reseau* This = malloc(sizeof(Reseau));
	if(!This) return NULL;
	Reseau_Init(This, g);
	This->Free=Reseau_New_Free;
	return This;
}

Reseau Reseau_Create(Grille *g)
{
	Reseau This;
	Reseau_Init(&This, g);
	This.Free=Reseau_Free;
	return This;
}

void Reseau_New_Free(Reseau* This)
{
	This->Clear(This);
	free(This);
}

void Reseau_Free(Reseau* This)
{
	This->Clear(This);
}

void Reseau_Init(Reseau* This, Grille* g){
	This->Clear=Reseau_Clear;
	This->flag=0;
	This->askForProperty=Reseau_askForProperty;
	This->giveProperty=Reseau_giveProperty;
	This->recupProperty=Reseau_recupProperty;

	This->askForVisibility=Reseau_askForVisibility;
	This->giveVisibility=Reseau_giveVisibility;
	This->recupVisibility=Reseau_recupVisibility;

	This->sendWin=Reseau_sendWin;
	This->sendPos=Reseau_sendPos;

	This->clients=New_ListeClient();
	pthread_mutex_init(&This->mutexMatricePropriete, NULL);
	pthread_mutex_init(&This->mutexNbrReponseAttendue, NULL);
	pthread_cond_init(&This->condEverythingRecieved, NULL);
	This->portEcouteInternalMessages=5000;
	This->portEcouteTcp=5100;
	This->portEcouteIncommingClients=5200;

	This->nbrReponseAttendue=0;

	FD_ZERO(&This->degradableSet);
	FD_ZERO(&This->untouchableSet);
	This->g=g;
	This->maxFd = 0;

	if (pipe(This->selfPipe) < 0 ) {
		perror("pipe");
		exit(1);
	}
	FD_SET(This->selfPipe[0], &This->untouchableSet);
	if (This->selfPipe[0] > This->maxFd){
		This->maxFd=This->selfPipe[0];
	}
	creatEcouteInternalMessages(This);
	creatEcouteTcp(This);

	//lancer la demande (Anybody out there ?) avec un timeout de deux secondes
	// --> etre connecté ou non
	This->carteInitialised=False;
	tenterConnection(This);
	if (This->clients->taille != 0){
		askForCarte(This);
		while (This->carteInitialised != True);
	}
	creatIncommingClients(This);
	printf("Fin init\n");
}

void Reseau_Clear(Reseau *This){
	pthread_cancel(This->th_ThreadIncommingPlayer);
	if (pthread_join(This->th_ThreadIncommingPlayer, NULL)) {
		perror("pthread_join");
		return;
	}
	pthread_cancel(This->th_ThreadTcp);
	if (pthread_join(This->th_ThreadTcp, NULL)) {
		perror("pthread_join");
		return;
	}
	pthread_cancel(This->th_ThreadInternalMessages);
	if (pthread_join(This->th_ThreadInternalMessages, NULL)) {
		perror("pthread_join");
		return;
	}

	close(This->selfPipe[0]);
	close(This->selfPipe[1]);
	pthread_mutex_destroy(&This->mutexMatricePropriete);
	pthread_mutex_destroy(&This->mutexNbrReponseAttendue);
	pthread_cond_destroy(&This->condEverythingRecieved);
	close(This->sockEcouteIncommingClients);
	close(This->sockEcouteInternalMessages);
	close(This->sockEcouteTcp);
	This->clients->Free(This->clients, 1);
}

int creatIncommingClients(Reseau *This) {
	struct sockaddr_in paramSocket;
	//    int on=1;
	if ( (This->sockEcouteIncommingClients = socket(AF_INET, SOCK_DGRAM, 0) ) <0){
		perror("Creation sockEcouteIncommingClients");
		exit(1);
	}

	//    if (setsockopt(This->sockEcouteIncommingClients, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
	//        perror("setsockopt");
	//        exit(1);
	//    }
	paramSocket.sin_family = AF_INET;
	paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
	paramSocket.sin_port=htons(This->portEcouteIncommingClients);

	if (bind (This->sockEcouteIncommingClients, (const struct sockaddr *)&paramSocket, sizeof(paramSocket)) < 0){
		printf("Port d'écoute Incoming Client %d déjà en cours d'utilisation -->inc\n", This->portEcouteIncommingClients);
		exit(1);
	}

	if (pthread_create(&This->th_ThreadIncommingPlayer, NULL, HandleIncommingPlayer, This)) {
		perror("pthread_create");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int creatEcouteInternalMessages(Reseau *This){
	struct sockaddr_in paramSocket;
	int i;

	if ( (This->sockEcouteInternalMessages= socket(AF_INET, SOCK_DGRAM, 0) ) <0){
		perror("Creation sockEcouteInternalClients");
		exit(1);
	}
	//    if (setsockopt(This->sockEcouteInternalMessages, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
	//        perror("setsockopt");
	//        exit(1);
	//    }
	for (i=0; i< 10; ++i){
		paramSocket.sin_family = AF_INET;
		paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
		paramSocket.sin_port=htons(This->portEcouteInternalMessages);

		if (bind (This->sockEcouteInternalMessages, (const struct sockaddr *)&paramSocket, sizeof(paramSocket)) < 0){
			printf("Port d'écoute Internal Message %d déjà en cours d'utilisation -->inc\n", This->portEcouteInternalMessages);
			++This->portEcouteInternalMessages;
			continue;
		}
		break;
	}

	This->portEcouteTcp=This->portEcouteInternalMessages+100;
	This->portEcouteIncommingClients=This->portEcouteInternalMessages+200;
	printf("InternalUDP: %d \t TCP: %d \t ExternalUDP: %d\n", This->portEcouteInternalMessages, This->portEcouteTcp, This->portEcouteIncommingClients);

	if (pthread_create(&This->th_ThreadInternalMessages, NULL, HandleInternalMessage, This)) {
		perror("pthread_create");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int creatEcouteTcp(Reseau *This){
	struct sockaddr_in paramSocket;
	//    int on=1;
	if ( (This->sockEcouteTcp = socket(AF_INET, SOCK_STREAM, 0) ) <0){
		perror("Creation sockEcouteTCP");
		exit(1);
	}

	//    if (setsockopt(This->sockEcouteTcp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
	//        perror("setsockopt");
	//        exit(1);
	//    }

	paramSocket.sin_family = AF_INET;
	paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
	paramSocket.sin_port=htons(This->portEcouteTcp);

	if (bind (This->sockEcouteTcp, (const struct sockaddr *)&paramSocket, sizeof(paramSocket))<0){
		printf("Port d'écoute TCP %d déjà en cours d'utilisation -->inc\n", This->portEcouteTcp);
		exit(-1);
	}

	if (listen(This->sockEcouteTcp, 10) < 0){
		perror("Listen sockEcouteTCP");
		exit(-1);
	}

	FD_SET(This->sockEcouteTcp, &This->untouchableSet);
	if (This->sockEcouteTcp > This->maxFd){
		This->maxFd=This->sockEcouteTcp;
	}

	if (pthread_create(&This->th_ThreadTcp, NULL, HandleTcpPlayer, This)) {
		perror("pthread_create");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void tenterConnection(Reseau *This){
	struct sockaddr_in  dst, from;
	struct hostent *hp;
	struct ifconf ifc;
	struct ifreq *ifr;
	int socketBroadcast, n, sizeRecieved, on;
	char recvBuffer[4096], liste[1024], toSend[20];
	unsigned int fromlen;

	// Création de socket UDP
	if ( (socketBroadcast = socket(AF_INET, SOCK_DGRAM, 0) ) <0){
		perror("socket de demande");
		exit(-1);
	}

	// Passage de la socket en mode broadcast
	on = 1;	// on == 1 pour activer
	if(setsockopt(socketBroadcast, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on) )<0) {
		perror("socket opt");
		exit(-1);
	}

	/* Trouve son adresse de broadcast */
	ifc.ifc_len = sizeof (liste);
	ifc.ifc_buf = liste;
	if (ioctl(socketBroadcast, SIOCGIFCONF, (char *) &ifc) <0){
		perror("ioctl IFCONF");
		exit(1);
	}

	n=ifc.ifc_len/sizeof(struct ifreq);
	ifr = ifc.ifc_req;
	for (; --n>= 0; ifr++){
		if (ioctl(socketBroadcast, SIOCGIFFLAGS, (char *) ifr)<0){
			perror("ioctl IFFLAG");
			exit(1);
		}
		// Si l'interface est up et que c'est pas loopback et pas PointToPoint et broadcast
		if (	((ifr->ifr_flags & IFF_UP)== 0) ||
				(ifr->ifr_flags & IFF_LOOPBACK) ||
				(ifr->ifr_flags &  IFF_POINTOPOINT)||
				((ifr->ifr_flags & IFF_BROADCAST)==0) )
		{
			continue;
		}

		/*******************************************************/

		/* Fixe adresse de broadcast sur la socket */
		if (ioctl(socketBroadcast, SIOCGIFBRDADDR, (char *) ifr)){
			perror("ioctl BROADADDR");
			exit(1);
		}
		bcopy(&(ifr->ifr_broadaddr), &dst, sizeof(ifr->ifr_broadaddr));


		/* Envoie à tout le monde d'une demande de connection */
		dst.sin_port=htons(5200);
		dst.sin_family=AF_INET;
		sendto(socketBroadcast, "#1q", 3, 0, (struct sockaddr *) &dst, sizeof(dst));
	}

	/* Setting du timeout de la socket pour les receptions*/
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(socketBroadcast, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
		exit(-1);
	}

	fromlen=sizeof(from);
	while ((sizeRecieved=recvfrom(socketBroadcast, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr *)&from, &fromlen)) >= 0 || (sizeRecieved < 0 && errno == EINTR)){
		if (errno == EINTR){
			continue;
		}
		/* Connection TCP à chaque client */
		printf("Nouveau serveur à qui se connecter\n");
		Client *c = New_Client();
		recvBuffer[sizeRecieved]='\0';
		int porttcp, portudp;
		if (strncmp(recvBuffer, "#1a", 3) != 0) return;
		sscanf(recvBuffer+3, "%d%d", &porttcp, &portudp);
		hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
		hp=hp;
		printf("Recu:\tcode:#1a\tportTCP:%d\tportUDP:%d\t from %s(%d)\n", porttcp, portudp ,inet_ntoa(from.sin_addr),htons(from.sin_port));

		c->socketTCP = socket(AF_INET, SOCK_STREAM, 0);
		c->from.sin_addr=from.sin_addr;
		c->from.sin_port=htons(portudp);

		struct sockaddr_in destTCP;
		destTCP.sin_family=AF_INET;
		destTCP.sin_port=htons(porttcp);
		destTCP.sin_addr=from.sin_addr;

		if (connect(c->socketTCP, (struct sockaddr*)&destTCP, sizeof(struct sockaddr_in)) < 0){
			perror("Connect");
			continue;
		}
		sprintf(toSend, "#2#%d\n", This->portEcouteInternalMessages);
		if (write(c->socketTCP, toSend, strlen(toSend)) <= 0){
			perror("PERROR Write");
		}

		This->clients->Push(This->clients, c);
		FD_SET(c->socketTCP, &This->untouchableSet);
		if (c->socketTCP > This->maxFd){
			This->maxFd=c->socketTCP;
		}
		write(This->selfPipe[1], "n", 1);
		/* Traitement de la donnée sensible :
		 * pthread_mutex_lock(pthread_mutex_t *mut);
		 */

		//        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		//		if (pthread_mutex_lock(&This->mutexMatricePropriete) < 0){
		//			perror("pthread_mutex_lock");
		//			exit(1);
		//		}
		/* Fin du traitement de la donnée
		 * pthread_mutex_unlock(pthread_mutex_t *mut);
		 */
		//		if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		//			perror("pthread_mutex_unlock");
		//			exit(1);
		//		}
		//        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	}
	if (errno == EAGAIN){
		/* Timeout : Soit on s'est connecté à tous les clients, soit on est seul */
		return;
	} else {
		perror("recvfrom");
		exit(1);
	}
}

void askForCarte(Reseau *This){
	int i;
	Client *c;
	//	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	//	if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
	//		perror("pthread_mutex_unlock");
	//		exit(-10);
	//	}
	for (i=0; i<This->clients->taille; ++i){
		c = This->clients->getNieme(This->clients, i);
		if (c == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			continue;
		}
		printf("Demande de partie de carte envoyée à : %s:%d\n", inet_ntoa(c->from.sin_addr), htons(c->from.sin_port));
		if (sendto(This->sockEcouteInternalMessages, "#3q", 3, 0, (struct sockaddr*)&c->from, sizeof(c->from)) == -1){
			perror("Send to __LINE__");
		}
	}
	//	if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
	//		perror("pthread_mutex_unlock");
	//		exit(-10);
	//	}
	//	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

void Reseau_askForProperty(Reseau *This, ListeCase* lcas){
	int i, j, offset;
	Case *cas;
	Client *cli;
	ListeClient* lcli = New_ListeClient();
	char *propDemandeSur;
	//	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	//	if (pthread_mutex_lock(&This->clients->mutexListeClient) < 0 ){
	//		perror("pthread_mutex_lock");
	//		exit(-10);
	//	}
	for (i=0; i<lcas->taille; ++i){
		cas = lcas->getNieme(lcas, i);
		if (cas == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			continue;
		}
		if (cas->proprietaire == NULL)
			continue;
		if ( (cli = lcli->getFromFrom(lcli, cas->proprietaire->from)) == NULL){
			lcli->Push(lcli, cas->proprietaire);
			cli=cas->proprietaire;
		}
		cli->casesTo->Push(cli->casesTo, cas);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexNbrReponseAttendue)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	This->nbrReponseAttendue=lcli->taille;
	if (This->nbrReponseAttendue <= 0){
		pthread_cond_signal(&This->condEverythingRecieved);
	}

	if (pthread_mutex_unlock(&This->mutexNbrReponseAttendue) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	for(i=0 ; i< lcli->taille; ++i){
		cli=lcli->getNieme(lcli, i);
		if (cli == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
		}
		// (5+1) = 1 int + 1\n * 2 car deux coordonnées
		propDemandeSur=malloc(((5+1)*2*(cli->casesTo->taille*2+1)+3+1)*sizeof(char));
		offset = sprintf(propDemandeSur, "#4q%d\n", cli->casesTo->taille);

		for(j=0; j<cli->casesTo->taille; ++j){
			cas=cli->casesTo->getNieme(cli->casesTo, j);
			offset += sprintf(propDemandeSur+offset, "%d\n%d\n", cas->posX, cas->posY);
		}
		offset += sprintf(propDemandeSur+offset, "#");

		//printf("Demande de propriété de case envoyée à : %s:%d\n", inet_ntoa(cas->proprietaire->from.sin_addr), htons(cas->proprietaire->from.sin_port));
		if (write(cli->socketTCP, propDemandeSur, strlen(propDemandeSur)) == -1){
			perror("Send to __LINE__");
		}
		free(propDemandeSur);
		cli->casesTo->Vider(cli->casesTo);
	}
	//	if (pthread_mutex_unlock(&This->clients->mutexListeClient) < 0 ){
	//		perror("pthread_mutex_unlock");
	//		exit(-10);
	//	}
	//	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	lcli->Free(lcli, 0);
}

char* Reseau_giveProperty(Reseau *This, char* str, Client *cli){
	uint16_t xCase, yCase, nbrCase, i, offset;
	ListeCase *lca = New_ListeCase();
	Case* ca;
	char* SerializedThis;
	char **leschaines;
	// Stream via pipe

	int fdRW[2];
	if (pipe(fdRW) < 0){
		perror("Pipe");
		exit(-1);
	}
	FILE* readStream=fdopen(fdRW[0], "r");
	write(fdRW[1], str, strlen(str));
	if (readStream == NULL){
		perror("fopen");
		exit(-1);
	}
	//Le lock - unlock est hors du for pour accélérer le temps de réponse à une demande de propriété (pour ne pas être ejecté)
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	fscanf(readStream, "%" SCNd16, &nbrCase);
	//printf("Demande de propriété recue %d cases demandées\n", nbrCase);
	for (i=0; i<nbrCase; ++i){
		fscanf(readStream, "%" SCNd16, &xCase);
		fscanf(readStream, "%" SCNd16, &yCase);
		//printf("Puis-je donner la case [%d][%d] : ", xCase, yCase);

		if (This->g->tab[xCase][yCase].proprietaire == NULL && This->g->tab[xCase][yCase].liste->HasAPecheur(This->g->tab[xCase][yCase].liste) == False){
			ca = &This->g->tab[xCase][yCase];
			--This->g->NbrCasesToMe;
			ca->proprietaire=cli;
			lca->Push(lca, &This->g->tab[xCase][yCase]);
			//printf("OUI\n");
		}
	}
	leschaines=malloc(lca->taille*sizeof(char*));
	if (!leschaines) return NULL;
	unsigned int nbrCaract=0;
	for(i=0;i<lca->taille;++i){
		ca=lca->getNieme(lca, i);
		leschaines[i]=ca->serialize(ca);
		nbrCaract+=strlen(leschaines[i]);
	}
	if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	SerializedThis=malloc((nbrCaract+(5+1)+4+1)*sizeof(char));
	offset = sprintf(SerializedThis, "#4a%d\n", lca->taille);

	for(i=0; i<lca->taille; ++i){
		if (leschaines[i] == NULL)
			continue;
		offset += sprintf(SerializedThis+offset, "%s", leschaines[i]);
		free(leschaines[i]);
	}
	offset += sprintf(SerializedThis+offset, "#");
	lca->Free(lca);
	free(leschaines);
	fclose(readStream);
	close(fdRW[1]);
	//printf("Je vais renvoyer : %s\n", SerializedThis);
	return SerializedThis;
}

Bool Reseau_recupProperty(Reseau* This, char* str, Client* cli){
	uint16_t xCase, yCase, nbrCase, nbrElem, i, j;
	uint16_t type, dernierRepas, sasiete, derniereReproduction;
	uint16_t sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY;
	int fdRW[2];
	if (pipe(fdRW) < 0){
		perror("Pipe");
		return False;
	}
	FILE* readStream=fdopen(fdRW[0], "r");
	write(fdRW[1], str, strlen(str));
	if (readStream == NULL){
		perror("fopen");
		exit(-1);
	}


	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	fscanf(readStream, "%" SCNd16, &nbrCase);
	for (i=0; i<nbrCase; ++i){
		fscanf(readStream, "%" SCNd16, &xCase);
		fscanf(readStream, "%" SCNd16, &yCase);
		fscanf(readStream, "%" SCNd16, &nbrElem);

		if (This->g->tab[xCase][yCase].proprietaire == cli){
			This->g->tab[xCase][yCase].proprietaire = NULL;
			++This->g->NbrCasesToMe;
			if (This->flag==1){
				//				This->g->tab[xCase][yCase].isLocked=True;
			}
			This->g->tab[xCase][yCase].liste->Clear(This->g->tab[xCase][yCase].liste);

			for (j=0; j<nbrElem ; ++j){
				//Scan d'un élément
				fscanf(readStream, "%" SCNd16, &type);
				if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
					fscanf(readStream, "%" SCNd16, &dernierRepas);
					fscanf(readStream, "%" SCNd16, &sasiete);
					fscanf(readStream, "%" SCNd16, &derniereReproduction);
					//printf("%d\t%d\t%d\t%d\n", type, dernierRepas, sasiete, derniereReproduction);
					ElementAnimal *ea = New_ElementAnimal(&This->g->tab[xCase][yCase], type);
					ea->SetDernierRepas(ea, dernierRepas);
					ea->SetSasiete(ea, sasiete);
					ea->SetDerniereReproduction(ea, derniereReproduction);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)ea);
				}
				else if (type == TERRE){
					//printf("%d\n", type);
					ElementTerre *t = New_ElementTerre(&This->g->tab[xCase][yCase]);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)t);
				}
				else if (type == PONT){
					//printf("%d\n", type);
					ElementPont*t = New_ElementPont(&This->g->tab[xCase][yCase]);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)t);
				}
				else if (type == PECHEUR){
					printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
					fscanf(readStream, "%" SCNd16, &sac);
					fscanf(readStream, "%" SCNd16, &longueurCanne);
					fscanf(readStream, "%" SCNd16, &tailleFilet);
					fscanf(readStream, "%" SCNd16, &distanceDeplacement);
					fscanf(readStream, "%" SCNd16, &PositionInitialeX);
					fscanf(readStream, "%" SCNd16, &PositionInitialeY);

					//printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", type, sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY);
					ElementPecheur *p = New_ElementPecheur(&This->g->tab[xCase][yCase]);
					p->SetSac(p, sac);
					p->SetLongueurCanne(p, longueurCanne);
					p->SetTailleFilet(p, tailleFilet);
					p->SetDistanceDeplacement(p, distanceDeplacement);
					p->SetPositionInitialeX(p, PositionInitialeX);
					p->SetPositionInitialeY(p, PositionInitialeY);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)p);
				}
			}
		}
		else {
			for (j=0; j<nbrElem ; ++j){
				fscanf(readStream, "%" SCNd16, &type);
				if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
					fscanf(readStream, "%" SCNd16, &dernierRepas);
					fscanf(readStream, "%" SCNd16, &sasiete);
					fscanf(readStream, "%" SCNd16, &derniereReproduction);
				}
				else if (type == PECHEUR){
					fscanf(readStream, "%" SCNd16, &sac);
					fscanf(readStream, "%" SCNd16, &longueurCanne);
					fscanf(readStream, "%" SCNd16, &tailleFilet);
					fscanf(readStream, "%" SCNd16, &distanceDeplacement);
					fscanf(readStream, "%" SCNd16, &PositionInitialeX);
					fscanf(readStream, "%" SCNd16, &PositionInitialeY);
				}
			}
		}

	}
	if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	fclose(readStream);
	close(fdRW[1]);
	return True;
}

void Reseau_askForVisibility(Reseau *This, ListeCase* lcas){
	int i, j, offset;
	Case *cas;
	Client *cli;
	ListeClient* lcli = New_ListeClient();
	char *propDemandeSur;
	for (i=0; i<lcas->taille; ++i){
		cas = lcas->getNieme(lcas, i);
		if (cas == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
		}
		if (cas->proprietaire == NULL)
			continue;
		if ( (cli = lcli->getFromFrom(lcli, cas->proprietaire->from)) == NULL){
			lcli->Push(lcli, cas->proprietaire);
			cli=cas->proprietaire;
		}
		if (cli->casesTo == NULL){
			cli->casesTo = New_ListeCase();
			if (cli->casesTo == NULL){
				return;
			}
		}
		cli->casesTo->Push(cli->casesTo, cas);
	}

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	This->nbrReponseAttendue=lcli->taille;
	if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	for(i=0 ; i< lcli->taille; ++i){
		cli=lcli->getNieme(lcli, i);
		if (cli == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
		}
		// (5+1) = 1 int + 1\n * 2 car deux coordonnées
		propDemandeSur=malloc(((5+1)*2*(cli->casesTo->taille*2+1)+3+1)*sizeof(char));
		offset = sprintf(propDemandeSur, "#5q%d\n", cli->casesTo->taille);

		for(j=0; j<cli->casesTo->taille; ++j){
			cas=cli->casesTo->getNieme(cli->casesTo, j);
			offset += sprintf(propDemandeSur+offset, "%d\n%d\n", cas->posX, cas->posY);
		}
		offset += sprintf(propDemandeSur+offset, "#");

		printf("Demande de visibilité de case envoyée à : %s:%d\n", inet_ntoa(cas->proprietaire->from.sin_addr), htons(cas->proprietaire->from.sin_port));
		if (sendto(This->sockEcouteInternalMessages, propDemandeSur, strlen(propDemandeSur), 0, (struct sockaddr*)&cas->proprietaire->from, sizeof(cas->proprietaire->from)) == -1){
			perror("Send to __LINE__");
		}
		free(propDemandeSur);
		cli->casesTo->Vider(cli->casesTo);
	}
	lcli->Free(lcli, 0);
}

char* Reseau_strAskVisibilitySurrounding(Reseau *This, Client* cli){
	Case ***visibilite;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	visibilite=This->g->getMatriceVoisins(This->g, cli->tabPosPecheurs[cli->noPecheurLastMove][indexPosX], cli->tabPosPecheurs[cli->noPecheurLastMove][indexPosY], 2);
	int i, j, offset;
	Case *cas;
	ListeCase *lcas=New_ListeCase();
	for(i=0; i<5; ++i){
		for(j=0; j<5; ++j){
			if (visibilite[i][j] != NULL && visibilite[i][j]->proprietaire == NULL)
				lcas->Push(lcas, visibilite[i][j]);
		}
	}
	char *SerializedThis;
	char **leschaines;
	leschaines=malloc(lcas->taille*sizeof(char*));
	if (!leschaines) return NULL;
	unsigned int nbrCaract=0;


	for(i=0;i<lcas->taille;++i){
		cas=lcas->getNieme(lcas, i);
		leschaines[i]=cas->serialize(cas);
		nbrCaract+=strlen(leschaines[i]);
	}

	if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	SerializedThis=malloc((nbrCaract+(5+1)+4+1)*sizeof(char));
	offset = sprintf(SerializedThis, "#7a%d\n", lcas->taille);

	for(i=0; i<lcas->taille; ++i){
		if (leschaines[i] == NULL)
			continue;
		offset += sprintf(SerializedThis+offset, "%s", leschaines[i]);
		free(leschaines[i]);
	}
	offset += sprintf(SerializedThis+offset, "#");
	lcas->Free(lcas);
	for (i=0; i<5;++i){
		if (visibilite[i] != NULL)
			free(visibilite[i]);
	}
	free(visibilite);
	free(leschaines);
	return SerializedThis;
}

char* Reseau_giveVisibility(Reseau *This, char* str){
	uint16_t xCase, yCase, nbrCase, i, offset;
	ListeCase *lca = New_ListeCase();
	Case* ca;
	char* SerializedThis;
	char **leschaines;
	// Stream via pipe

	int fdRW[2];
	if (pipe(fdRW) < 0){
		perror("Pipe");
		exit(-1);
	}
	FILE* readStream=fdopen(fdRW[0], "r");
	write(fdRW[1], str, strlen(str));
	if (readStream == NULL){
		perror("fopen");
		exit(-1);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	fscanf(readStream, "%" SCNd16, &nbrCase);
	for (i=0; i<nbrCase; ++i){
		fscanf(readStream, "%" SCNd16, &xCase);
		fscanf(readStream, "%" SCNd16, &yCase);

		if (This->g->tab[xCase][yCase].proprietaire == NULL){
			lca->Push(lca, &This->g->tab[xCase][yCase]);
		}
	}
	leschaines=malloc(lca->taille*sizeof(char*));
	if (!leschaines) return NULL;
	unsigned int nbrCaract=0;


	for(i=0;i<lca->taille;++i){
		ca=lca->getNieme(lca, i);
		leschaines[i]=ca->serialize(ca);
		nbrCaract+=strlen(leschaines[i]);
	}
	if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	SerializedThis=malloc((nbrCaract+(5+1)+4+1)*sizeof(char));
	offset = sprintf(SerializedThis, "#5a%d\n", lca->taille);
	for(i=0; i<lca->taille; ++i){
		if (leschaines[i] == NULL)
			continue;
		offset += sprintf(SerializedThis+offset, "%s", leschaines[i]);
		free(leschaines[i]);
	}
	offset += sprintf(SerializedThis+offset, "#");
	lca->Free(lca);
	free(leschaines);
	fclose(readStream);
	close(fdRW[1]);
	return SerializedThis;
}

Bool Reseau_recupVisibility(Reseau* This, char* str, Client* cli){
	uint16_t xCase, yCase, nbrCase, nbrElem, i, j;
	uint16_t type, dernierRepas, sasiete, derniereReproduction;
	uint16_t sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY;
	int fdRW[2];
	if (pipe(fdRW) < 0){
		perror("Pipe");
		return False;
	}
	FILE* readStream=fdopen(fdRW[0], "r");
	write(fdRW[1], str, strlen(str));
	if (readStream == NULL){
		perror("fopen");
		exit(-1);
	}

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if (pthread_mutex_lock(&This->mutexMatricePropriete)){
		perror("pthread_mutex_lock");
		exit(-10);
	}
	fscanf(readStream, "%" SCNd16, &nbrCase);
	for (i=0; i<nbrCase; ++i){
		fscanf(readStream, "%" SCNd16, &xCase);
		fscanf(readStream, "%" SCNd16, &yCase);
		fscanf(readStream, "%" SCNd16, &nbrElem);

		if (This->g->tab[xCase][yCase].proprietaire == cli){
			This->g->tab[xCase][yCase].liste->Clear(This->g->tab[xCase][yCase].liste);

			for (j=0; j<nbrElem ; ++j){
				//Scan d'un élément
				fscanf(readStream, "%" SCNd16, &type);
				if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
					fscanf(readStream, "%" SCNd16, &dernierRepas);
					fscanf(readStream, "%" SCNd16, &sasiete);
					fscanf(readStream, "%" SCNd16, &derniereReproduction);
					//printf("%d\t%d\t%d\t%d\n", type, dernierRepas, sasiete, derniereReproduction);
					ElementAnimal *ea = New_ElementAnimal(&This->g->tab[xCase][yCase], type);
					ea->SetDernierRepas(ea, dernierRepas);
					ea->SetSasiete(ea, sasiete);
					ea->SetDerniereReproduction(ea, derniereReproduction);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)ea);
				}
				else if (type == TERRE){
					//printf("%d\n", type);
					ElementTerre *t = New_ElementTerre(&This->g->tab[xCase][yCase]);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)t);
				}
				else if (type == PONT){
					//printf("%d\n", type);
					ElementPont*t = New_ElementPont(&This->g->tab[xCase][yCase]);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)t);
				}
				else if (type == PECHEUR){
					fscanf(readStream, "%" SCNd16, &sac);
					fscanf(readStream, "%" SCNd16, &longueurCanne);
					fscanf(readStream, "%" SCNd16, &tailleFilet);
					fscanf(readStream, "%" SCNd16, &distanceDeplacement);
					fscanf(readStream, "%" SCNd16, &PositionInitialeX);
					fscanf(readStream, "%" SCNd16, &PositionInitialeY);

					//printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", type, sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY);
					ElementPecheur *p = New_ElementPecheur(&This->g->tab[xCase][yCase]);
					p->SetSac(p, sac);
					p->SetLongueurCanne(p, longueurCanne);
					p->SetTailleFilet(p, tailleFilet);
					p->SetDistanceDeplacement(p, distanceDeplacement);
					p->SetPositionInitialeX(p, PositionInitialeX);
					p->SetPositionInitialeY(p, PositionInitialeY);
					This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)p);
				}
			}
		}
		else {
			for (j=0; j<nbrElem ; ++j){
				//Scan d'un élément
				fscanf(readStream, "%" SCNd16, &type);
				if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
					fscanf(readStream, "%" SCNd16, &dernierRepas);
					fscanf(readStream, "%" SCNd16, &sasiete);
					fscanf(readStream, "%" SCNd16, &derniereReproduction);
				}
				else if (type == PECHEUR){
					fscanf(readStream, "%" SCNd16, &sac);
					fscanf(readStream, "%" SCNd16, &longueurCanne);
					fscanf(readStream, "%" SCNd16, &tailleFilet);
					fscanf(readStream, "%" SCNd16, &distanceDeplacement);
					fscanf(readStream, "%" SCNd16, &PositionInitialeX);
					fscanf(readStream, "%" SCNd16, &PositionInitialeY);
				}
			}
		}
	}
	if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
		perror("pthread_mutex_unlock");
		exit(-10);
	}
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	fclose(readStream);
	close(fdRW[1]);
	return True;
}

Bool Reseau_recupCoordinatesEnnemy(Reseau* This, char* str, Client* cli){

	int fdRW[2], i;
	if (pipe(fdRW) < 0){
		perror("Pipe");
		return False;
	}
	FILE* readStream=fdopen(fdRW[0], "r");
	write(fdRW[1], str, strlen(str));
	if (readStream == NULL){
		perror("fopen");
		exit(-1);
	}

	int16_t posX, posY, nbPecheur, lastPecheur;
	fscanf(readStream, "%" SCNd16, &nbPecheur);
	cli->nbrPecheurs=nbPecheur;
	fscanf(readStream, "%" SCNd16, &lastPecheur);
	cli->noPecheurLastMove=lastPecheur;

	for(i=0;i<nbPecheur;++i){
		printf("Passage dans la boucle numero %d\n", i);
		fscanf(readStream, "%" SCNd16, &posX);
		fscanf(readStream, "%" SCNd16, &posY);
		if (posX >= 0 && posX < This->g->Taille)
			cli->tabPosPecheurs[i][indexPosX]=posX;
		if (posY >= 0 && posY < This->g->Taille)
			cli->tabPosPecheurs[i][indexPosY]=posY;
	}
	fclose(readStream);
	close(fdRW[1]);
	return True;
}

void unSerialize(Reseau* This, char* str, Client *cli){
	if (cli == NULL){
		printf("unSerialize recieved NULL");
		return;
	}
	uint16_t xCase, yCase, nbrElem, nbrCase, i, k;
	uint16_t type, dernierRepas, sasiete, derniereReproduction;
	uint16_t sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY;
	//Strem via pipe
	int fdRW[2];
	if (pipe(fdRW) < 0){
		perror("Pipe");
	}
	FILE* readStream=fdopen(fdRW[0], "r");
	write(fdRW[1], str, strlen(str));
	if (readStream == NULL){
		perror("fopen");
		exit(-1);
	}
	fscanf(readStream, "%" SCNd16, &nbrCase);
	//printf("Nombre de cases : %d\n", nbrCase);
	for (k=0; k<nbrCase; ++k){
		fscanf(readStream, "%" SCNd16, &xCase);
		fscanf(readStream, "%" SCNd16, &yCase);
		fscanf(readStream, "%" SCNd16, &nbrElem);

		This->g->tab[xCase][yCase].liste->Clear(This->g->tab[xCase][yCase].liste);
		This->g->tab[xCase][yCase].proprietaire=cli;

		//printf("Case[%d][%d] : ", xCase, yCase);
		//if (nbrElem == 0){
		//	printf("\n");
		//}
		for (i=0; i<nbrElem ; ++i){
			//Scan d'un élément
			fscanf(readStream, "%" SCNd16, &type);
			if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
				fscanf(readStream, "%" SCNd16, &dernierRepas);
				fscanf(readStream, "%" SCNd16, &sasiete);
				fscanf(readStream, "%" SCNd16, &derniereReproduction);
				//printf("%d\t%d\t%d\t%d\n", type, dernierRepas, sasiete, derniereReproduction);
				ElementAnimal *ea = New_ElementAnimal(&This->g->tab[xCase][yCase], type);
				ea->SetDernierRepas(ea, dernierRepas);
				ea->SetSasiete(ea, sasiete);
				ea->SetDerniereReproduction(ea, derniereReproduction);
				This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)ea);
			}
			else if (type == TERRE){
				//printf("%d\n", type);
				ElementTerre *t = New_ElementTerre(&This->g->tab[xCase][yCase]);
				This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)t);
			}
			else if (type == PONT){
				//printf("%d\n", type);
				ElementPont*t = New_ElementPont(&This->g->tab[xCase][yCase]);
				This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)t);
			}
			else if (type == PECHEUR){
				fscanf(readStream, "%" SCNd16, &sac);
				fscanf(readStream, "%" SCNd16, &longueurCanne);
				fscanf(readStream, "%" SCNd16, &tailleFilet);
				fscanf(readStream, "%" SCNd16, &distanceDeplacement);
				fscanf(readStream, "%" SCNd16, &PositionInitialeX);
				fscanf(readStream, "%" SCNd16, &PositionInitialeY);

				//printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", type, sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY);
				ElementPecheur *p = New_ElementPecheur(&This->g->tab[xCase][yCase]);
				p->SetSac(p, sac);
				p->SetLongueurCanne(p, longueurCanne);
				p->SetTailleFilet(p, tailleFilet);
				p->SetDistanceDeplacement(p, distanceDeplacement);
				p->SetPositionInitialeX(p, PositionInitialeX);
				p->SetPositionInitialeY(p, PositionInitialeY);
				This->g->tab[xCase][yCase].liste->Push(This->g->tab[xCase][yCase].liste, (Element*)p);
			}
		}
	}
	fclose(readStream);
	close(fdRW[1]);
}



void Reseau_sendWin(Reseau* This)
{
	int i;
	Client *cli;
	for(i=0 ; i< This->clients->taille; ++i){
		cli=This->clients->getNieme(This->clients, i);
		if (cli == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			continue;
		}
		// (5+1) = 1 int + 1\n * 2 car deux coordonnées


		if (sendto(This->sockEcouteInternalMessages, "#6#\n", 4, 0, (struct sockaddr*)&cli->from, sizeof(cli->from)) == -1){
			perror("Send to __LINE__");
		}
	}
}

void Reseau_sendPos(Reseau* This, ElementPecheur* p)
{
	int i, offset;
	int16_t noPecheur;
	Client *cli;
	char *toSend;

	for(i=0; i<This->g->nbPecheur; ++i){
		if (This->g->tabPecheur[i] == p){
			noPecheur=i;
			break;
		}
	}
	toSend=malloc(((5+1)*2*This->g->nbPecheur+(5+1)*2+4+10)*sizeof(char));
	offset=sprintf(toSend, "#7q%d\n%d\n", This->g->nbPecheur, noPecheur);
	for (i=0; i<This->g->nbPecheur; ++i){
		offset+=sprintf(toSend+offset, "%d\n%d\n", This->g->tabPecheur[i]->caseParent->posX, This->g->tabPecheur[i]->caseParent->posY);
	}
	for(i=0 ; i< This->clients->taille; ++i){
		cli=This->clients->getNieme(This->clients, i);
		if (cli == NULL){
			printf("Call the admin NOOOOOOOOOW %s:%d\n", __FUNCTION__, __LINE__);
			continue;
		}
		if (sendto(This->sockEcouteInternalMessages, toSend, strlen(toSend)+1, 0, (struct sockaddr*)&cli->from, sizeof(cli->from)) == -1){
			perror("Send to __LINE__");
		}
	}
	free(toSend);
}
