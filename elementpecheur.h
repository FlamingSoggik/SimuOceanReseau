#ifndef ELEMENTPECHEUR_H
#define ELEMENTPECHEUR_H

#include "element.h"
#include "Bool.h"
#include "listetype.h"

#define ERR_TYPE_NOT_ANIMAL -1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ElementPecheur
{

/*************************** Partie Héritée de Element ***************************/
		struct Case* caseParent;
		Type type;
		void (*Free)(struct Element *This);
		void (*Clear)(struct Element *This);
        char* (*serialize)(struct Element* This);


/*********************************************************************************/

/************************* Partie propre à ElementPecheur ************************/
		///
		/// \brief sac Alias besace : masse de poisson disponible pour la consruction du pont
		///
		uint16_t sac;
		uint16_t (*GetSac)(struct ElementPecheur*);
		void (*SetSac)(struct ElementPecheur* , uint16_t);

		uint16_t longueurCanne;
		uint16_t (*GetLongueurCanne)(struct ElementPecheur*);
		void (*SetLongueurCanne)(struct ElementPecheur* , uint16_t);

		uint16_t tailleFilet;
		uint16_t (*GetTailleFilet)(struct ElementPecheur*);
		void (*SetTailleFilet)(struct ElementPecheur* , uint16_t);

		uint16_t distanceDeplacement;
		uint16_t (*GetDistanceDeplacement)(struct ElementPecheur*);
		void (*SetDistanceDeplacement)(struct ElementPecheur* , uint16_t);

		struct ListeType* listeDePeche;

		uint16_t PositionInitialeX;
		uint16_t (*GetPositionInitialeX)(struct ElementPecheur*);
		void (*SetPositionInitialeX)(struct ElementPecheur* , uint16_t);

		uint16_t PositionInitialeY;
		uint16_t (*GetPositionInitialeY)(struct ElementPecheur*);
		void (*SetPositionInitialeY)(struct ElementPecheur* , uint16_t);

		void (*testVictory)(struct ElementPecheur* , int16_t);

		char estSelectionne;
		void (*pecheParCanne)(struct ElementPecheur*, char*);
		Type (*pecheParCanneSDL)(struct ElementPecheur*, int16_t, int16_t);
		void (*pecheParFilet)(struct ElementPecheur*, char*);
		void (*pecheParFiletSDL)(struct ElementPecheur*, int16_t, int16_t);
		Bool (*deplacement)(struct ElementPecheur*, char);
		Bool (*construirePont)(struct ElementPecheur*, char);
		void (*mourir)(struct ElementPecheur*); // --> fait
		void (*lancerpoisson)(struct ElementPecheur*);
		void (*reinitSac)(struct ElementPecheur*);
		Bool (*peutPecher)(struct ElementPecheur*, Type);


/*********************************************************************************/
} ElementPecheur;

//Constructeurs
///
/// \brief New_ElementPecheur Créé un ElementPecheur de manière dynamique
/// \param c Case dans laquelle se trouve le pecheur
/// \return Pointeur vers l'ElementPEcheur nouvellement créé
///
ElementPecheur* New_ElementPecheur(Case *c);


//Initialisateur

///
/// \brief ElementPecheur_Init Initialise l'ElementPecheur avec les pointeur sur fonction et ls attributs
/// \param c Case dans laquelle se trouve l'ElementPecheur
/// \param This Pointeur sur l'ElementPecheur
/// \return Code d'erreur
///
char ElementPecheur_Init(Case *c, ElementPecheur* This);

//Destructeurs

//Others

///
/// \brief ElementPecheur_pecheParCanne Peche du pecheur avec la canne à peche
/// \param This Pointeur sur le pecheur
/// \param buffer Chaine de caractere : enchainement de direction à appliquer à partir du pecheur pour trouver la case de lancé
///
void ElementPecheur_pecheParCanne(ElementPecheur* This, char *buffer);
Type ElementPecheur_pecheParCanneSDL(ElementPecheur *This, int16_t x, int16_t y);

///
/// \brief ElementPecheur_pecheParFilet Peche du pecheur avec un filet
/// \param This Pointeur sur le pecheur
/// \param buffer Chaine de caractere : enchainement de direction à appliquer à partir du pecheur pour trouver la case de lancé
///
void ElementPecheur_pecheParFilet(ElementPecheur* This, char *buffer);
void ElementPecheur_pecheParFiletSDL(ElementPecheur *This, int16_t x, int16_t y);
///
/// \brief ElementPecheur_deplacement Deplacement du pecheur
/// \param This Pointeur sur le pecheur
/// \param direction Direction du pecheur : '1' .. '9'
/// \return
///
Bool ElementPecheur_deplacement(ElementPecheur* This, char direction);

///
/// \brief ElementPecheur_construirePont Construction d'un pont
/// \param This Pointeur sur le pecheur
/// \param direction Direction de la construction du pont: '1' .. '9'
/// \return
///
Bool ElementPecheur_construirePont(ElementPecheur* This, char direction);

///
/// \brief ElementPecheur_mourir Fait réapparaitre l'ElementPecheur à sa position initiale
/// \param This Pointeur sur l'ElementPecheur à faire respawn
///
void ElementPecheur_mourir(ElementPecheur* This);
void ElementPecheur_lancerpoisson(ElementPecheur* This); // Non implementé, implémentation prévue pour le mode multijoueur
Bool ElementPecheur_peutPecher(ElementPecheur* This, Type t);
///
/// \brief ElementPecheur_reinitSac Remet le sac du pecheur à la taille originale
/// \param This Pointeur sur le pecheur
///
void ElementPecheur_reinitSac(ElementPecheur* This);

void ElementPecheur_Clear(Element *This);
void ElementPecheur_New_Free(Element* This);


uint16_t ElementPecheur_getSac(struct ElementPecheur *This);
void ElementPecheur_setSac(struct ElementPecheur *This, uint16_t toset);
uint16_t ElementPecheur_getLongueurCanne(struct ElementPecheur *This);
void ElementPecheur_setLongueurCanne(struct ElementPecheur *This, uint16_t toset);
uint16_t ElementPecheur_getTailleFilet(struct ElementPecheur *This);
void ElementPecheur_setTailleFilet(struct ElementPecheur *This, uint16_t toset);
uint16_t ElementPecheur_getDistanceDeplacement(struct ElementPecheur *This);
void ElementPecheur_setDistanceDeplacement(struct ElementPecheur *This, uint16_t toset);
uint16_t ElementPecheur_getPositionInitialex(struct ElementPecheur *This);
void ElementPecheur_setPositionInitialex(struct ElementPecheur *This, uint16_t toset);
uint16_t ElementPecheur_getPositionInitialey(struct ElementPecheur *This);
void ElementPecheur_setPositionInitialey(struct ElementPecheur *This, uint16_t toset);


void ElementPecheur_testVictory(struct ElementPecheur *This, int16_t joueur);

char* ElementPecheur_serialize(Element *This);


#ifdef __cplusplus
}
#endif

#endif // ELEMENTPECHEUR_H
