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
						Client* c = New_Client();
						unsigned int onSenTape=sizeof(struct sockaddr_in);
						c->socketTCP=newClientSocket;
						if (getpeername(newClientSocket, (struct sockaddr*)&c->from, &onSenTape) < 0){
							perror("getpeername");
							++done;
							continue;
						}
						write(c->socketTCP, "#2#ports?\n", 9);
						This->clients->Push(This->clients, c);
						printf("Nouveau Client ajoutté au jeu, adresse ip: %s portTCP: %d\n", inet_ntoa(c->from.sin_addr),htons(c->from.sin_port));
					} else {
						if ( (retread=read(i, recvBuffer, sizeof(recvBuffer))) < 0){
							printf("Un client quitte la partie.\n");
							This->clients->remove(This->clients, i);
							FD_CLR(i, &This->untouchableSet);
							if (This->maxFd == i){
								updateMax(This, i);
							}
						}
						else if (retread > 0){
							printf("Message recu sur une socket TCP\n");
							struct sockaddr_in from;
							unsigned int onSenTape;
							//Quel est ton port udpInterne
							if (strncmp(recvBuffer, "#2#", 3) == 0){
								if (getpeername(newClientSocket, (struct sockaddr*)&from, &onSenTape) < 0){
									perror("getpeername");
									++done;
									continue;
								}
								Client *c=This->clients->getFrom(This->clients, from);
								printf("Avant le recieve : %d", c->portInterneUDP);
								sscanf(recvBuffer+3, "%" SCNd16, &c->portInterneUDP);
								printf("Apres le recieve : %d", c->portInterneUDP);
							}
						}
						else {
							printf("Un client quitte la partie.\n");
							This->clients->remove(This->clients, i);
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
	This->clients=New_ListeClient();
	pthread_mutex_init(&This->mutexMatricePropriete, NULL);
	This->portEcouteIncommingClients=5020;
	This->portEcouteInternalMessages=5010;
	This->portEcouteTcp=5000;
	creatEcouteInternalMessages(This);
	creatEcouteTcp(This);

	//lancer la demande (Anybody out there ?) avec un timeout de deux secondes
	// --> etre connecté ou non
	tenterConnection(This);
	if (This->clients->taille == 0){
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
	This->clients->Free(This->Free);
}

int creatIncommingClients(Reseau *This)
{
	struct sockaddr_in paramSocket;

	if ( (This->sockEcouteIncommingClients = socket(AF_INET, SOCK_DGRAM, 0) ) <0){
		perror("Creation sockEcouteIncommingClients");
		exit(1);
	}

	paramSocket.sin_family = AF_INET;
	paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
	paramSocket.sin_port=htons(This->portEcouteIncommingClients);

	if (bind (This->sockEcouteIncommingClients, (const struct sockaddr *)&paramSocket, sizeof(paramSocket)) < 0){
		printf("%d", This->portEcouteIncommingClients);
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
		perror("Creation sockEcouteTCP");
		exit(1);
	}

	int i;
	for (i=0; i< 10; ++i){
		paramSocket.sin_family = AF_INET;
		paramSocket.sin_addr.s_addr= htonl(INADDR_ANY);
		paramSocket.sin_port=htons(This->portEcouteTcp);

		if (bind (This->sockEcouteTcp, (const struct sockaddr *)&paramSocket, sizeof(paramSocket))<0){
			printf("Port d'écoute TCP %d déjà en cours d'utilisation -->inc\n", This->portEcouteTcp);
			++This->portEcouteTcp;
			continue;
		} else {
			break;
		}
	}
	This->portEcouteIncommingClients=This->portEcouteTcp+100;
	This->portEcouteInternalMessages=This->portEcouteTcp+200;

	if (listen(This->sockEcouteTcp, 10) < 0){
		perror("Listen sockEcouteTCP");
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


void tenterConnection(Reseau *This)
{
	struct sockaddr_in  dst, from;
	struct hostent *hp;
	struct ifconf ifc;
	struct ifreq *ifr;
	int socketBroadcast;
	char buf[4096], liste[1024], buff[10];
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
		dst.sin_port=htons(5100);
		dst.sin_family=AF_INET;
		sendto(socketBroadcast, "#1#", 3, 0, (struct sockaddr *) &dst, sizeof(dst));
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
		/* Connection TCP à chaque client */
		Client *c = New_Client();
		buf[nc]='\0';
		int code, porttcp, portudp;
		sscanf(buf, "%d%d%d", &code, &porttcp, &portudp);
		printf("Recu : code:%d portTCP:%d, portUDP:%d", code, porttcp, portudp);
		hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
		printf(" de la part de %s(%d)\n",inet_ntoa(from.sin_addr),htons(from.sin_port));

		c->socketTCP = socket(AF_INET, SOCK_STREAM, 0);

		struct sockaddr_in destTCP;
		destTCP.sin_family=AF_INET;
		destTCP.sin_port=htons(porttcp);
		destTCP.sin_addr=from.sin_addr;
		int retConnect;

printf("On tente de se connecter à un client\n");
		retConnect=connect(c->socketTCP, (struct sockaddr*)&destTCP, sizeof(struct sockaddr_in));
		if (retConnect < 0){
			perror("Connect");
			continue;
		}
printf("Connexion réussie\n");
		sprintf(buff, "%d\n", This->portEcouteInternalMessages);

		printf("SLEEP\n");
		sleep(10);
		printf("END SLEEP\n");
		if (write(c->socketTCP, buff, strlen(buff) <= 0)){
			perror("PERROR Write");
		}
		else {
printf("Write réussie\n");
		}

		This->clients->Push(This->clients, c);
		FD_SET(c->socketTCP, &This->untouchableSet);
		if (c->socketTCP > This->maxFd){
			This->maxFd=c->socketTCP;
		}

		/* Traitement de la donnée sensible :
		 * pthread_mutex_lock(pthread_mutex_t *mut);
		 */
//		if (pthread_mutex_lock(&This->mutexMatricePropriete) < 0){
//			perror("pthread_mutex_lock");
//			exit(1);
//		}
		/* Fin du traitement de la donnée
		 * pthread_mutex_unlock(pthread_mutex_t *mut);
		 */
//		if (pthread_mutex_unlock(&This->mutexMatricePropriete) < 0){
//			perror("pthread_mutex_lock");
//			exit(1);
//		}

	}
	if (errno == EAGAIN){
		/* Timeout : Soit on s'est connecté à tous les clients, soit on est seul */
		return;
	} else {
		perror("recvfrom");
		exit(1);
	}
}

void unSerialize(char* str, Grille* g){
	uint16_t xCase, yCase, nbr, i;
	uint16_t type, dernierRepas, sasiete, derniereReproduction;
	uint16_t sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY;
	//On créé un strean (via un fichier) ---> probablement assez sale
	FILE* stream=fopen("/tmp/streamSimuOcean", "r+");
	fprintf(stream, "%s", str);
	fseek(stream, 0L, SEEK_SET);
	fscanf(stream, "%" SCNd16, &xCase);
	fscanf(stream, "%" SCNd16, &yCase);
	fscanf(stream, "%" SCNd16, &nbr);
	for (i=0; i<nbr ; ++i){
		//Scan d'un élément
		fscanf(stream, "%" SCNd16, &type);
		if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
			fscanf(stream, "%" SCNd16, &dernierRepas);
			fscanf(stream, "%" SCNd16, &sasiete);
			fscanf(stream, "%" SCNd16, &derniereReproduction);
			ElementAnimal *ea = New_ElementAnimal(&g->tab[xCase][yCase], type);
			ea->SetDernierRepas(ea, dernierRepas);
			ea->SetSasiete(ea, sasiete);
			ea->SetDerniereReproduction(ea, derniereReproduction);
			g->tab[xCase][yCase].liste->Clear(g->tab[xCase][yCase].liste);
			g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)ea);
		}
		else if (type == TERRE){
			ElementTerre *t = New_ElementTerre(&g->tab[xCase][yCase]);
			g->tab[xCase][yCase].liste->Clear(g->tab[xCase][yCase].liste);
			g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)t);
		}
		else if (type == PONT){
			ElementPont*t = New_ElementPont(&g->tab[xCase][yCase]);
			g->tab[xCase][yCase].liste->Clear(g->tab[xCase][yCase].liste);
			g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)t);
		}
		else if (type == PECHEUR){
			fscanf(stream, "%" SCNd16, &sac);
			fscanf(stream, "%" SCNd16, &longueurCanne);
			fscanf(stream, "%" SCNd16, &tailleFilet);
			fscanf(stream, "%" SCNd16, &distanceDeplacement);
			fscanf(stream, "%" SCNd16, &PositionInitialeX);
			fscanf(stream, "%" SCNd16, &PositionInitialeY);

			ElementPecheur *p = New_ElementPecheur(&g->tab[xCase][yCase]);
			p->SetSac(p, sac);
			p->SetLongueurCanne(p, longueurCanne);
			p->SetTailleFilet(p, tailleFilet);
			p->SetDistanceDeplacement(p, distanceDeplacement);
			p->SetPositionInitialeX(p, PositionInitialeX);
			p->SetPositionInitialeY(p, PositionInitialeY);
			g->tab[xCase][yCase].liste->Clear(g->tab[xCase][yCase].liste);
			g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)p);
		}
	}
}

