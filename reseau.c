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

void* TimerFunction(void* arg){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    Reseau* This = (Reseau*)arg;
    sleep(1);
    This->nbrReponseAttendue[4]=0;
    pthread_cond_signal(&This->condEverythingRecieved);
    return EXIT_SUCCESS;
}

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
            printf("Continue car osrdre trop court\n");
            continue;
        }
        if (strncmp(recvBuffer, "#1#", 3) == 0){
            hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
            printf("Demande de connection recue de la part de %s:%d\n",inet_ntoa(from.sin_addr),htons(from.sin_port));

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
            perror("recvfrom");
            exit(1);
        }
        else if (sizeRecieved < 3){
            printf("Continue car ordre trop court\n");
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
            sendto(This->sockEcouteIncommingClients, toSend, strlen(toSend)+1, 0, (struct sockaddr *) &dst, sizeof(dst));
            free(toSend);
        }
        else if (strncmp(recvBuffer, "#3a", 3) == 0){
            printf("Réponse à notre demande de carte de la part de %s:%d\n",inet_ntoa(from.sin_addr),htons(from.sin_port));
            Client *c = This->clients->getFrom(This->clients, from);
            unSerialize(recvBuffer+3, This->g, c);
            printf("FIN UNSERIALIZED\n");
        }
        else if (strncmp(recvBuffer, "#4q", 3) == 0){
           //Donner la propriétée
        }
        else if (strncmp(recvBuffer, "#4a", 3) == 0){
            //mettre a jour la matrice de propriété
            if (This->nbrReponseAttendue[4]-- == 0){
                pthread_cancel(This->th_AnswerTimeout4);
                pthread_cond_signal(&This->condEverythingRecieved);
            }
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

            //sendto(This->sockEcouteIncommingClients, "#1#", 3, 0, (struct sockaddr *) &dst, sizeof(dst));
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
	int newClientSocket, i, done, retread, retSelect;
    char recvBuffer[4096];
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
                    ++done;
                    if (i == This->sockEcouteTcp){
                        Client* c = New_Client();
						if ( (newClientSocket=accept(This->sockEcouteTcp, (struct sockaddr*)&newClientInfos, &newClientInfosSize)) == -1){
							perror("accept");
							exit(1);
                        }
                        c->socketTCP=newClientSocket;
                        unsigned int onSenTape=sizeof(struct sockaddr_in);
                        if (getpeername(newClientSocket, (struct sockaddr*)&c->from, &onSenTape) < 0){
                            perror("getpeername");
                            continue;
                        }
                        This->clients->Push(This->clients, c);
						FD_SET(newClientSocket, &This->untouchableSet);
						if (newClientSocket > This->maxFd){
							This->maxFd=newClientSocket;
                        }
                        printf("Nouveau Client ajoutté au jeu, adresse ip: %s portTCP: %d socket no : %d\n", inet_ntoa(c->from.sin_addr),htons(c->from.sin_port), i);
                    }
                    else if (i == This->selfPipe[0]){
                        char buf[2];
                        read(This->selfPipe[0], buf, sizeof(buf));
                    }
                    else {
                        if ( (retread=read(i, recvBuffer, sizeof(recvBuffer))) <= 0){
							printf("Un client quitte la partie.\n");
							This->clients->remove(This->clients, i);
							FD_CLR(i, &This->untouchableSet);
							if (This->maxFd == i){
								updateMax(This, i);
							}
                        }
                        else {
							struct sockaddr_in from;
							unsigned int onSenTape;
							//Quel est ton port udpInterne
                            if (strncmp(recvBuffer, "#2#", 3) == 0){
								if (getpeername(newClientSocket, (struct sockaddr*)&from, &onSenTape) < 0){
                                    perror("getpeername");
									continue;
								}
                                Client *c=This->clients->getFrom(This->clients, from);
                                sscanf(recvBuffer+3, "%" SCNd16, &c->from.sin_port);
                            }
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
	This->clients=New_ListeClient();
    pthread_mutex_init(&This->mutexMatricePropriete, NULL);
    pthread_cond_init(&This->condEverythingRecieved, NULL);
    This->portEcouteInternalMessages=5000;
    This->portEcouteTcp=5100;
    This->portEcouteIncommingClients=5200;

    int i;
    for (i=0; i<10; ++i){
        This->nbrReponseAttendue[0]=0;
    }
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
    for (i=0; i < This->maxFd+1; ++i){
        if (FD_ISSET(i, &This->untouchableSet)){
            printf("The master set contain : %d\n", i);
        }
    }
	creatEcouteInternalMessages(This);
	creatEcouteTcp(This);

	//lancer la demande (Anybody out there ?) avec un timeout de deux secondes
	// --> etre connecté ou non
    tenterConnection(This);
    if (This->clients->taille != 0){
        askForCarte(This);
    }
    creatIncommingClients(This);
    printf("Fin init\n");
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
    pthread_cancel(This->th_ThreadInternalMessages);
    if (pthread_join(This->th_ThreadInternalMessages, NULL)) {
        perror("pthread_join");
        return; // EXIT_FAILURE;
    }
    if (pthread_join(This->th_AnswerTimeout4, NULL)) {
        perror("pthread_join");
        return; // EXIT_FAILURE;
    }
    close(This->selfPipe[0]);
    close(This->selfPipe[1]);
	pthread_mutex_destroy(&This->mutexMatricePropriete);
    pthread_cond_destroy(&This->condEverythingRecieved);
	close(This->sockEcouteIncommingClients);
	close(This->sockEcouteInternalMessages);
	close(This->sockEcouteTcp);
    This->clients->Free(This->clients);
}

int creatIncommingClients(Reseau *This)
{
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

int creatEcouteInternalMessages(Reseau *This)
{
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
        } else {
            break;
        }
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

int creatEcouteTcp(Reseau *This)
{
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
        exit(1);
    }

	if (listen(This->sockEcouteTcp, 10) < 0){
		perror("Listen sockEcouteTCP");
		exit(1);
	}

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


        /* Envoie à tout le monde d'une demande de connection */
        dst.sin_port=htons(5200);
		dst.sin_family=AF_INET;
		sendto(socketBroadcast, "#1#", 3, 0, (struct sockaddr *) &dst, sizeof(dst));
	}

    /* Setting du timeout de la socket pour les receptions*/
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(socketBroadcast, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}

	fromlen=sizeof(from);
	while ((nc=recvfrom(socketBroadcast, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen)) >= 0){
		/* Connection TCP à chaque client */
        printf("Nouveau serveur à qui se connecter\n");
		Client *c = New_Client();
		buf[nc]='\0';
		int code, porttcp, portudp;
        sscanf(buf, "%d%d%d", &code, &porttcp, &portudp);
		hp=gethostbyaddr( &from.sin_addr,sizeof(from.sin_addr) , AF_INET);
        hp=hp;
        printf("Recu : code:%d portTCP:%d, portUDP:%d de la part de %s(%d)\n", code, porttcp, portudp ,inet_ntoa(from.sin_addr),htons(from.sin_port));

        c->socketTCP = socket(AF_INET, SOCK_STREAM, 0);
        c->from.sin_addr=from.sin_addr;
        c->from.sin_port=htons(portudp);

		struct sockaddr_in destTCP;
		destTCP.sin_family=AF_INET;
		destTCP.sin_port=htons(porttcp);
		destTCP.sin_addr=from.sin_addr;
		int retConnect;

		retConnect=connect(c->socketTCP, (struct sockaddr*)&destTCP, sizeof(struct sockaddr_in));
		if (retConnect < 0){
			perror("Connect");
			continue;
        }
        sprintf(buff, "#2#%d\n", This->portEcouteInternalMessages);
        if (write(c->socketTCP, buff, strlen(buff)) <= 0){
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
//			perror("pthread_mutex_lock");
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
    for (i=0; i<This->clients->taille; ++i){
        c=This->clients->getNieme(This->clients, i);
        printf("Demande de partie de carte envoyée à : %s:%d\n", inet_ntoa(c->from.sin_addr), htons(c->from.sin_port));
        if (sendto(This->sockEcouteInternalMessages, "#3q", 3, 0, (struct sockaddr*)&c->from, sizeof(c->from)) == -1){
            perror("Send to __LINE__");
        }
    }
}

void askForProperty(Reseau *This, ListeCase* lcas){
    int i, j, offset;
    Case *cas;
    Client *cli;
    ListeClient* lcli = New_ListeClient();
    char *propDemandeSur;
    for (i=0; i<lcas->taille; ++i){
        cas = lcas->getNieme(lcas, i);
        if (cas->proprietaire == NULL)
            continue;
        if ( (cli = lcli->getFrom(lcli, cas->proprietaire->from)) == NULL){
            lcli->Push(lcli, cas->proprietaire);
            cli=cas->proprietaire;
        }
        cli->casesTo->Push(cli->casesTo, cas);
        This->nbrReponseAttendue[4]++;
    }
    for(i=0 ; i< lcli->taille; ++i){
        cli=lcli->getNieme(lcli, i);
        // (5+1) = 1 int + 1\n * 2 car deux coordonnées
        propDemandeSur=malloc(((5+1)*2*(cli->casesTo->taille*2+1)+3+1)*sizeof(char));
        offset = sprintf(propDemandeSur, "#4q%d\n", cli->casesTo->taille);

        for(j=0; i<cli->casesTo->taille; ++j){
            cas=cli->casesTo->getNieme(cli->casesTo, j);
            offset += sprintf(propDemandeSur+offset, "%d\n%d\n", cas->posX, cas->posY);
        }
        offset += sprintf(propDemandeSur+offset, "#");

        printf("Demande de propriété de case envoyée à : %s:%d\n", inet_ntoa(cas->proprietaire->from.sin_addr), htons(cas->proprietaire->from.sin_port));
        if (sendto(This->sockEcouteInternalMessages, propDemandeSur, strlen(propDemandeSur), 0, (struct sockaddr*)&cas->proprietaire->from, sizeof(cas->proprietaire->from)) == -1){
            perror("Send to __LINE__");
        }
        free(propDemandeSur);
        cli->casesTo->Free(cli->casesTo);
    }
    if (pthread_create(&This->th_AnswerTimeout4, NULL, TimerFunction, This)){
        perror("pthread_create");
        return;
    }
    lcli->Free(lcli);
}

void unSerialize(char* str, Grille* g, Client *cli){
    uint16_t xCase, yCase, nbrElem, nbrCase, i, k;
	uint16_t type, dernierRepas, sasiete, derniereReproduction;
	uint16_t sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY;
	//On créé un strean (via un fichier) ---> probablement assez sale
    /* Strem via pipe : ne fonctionne pas
    int fdRW[2];
    if (pipe(fdRW) < 0){
        perror("Pipe");
    }
    FILE* readStream=fdopen(fdRW[0], "r");
    FILE* writeStream=fdopen(fdRW[1], "w");
    //FILE* readStream=fopen("fic.txt", "r");
    fprintf(writeStream, "%s", str);
    //fprintf(debugFic, "%s", str);
    //fclose(debugFic);
    */
    FILE* readStream=fopen("SimuOceanStream.txt", "w+");
    if (readStream == NULL){
        perror("fopen");
        exit(-1);
    }
    //FILE* readStream=fopen("fic.txt", "r");
    fprintf(readStream, "%s", str);
    rewind(readStream);
    //fprintf(debugFic, "%s", str);
    //fclose(debugFic);
    fscanf(readStream, "%" SCNd16, &nbrCase);
    printf("Nombre de cases : %d\n", nbrCase);
    for (k=0; k<nbrCase; ++k){
        fscanf(readStream, "%" SCNd16, &xCase);
        fscanf(readStream, "%" SCNd16, &yCase);
        fscanf(readStream, "%" SCNd16, &nbrElem);
        g->tab[xCase][yCase].liste->Clear(g->tab[xCase][yCase].liste);
        g->tab[xCase][yCase].proprietaire=cli;
        printf("Case[%d][%d] : ", xCase, yCase);
        if (nbrElem == 0){
            printf("\n");
        }
        for (i=0; i<nbrElem ; ++i){
            //Scan d'un élément
            fscanf(readStream, "%" SCNd16, &type);
            if (type >= TYPEMINANIMAL && type <= TYPEMAXANIMAL){
                fscanf(readStream, "%" SCNd16, &dernierRepas);
                fscanf(readStream, "%" SCNd16, &sasiete);
                fscanf(readStream, "%" SCNd16, &derniereReproduction);
                printf("%d\t%d\t%d\t%d\n", type, dernierRepas, sasiete, derniereReproduction);
                ElementAnimal *ea = New_ElementAnimal(&g->tab[xCase][yCase], type);
                ea->SetDernierRepas(ea, dernierRepas);
                ea->SetSasiete(ea, sasiete);
                ea->SetDerniereReproduction(ea, derniereReproduction);
                g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)ea);
            }
            else if (type == TERRE){
                printf("%d\n", type);
                ElementTerre *t = New_ElementTerre(&g->tab[xCase][yCase]);
                g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)t);
            }
            else if (type == PONT){
                printf("%d\n", type);
                ElementPont*t = New_ElementPont(&g->tab[xCase][yCase]);
                g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)t);
            }
            else if (type == PECHEUR){
                fscanf(readStream, "%" SCNd16, &sac);
                fscanf(readStream, "%" SCNd16, &longueurCanne);
                fscanf(readStream, "%" SCNd16, &tailleFilet);
                fscanf(readStream, "%" SCNd16, &distanceDeplacement);
                fscanf(readStream, "%" SCNd16, &PositionInitialeX);
                fscanf(readStream, "%" SCNd16, &PositionInitialeY);

                printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", type, sac, longueurCanne, tailleFilet, distanceDeplacement, PositionInitialeX, PositionInitialeY);
                ElementPecheur *p = New_ElementPecheur(&g->tab[xCase][yCase]);
                p->SetSac(p, sac);
                p->SetLongueurCanne(p, longueurCanne);
                p->SetTailleFilet(p, tailleFilet);
                p->SetDistanceDeplacement(p, distanceDeplacement);
                p->SetPositionInitialeX(p, PositionInitialeX);
                p->SetPositionInitialeY(p, PositionInitialeY);
                g->tab[xCase][yCase].liste->Push(g->tab[xCase][yCase].liste, (Element*)p);
            }
        }
    }
}

