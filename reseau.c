#include "reseau.h"
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

void* HandleIncommingPlayer(void *arg){
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	Reseau* This = (Reseau*)arg;
	// Le thread pourra être interrompu à tout moment (notamment dans le recvfrom bloquant)
	struct sockaddr_in from;
	struct sockaddr_in dst;
	struct hostent *hp;

	int sizeRecieved;
	char recvBuffer[10], sendbuffer[10];
	unsigned int fromlen=sizeof(from);

	for(;;){
		sizeRecieved=recvfrom(This->sockEcouteIncommingClients, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&from, &fromlen);
		if (sizeRecieved <= 0){
			perror("recvfrom");
			exit(1);
		}
		else if (sizeRecieved < 3){
			printf("Continue car ordre trop court\n");
			continue;
		}
		if (strncmp(recvBuffer, "#1#", 3) == 0){
			printf("Demande de connection recue");
			hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
			printf(" de la part de %s:%d\n",inet_ntoa(from.sin_addr),htons(from.sin_port));


			sprintf(sendbuffer, "1\n%d\n%d\n", This->portEcouteTcp, This->portEcouteInternalMessages);

			dst.sin_addr=from.sin_addr;
			dst.sin_port=from.sin_port;
			dst.sin_family=AF_INET;

			sendto(This->sockEcouteIncommingClients, sendbuffer, strlen(sendbuffer)+1, 0, (struct sockaddr *) &dst, sizeof(dst));
		}
		else {
			printf("Message recu inconnu\n");

			recvBuffer[sizeRecieved]='\0';
			printf("%s\n",recvBuffer);
			hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
			printf("...de la part de %s(%d)\n",inet_ntoa(from.sin_addr),htons(from.sin_port));

			dst.sin_addr=from.sin_addr;
			dst.sin_port=from.sin_port;
			dst.sin_family=AF_INET;

			sendto(This->sockEcouteIncommingClients, "#1#", 7, 0, (struct sockaddr *) &dst, sizeof(dst));
		}
	}
	//Anti warning trick
	hp=hp;
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
	int newClientSocket, i, done, retread, retSelect;
	char recvBuffer[256];
	for(;;){
		memcpy(&This->degradableSet, &This->untouchableSet, sizeof(fd_set));
		retSelect=select(This->maxFd+1, &This->degradableSet, NULL, NULL, NULL);
		if (retSelect <= 0){
			perror("Select");
			exit(1);
		}
		else {
			for(i=0, done = 0 ; i < This->maxFd+1 && done != retSelect; ++i){
				if (FD_ISSET(i, &This->degradableSet)){
					if (i == This->sockEcouteTcp){
						if ( (newClientSocket=accept(This->sockEcouteTcp, (struct sockaddr*)&newClientInfos, &newClientInfosSize)) == -1){
							perror("accept");
							exit(1);
						}
						FD_SET(newClientSocket, &This->untouchableSet);
						if (newClientSocket > This->maxFd){
							This->maxFd=newClientSocket;
						}
						printf("Nouveau client, socket : %d\n", newClientSocket);
					} else {
						if ( (retread=read(i, recvBuffer, sizeof(recvBuffer))) < 0){
							perror("Read Socket TCP\n.");
							exit(1);
						} else if (retread == 0){
							printf("Le client de la socket %d a quitté le jeu", i);
							close(i);
							FD_CLR(i, &This->untouchableSet);
							if (This->maxFd == i){
								updateMax(This, i);
							}
						}
					}
					++done;
				}
			}
		}
	}
	pthread_exit(NULL);
}

Reseau *New_Reseau()
{
	Reseau* This = malloc(sizeof(Reseau));
	if(!This) return NULL;
	Reseau_Init(This);
	This->Free=Reseau_New_Free;
	return This;
}

Reseau Reseau_Create()
{
	Reseau This;
	Reseau_Init(&This);
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


void Reseau_Init(Reseau* This){
	This->Clear=Reseau_Clear;
	pthread_mutex_init(&This->mutexMatricePropriete, NULL);
	//lancer la demande (Anybody out there ?) avec un timeout de deux secondes
	// --> etre connecté ou non
	This->portEcouteIncommingClients=5011;
	This->portEcouteInternalMessages=5012;
	This->portEcouteTcp=5013;
	creatEcouteInternalMessages(This);
	creatEcouteTcp(This);
	tenterConnection(This);
	if (This->nbConnectes == 0){
		printf("Aucune partie en cours\n");
	}
	creatIncommingClients(This);
}

void Reseau_Clear(Reseau *This){
	pthread_cancel(This->th_ThreadIncommingPlayer);
	if (pthread_join(This->th_ThreadIncommingPlayer, NULL)) {
		perror("pthread_join");
		return; // EXIT_FAILURE;
	}
	pthread_cancel(This->th_ThreadTcp);
	if (pthread_join(This->th_ThreadTcp, NULL)) {
		perror("pthread_join");
		return; // EXIT_FAILURE;
	}
	pthread_mutex_destroy(&This->mutexMatricePropriete);
	close(This->sockEcouteIncommingClients);
	close(This->sockEcouteInternalMessages);
	close(This->sockEcouteTcp);
	int i=0;
	for (; i< This->maxFd; ++i){
		if (FD_ISSET(i, &This->untouchableSet)){
			close(i);
		}
	}
}


int creatIncommingClients(Reseau *This)
{
	struct sockaddr_in  paramSocket;

	if ( (This->sockEcouteIncommingClients = socket(AF_INET, SOCK_DGRAM, 0) ) <0){
		perror("Creation sockEcouteIncommingClients");
		exit(1);
	}

	paramSocket.sin_family = AF_INET;
	paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
	paramSocket.sin_port=htons(This->portEcouteIncommingClients);

	if (bind (This->sockEcouteIncommingClients, (const struct sockaddr *)&paramSocket, sizeof(paramSocket))<0){
		perror("bind ");
		exit(1);
	}

	if (pthread_create(&This->th_ThreadIncommingPlayer, NULL, HandleIncommingPlayer, This)) {
		perror("pthread_create");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


void creatEcouteInternalMessages(Reseau *This)
{
	This=This;
}


int creatEcouteTcp(Reseau *This)
{
	struct sockaddr_in paramSocket;
	if ( (This->sockEcouteTcp = socket(AF_INET, SOCK_STREAM, 0) ) <0){
		perror("Creation sockEcouteIncommingClients");
		exit(1);
	}

	paramSocket.sin_family = AF_INET;
	paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
	paramSocket.sin_port=htons(This->portEcouteTcp);

	if (bind (This->sockEcouteTcp, (const struct sockaddr *)&paramSocket, sizeof(paramSocket))<0){
		perror("Bind sockEcouteIncommingClients");
		exit(1);
	}


	if (listen(This->sockEcouteTcp, 10) < 0){
		perror("Bind sockEcouteIncommingClients");
		exit(1);
	}

	FD_ZERO(&This->untouchableSet);
	FD_SET(This->sockEcouteTcp, &This->untouchableSet);
	This->maxFd=This->sockEcouteTcp;

	if (pthread_create(&This->th_ThreadTcp, NULL, HandleTcpPlayer, This)) {
		perror("pthread_create");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


void  tenterConnection(Reseau *This)
{
	struct sockaddr_in  dst, from;
	struct hostent *hp;
	struct ifconf ifc;
	struct ifreq *ifr;
	char liste[1024];
	int socketBroadcast;
	char buf[4096];
	int n, nc, on;
	unsigned int fromlen;

	// Création de socket UDP
	if ( (socketBroadcast = socket(AF_INET, SOCK_DGRAM, 0) ) <0){
		perror("socket de demande");
		exit(1);
	}

	// Passage de la socket en mode broadcast
	on = 1;	// on == 1 pour activer
	if(setsockopt(socketBroadcast, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on) )<0) {
		perror("socket opt");
		exit(1);
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


		/* Envoie d'un message à quelqu'un */
		dst.sin_port=htons(5011);
		dst.sin_family=AF_INET;
		sendto(socketBroadcast, "#1#blabla",9, 0, (struct sockaddr *) &dst, sizeof(dst));
	}

	/* Setting tu timeout de la socket pour les receptions*/
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(socketBroadcast, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}

	fromlen=sizeof(from);
	while ((nc=recvfrom(socketBroadcast, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen)) >= 0){
		buf[nc]='\0';
		int code, porttcp, portudp;
		sscanf(buf, "%d%d%d", &code, &porttcp, &portudp);
		printf("Recu : code:%d portTCP:%d, portUDP:%d", code, porttcp, portudp);
		hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
		printf(" de la part de %s(%d)\n",inet_ntoa(from.sin_addr),htons(from.sin_port));


		/* Connection TCP à chaque client
		 */
		int sockClient = socket(AF_INET, SOCK_STREAM, 0);

		struct sockaddr_in destTCP;
		destTCP.sin_family=AF_INET;
		destTCP.sin_port=htons(porttcp);
		destTCP.sin_addr=from.sin_addr;
		int retConnect;
		retConnect=connect(sockClient, (struct sockaddr*)&destTCP, sizeof(struct sockaddr_in));
		if (retConnect < 0){
			perror("Connect");
			continue;
		}
		This->nbConnectes+=1;
		FD_SET(sockClient, &This->untouchableSet);
		/* Traitement de la donnée sensible :
		 * pthread_mutex_lock(pthread_mutex_t *mut);
		 */
		if (pthread_mutex_lock(&This->mutexMatricePropriete) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}
		/* Fin du traitement de la donnée
		 * pthread_mutex_unlock(pthread_mutex_t *mut);
		 */
		if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
			perror("pthread_mutex_lock");
			exit(1);
		}

	}
	if (errno == EAGAIN){
		/* Soit on s'est connecté à tous les clients, soit on est seul */
		return;
	} else {
		perror("recvfrom");
		exit(1);
	}
}
