//On charge les bibliothèques nécssaires pour les différents périphériques
#include <Arduino.h>

#include <Tone.h>
Tone buzz;

//#include <TM1637.h>
#include "SevenSegmentTM1637.h"
#include "SevenSegmentExtended.h"
//#include "SevenSegmentFun.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

//On donne un nom aux boutons et leur pin de connexion
#define selectNbJoueurs 17 

//Boutons Player 1
#define P1_Bout 14
#define P1_Int 2

bool memoire_P1 = 1;
int etat_P1;
bool coolDown_P1=true;
int rang_P1;
bool victoireManche_P1;

//Boutons Player 2
#define P2_Bout 15
#define P2_Int 3

bool memoire_P2 = 1;
int etat_P2;
bool coolDown_P2=true;
int rang_P2;
bool victoireManche_P2;

//Buzzer
#define pinBuzz 9


//Définition de l'afficheur des scores
#define CLK 11
#define DIO 12
SevenSegmentExtended score(CLK, DIO);

//Définition de la bande de leds neopixels
#define NEOPIN 10
#define NUM_LEDS 16
#define BRIGHTNESS 7
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIN, NEO_RGBW + NEO_KHZ800);

//on déclare quelques variables
int scoreWin = 20;        //score a atteindre pour la victoire
int tauxReussiteOrdi = 8; //Taux de réussite de l'ordi en singlePlayer (il trouve juste n foix sur 10)
int nbJoueurs;            //Pour jouer seul =1 ou à deux =2
int tabLeds[NUM_LEDS];    //on crée un tableau avec autant de cases que de leds
int nbR;                  //on crée des variables pour stocker le nombres de cases de chaque couleur
int nbG;
int nbB;
int nbY;

int P1_score = 0;   //On met le score de P1 à 0
int P2_score = 0;   //On met le score de P1 à 0
char choix_P1 = 'Z'; //On met le choix de P1 sur une valeur bidon "Z"
char choix_P2 = 'Z'; //On met le choix de P1 sur une valeur bidon "Z"

char cible;
char cible_P1;
char cible_P2;
char currentColor;
unsigned long temps_P1;
unsigned long temps_P2;
long randNumber;
long randDelay;
bool relance;
//bool recommencer;

bool partieEnCours = false;


void setup() {
  //Pour ne pas avoir toujours la même cible au hasard en premier, on choppe les parasites de la pin 16
  //en tant que "randomSeed"
  randomSeed(analogRead(16));

  //On déclare les boutons en entrées
  pinMode(P1_Bout, INPUT);
  pinMode(P2_Bout, INPUT);

  //Le buzzer en sortie
  pinMode(pinBuzz, OUTPUT);
  buzz.begin(pinBuzz);

  //Le selecteur du nombre de joueurs
  pinMode(selectNbJoueurs, INPUT);
  
  //on joue le générique de début :
 sonGenerique();

  //On initialise l'afficheur de scores
  score.begin();
  score.setBacklight(100);  //on règle la luminausité

  //On initialise la bande de leds neopixels
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show();
  delay(500);

  //On démarre le port série
  Serial.begin(9600);

  //On detecte le nombre de joueurs
  if (analogRead(selectNbJoueurs) >= 512){
    nbJoueurs = 2;
  }
  else {
    nbJoueurs = 1;
  }
  nbJoueurs = 2;
}

void loop() {
  //Serial.println("");
/*  while ((choix_P1 == 'Z')||(partieEnCours = false)) {
    LectureBoutons();
    Serial.println("attente");
    }
    if (choix_P1 == 'B'){
    duelCible();
    }
    if (choix_P1 == 'R'){
    duelCompte();
    }
    if (choix_P1 == 'Y'){
    duelSerie();
    }
    if (choix_P1 == 'G'){
    //duelMix();
 //   }
  */
  //On check si la partie est en cours
  //
  // if(!partieEnCours){
  // lobby();
  // }
  // while (recommencer == 0) {
  int t = random (1,4);
 //int t = 1;
  if (t == 1){duelCible();}
  if (t == 2){duelCompte();}
  if (t == 3){duelSerie();}
  //duelCompte();
  //duelSerie();
  // }
  //Fin de la loop principale
}

void duelCible() {
  partieEnCours = true;
  //On affiche le score  (et au passage on regarde si on a un gagnant)
  AfficheScore();

  //On envoie une Cible au hasard
  //On fait tourner un nombre de fois aléatoire le choix de cible pour que ça prenne jamais le même temps à tomber
  randDelay = random(20, 50);
  for (int i = 0; i <= randDelay; i++) {
    CalculCible();
  }
  buzz.play(1000, 50);
  delay(50);
  buzz.play(1500, 100);
  //Serial.print("Cible :  ");Serial.println(cible);

  //On lit les bouton
  //Tant que les deux joueurs n'ont pas fait un choix on attends
  // en mode 1 seul joueur c'est l'ordinateur qui fera le choix du joueur 2
  Serial.print("cible =  ");
  Serial.println(cible);
  while (choix_P1 == 'Z' || choix_P2 == 'Z') {
    LectureBoutons(); 
   
  }

  //On réagit fonction des boutons et du temps de réponse
  ReactionBoutonsCible();
}

void duelCompte() {
  //Dans ce mode de jeu toute la bande de neopixels affiche des couleurs au hasard et il s'agit de trouver la couleur dominante
  partieEnCours = true;
  relance = true;
  AfficheScore();
  while (relance == true) {
    remplitBandeCompte(); //on relance le remplissage tant qu'on n'a pas une couleurs dominante unique
  }
  buzz.play(1000, 50);
  delay(50);
  buzz.play(1500, 100);
  while (choix_P1 == 'Z' || choix_P2 == 'Z') {
    LectureBoutons();
  }
  ReactionBoutonsCompte();

}

void duelSerie() {
  /*Dans ce mode de jeu la bande affiche a gauche et à droite deux séries aléatoires en ligne
     (sauf les deux pixels du milieu qui restent noirs)
     chaque joueur doit faire au clavier les couleurs de sa serie dans l'ordre
     à chaque errreur on recule d'un pas en arriere.
  */
  partieEnCours = true;
  relance = true;
  Serial.println("Debut");
  AfficheScore();
  remplitBandeSerie();
  rang_P1 = 0;
  rang_P2 = 0;
  victoireManche_P1 = false;
  victoireManche_P2 = false;
  while (relance == true) {
    //Serial.println("Relance");
    choix_P1 = 'Z';
    choix_P2 = 'Z';
    LectureBoutons();
    reactionBoutonsSerie();
    if ((victoireManche_P1 == true) && (victoireManche_P2 == true)) {
      //égalité :
      sonWow();
      P1_score++;
      P2_score++;
    }
    if ((victoireManche_P1 == true) && (victoireManche_P2 == false)) {
      //Victoire du joueur 1:
      P1_score+=2;

    }
    if ((victoireManche_P1 == false) && (victoireManche_P2 == true)) {
      //Victoire du joueur 2:
      P2_score+=2;
    }
  }

}


void reactionBoutonsSerie() {
  
  //on traite la réponse de P1 :
  if (choix_P1 == (tabLeds[rang_P1])) {   //Si le choix correspond à la première case du tableau
    sonBlip(1);
    strip.setPixelColor(rang_P1, (codeCouleur('K'))); //on éteind la première case coté P1
    strip.show();
    rang_P1++;                                        //on passe à la deuxième pour P1
    choix_P1 = 'Z';                   //on remet son choix sur "Z" pour pas que sa réponse soit interprétée pour le cran suivant
    relance = true;
    if (rang_P1 == ((NUM_LEDS / 2)-1)) {      //Si P1 arrive au milieu il a gagné
      colorWipe((codeCouleur('W')), 10, 1);
      colorWipe((codeCouleur('K')), 10, 1);      
      victoireManche_P1 = true;                       //On déclare la victoire de manche pour P1
      relance = false;                                //on cesse la boucle de lecture des boutons
    }
  }
  if ((choix_P1 != (tabLeds[rang_P1])) && (choix_P1 != 'Z')) {  //si le choix de P1 est mauvais (et n'est pas encore sur "Z")
    sonBlop(1);
    rang_P1 --;                                                     //On rétrograde P1 d'un cran
    strip.setPixelColor(rang_P1, (codeCouleur(tabLeds[rang_P1])));  //On réaffiche la couleur du cran d'avant
    strip.show();
    choix_P1 = 'Z';                   //on remet son choix sur "Z" pour pas que sa réponse soit interprétée pour le cran d'avant
    if (rang_P1 < 0) {                //Si il est passé en dessous de zéro il reste à zéro
      rang_P1 = 0;
    }
  }

  //on traite la réponse de P2 :
  if (choix_P2 == (tabLeds[(NUM_LEDS - (rang_P2+1))])) {
    sonBlip(2);
    strip.setPixelColor((NUM_LEDS - (rang_P2+1)), (codeCouleur('K'))); //on éteind la première case coté P2
    strip.show();
    rang_P2++;                                        //on passe à la deuxième pour P1
    choix_P2 = 'Z';                   //on remet son choix sur "Z" pour pas que sa réponse soit interprétée pour le cran suivant
    relance = true;
    if (rang_P2 == ((NUM_LEDS / 2)-1)) {          //Si P2 arrive au milieu il a gagné
      colorWipe((codeCouleur('W')), 10, 2);
      colorWipe((codeCouleur('K')), 10, 2);
      victoireManche_P2 = true;                       //On déclare la victoire de manche pour P2
      relance = false;                                //on cesse la boucle de lecture des boutons

    }
  }
  if ((choix_P2 != (tabLeds[(NUM_LEDS - (rang_P2+1))])) && (choix_P2 != 'Z')) {  //si le choix de P2 est mauvais (et n'est pas encore sur "Z")
    sonBlop(2);
    rang_P2 --;                                                     //On rétrograde P2 d'un cran  
    strip.setPixelColor((NUM_LEDS - (rang_P2+1)), (codeCouleur(tabLeds[rang_P2])));
    strip.show();
    choix_P2 = 'Z';                   //on remet son choix sur "Z" pour pas que sa réponse soit interprétée pour le cran d'avant
    if (rang_P2 < 0) {                //Si il est passé en dessous de zéro il reste à zéro
      rang_P2 = 0;
    }
  }

}



void remplitBandeSerie() {
  for (uint16_t i = 0; i < ((strip.numPixels() / 2) - 1); i++) { //Pour la moitié des pixels -1
    randDelay = random(1, 20);                       //on choisi un temps de 'rotation' de chaque led au pif
    for (int j = 0; j <= randDelay; j++) {            //on fait tourner une couleur aléatoire pour chaque led pendants ce temps
      randNumber = random(1, 5);
      if (randNumber == 1) {
        currentColor = 'R';
      }
      if (randNumber == 2) {
        currentColor = 'G';
      }
      if (randNumber == 3) {
        currentColor = 'B';
      }
      if (randNumber == 4) {
        currentColor = 'Y';
      }
      strip.setPixelColor(i, (codeCouleur(currentColor)));  //on affiche cette couleur sur le pixel courant par la gauche
      strip.setPixelColor(strip.numPixels() - (i+1), (codeCouleur(currentColor))); //et par la droite
      strip.show();
      buzz.play((240 * randNumber) + (100 * i), 10);
      delay(10);
    }
    tabLeds[i] = currentColor;                              //on l'enregistre dans un tableau récapitulatif par la gauche
    tabLeds[strip.numPixels() - (i+1)] = currentColor;          //et par la droite
    tabLeds[i + 1] = 'K';                            //et par sécu on efface la case suivante avec du noir par la gauche
    tabLeds[strip.numPixels() - (i + 2)] = 'K';      //et par la droite

  }
}


void remplitBandeCompte() {
  //l'enjeu est de remplir la bande de couleurs aléatoires en n'ayant qu'une seule couleur dominante
  //(pour n'avoir qu'une seule couleur gagnante)
  relance = false;
  nbR = 0;
  nbG = 0;
  nbB = 0;
  nbY = 0;

  for (uint16_t i = 0; i < strip.numPixels(); i++) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
    randDelay = random(10, 20);                       //on choisi un temps de 'rotation' de chaque led au pif
    for (int j = 0; j <= randDelay; j++) {            //on fait tourner une couleur aléatoire pour chaque led pendants ce temps
      randNumber = random(1, 5);                      //on choisis une couleur au pif
      if (randNumber == 1) {
        currentColor = 'R';
      }
      if (randNumber == 2) {
        currentColor = 'G';
      }
      if (randNumber == 3) {
        currentColor = 'B';
      }
      if (randNumber == 4) {
        currentColor = 'Y';
      }
      strip.setPixelColor(i, (codeCouleur(currentColor)));  //on affiche cette couleur sur le pixel courant
      strip.show();
      buzz.play((240 * randNumber) + (100 * i), 10);
    }                                                       //fin de la boucle la couleur est figée
    tabLeds[i] = currentColor;                              //on l'enregistre dans un tableau récapitulatif
    if (currentColor == 'R') {
      nbR ++; //puis on teste quelle couleur c'est pour incrémenter le total de chacune
    }
    if (currentColor == 'G') {
      nbG ++;
    }
    if (currentColor == 'B') {
      nbB ++;
    }
    if (currentColor == 'Y') {
      nbY ++;
    }
  }
  //on calcule maintenant la cible finale
  Serial.print("   total Rouges = "); Serial.println(nbR);
  Serial.print("   total Verts  = "); Serial.println(nbG);
  Serial.print("   total Bleus  = "); Serial.println(nbB);
  Serial.print("   total Jaunes = "); Serial.println(nbY);
  int totalMax = nbR;   //pour ce faire on assigne tout d'abord le nombre de rouges au totalMax
  cible = 'R';          //et la cible
  if (nbG == totalMax) {
    relance = true; //si le nombre de verts est le même, on se dit qu'il faudra peut être relancer
  }
  if (nbG > totalMax) {
    totalMax = nbG;  //si il y a plus de verts, alors ça devient lui le max et la cible
    cible = 'G';
    relance = false;
  }
  if (nbB == totalMax) {
    relance = true; //et ainsi de suite
  }
  if (nbB > totalMax) {
    totalMax = nbB;
    cible = 'B';
    relance = false;
  }
  if (nbY == totalMax) {
    relance = true;
  }
  if (nbY > totalMax) {
    totalMax = nbY;
    cible = 'Y';
    relance = false;
  }
  //on sort de cette série de tests avec une valeur max, mais surtout l'info de devoir relancer si elle correspond à plusieurs couleurs
}

void ReactionBoutonsCompte() {

  Serial.print("Choix P1 = ");
  Serial.print(choix_P1);
  Serial.print(" Temps = ");
  Serial.print(temps_P1);

  Serial.print("  ---  Choix P2 = ");
  Serial.print(choix_P2);
  Serial.print(" Temps = ");
  Serial.println(temps_P2);


  //On compare le temps de P1 et de P2

  //Si P1 et P2 sont exaequos :
  if (temps_P1 == temps_P2) {
    if ((choix_P1 == choix_P2) && (choix_P1 == cible)) { //si les deux ont juste :
      Serial.println("les deux gagnent");
      laisseCouleurSel(choix_P1, 20, 1);
      laserBi('K', 'K', 1, 0);
      afficheCouleurSel(cible, 1, 0);
      laserBi ('K', 'K', 1, 0);
      afficheCouleurSel(cible, 1, 0);

      sonWow();       //on fait un son remarquable
      P1_score++;     //chacun marque un point
      P2_score++;
      delay(300);
    }
    if ((choix_P1 != choix_P2) && (choix_P1 == cible)) { //si seulement le joueur 1 a juste
      Serial.println("Joueur 1 gagne");
      laisseCouleurSel(choix_P1, 20, 1);
      laserBi ('K', 'K', 10, 1);
      sonWow();
      P1_score += 2;  //P1 marque deux point
    }
    if ((choix_P1 != choix_P2) && (choix_P2 == cible)) { //si seulement le joueur 2 a juste
      Serial.println("Joueur 2 gagne");
      laisseCouleurSel(choix_P2, 20, 2);
      laserBi ('K', 'K', 10, 2);
      sonWow();
      P2_score += 2;  //P2 marque deux point
    }
    if ((choix_P1 != cible) && (choix_P2 != cible)) { //si les deux ont tord
      Serial.println("les deux perdent");
      sonErreur();
      laisseCouleurSel(cible, 10, 0);
      laserBi ('K', 'K', 10, 0);
      delay(300);
    }
  }

  //Si P1 est le plus rapide :
  if (temps_P1 < temps_P2) {
    //Serial.println(" P1 plus rapide ");
    //On efface tout sauf la couleur choisie
    laisseCouleurSel(choix_P1, 20, 1);
    delay(200);
    colorWipe((codeCouleur('K')), 10, 1);
    //Si c'était une erreur :
    if (choix_P1 != cible) {
      sonErreur();
      //afficheTabCouleurs(); //on ré-affiche toutes les couleurs
      //delay(200);
      if (choix_P2 == cible) { //du coup on regarde si le P2 avait bon
        afficheCouleurSel(choix_P2, 20, 2);
        sonJuste();
        P2_score++;                                   //P2 marque un point
        P1_score--;                                   //P1 perds un point
      }
      if (choix_P2 != cible) {
        afficheCouleurSel(cible, 20, 0);
      }
      //laisseCouleurSel(cible, 0, 2);
      delay(400);
      colorWipe((codeCouleur('K')), 10, 2);
    }

    //Si c'était bien la cible :
    if (choix_P1 == cible) {
      P1_score += 2;  //P1 marque un point
      sonJuste();
    }

  }

  //Si P2 est le plus rapide :
  if (temps_P1 > temps_P2) {
    //Serial.println(" P2 plus rapide ");
    //On efface tout sauf la couleur choisie
    laisseCouleurSel(choix_P2, 20, 2);
    delay(200);
    colorWipe((codeCouleur('K')), 10, 2);       //Qu'on efface ensuite
    //Si c'était une erreur :
    if (choix_P2 != cible) {
      sonErreur();
      //afficheTabCouleurs(); //on ré-affiche toutes les couleurs
      //delay(200);
      if (choix_P1 == cible) { //du coup on regarde si le P1 avait bon
        afficheCouleurSel(choix_P1, 20, 1);
        sonJuste();
        P1_score++;                                   //P1 marque un point
        P2_score--;                                   //P2 perds un point
      }
      if (choix_P1 != cible) {
        afficheCouleurSel(cible, 20, 0);
      }
      //laisseCouleurSel(cible, 0, 1);
      delay(400);
      colorWipe((codeCouleur('K')), 10, 1);
    }

    //Si c'était bien la cible :
    if (choix_P2 == cible) {
      P2_score += 2;  //P2 marque un point
      sonJuste();
    }


  }

  // Apres ce calcul de score on s'assure que personne n'es tombé en négatif :
  if (P1_score <= 0) {
    P1_score = 0; //si le score de P1 est devenu négatif on le remet à 0
  }
  if (P2_score <= 0) {
    P2_score = 0; //si le score de P2 est devenu négatif on le remet à 0
  }

  //Puis on remet le choix des joueurs sur une valeur bidon "Z" :
  choix_P1 = 'Z';
  choix_P2 = 'Z';
}


void lobby() { //à ecrire

  score.clear();
  score.print("CHOISIR MODE  ");

  //LectureBoutons(1);
  LectureBoutons();
}


void AfficheScore() {
  //On affiche le tableau des scores et au passage c'est ici qu'on détecte si il y a un gagnant
  score.printDualCounter(P1_score, P2_score);
  if ((P2_score >= scoreWin) && (P1_score >= scoreWin)) {
    victoire(0, P1_score);
    recommencer(0);
  }
  if ((P1_score >= scoreWin) && (P1_score > P2_score)) {
    victoire(1, P1_score);
    recommencer(2);
  }
  if ((P2_score >= scoreWin) && (P2_score > P1_score)) {
    victoire(2, P2_score);
    recommencer(1);
  }

}

void CalculCible() {
  /*On choisis une couleur au pif en faisant un tres court son au pif
     et on l'affiche sur les deux pixels du millieu

    (Bon ok cette procedure est très moche... mais ça fait ce que ça doit en attendant de la re-écrire)
  */
  randNumber = random(1, 5);
  //randNumber= 2;
  if (randNumber == 1) {
    cible = 'R';
    strip.setPixelColor(7, 0, 255, 0);
    strip.setPixelColor(8, 0, 255, 0);
  }

  if (randNumber == 2) {
    cible = 'G';
    strip.setPixelColor(7, 255, 0, 0);
    strip.setPixelColor(8, 255, 0, 0);
  }

  if (randNumber == 3) {
    cible = 'B';
    strip.setPixelColor(7, 0, 0, 255);
    strip.setPixelColor(8, 0, 0, 255);
  }

  if (randNumber == 4) {
    cible = 'Y';
    strip.setPixelColor(7, 255, 255, 0);
    strip.setPixelColor(8, 255, 255, 0);
  }

  buzz.play(240 * randNumber, 10);
  strip.show();
  delay(50);

}

void LectureBoutons() {
  //On lit les valeurs en entrée analogique pour P1 et P2
  /*Notre systeme est un pont diviseur où différentes resistances se cumulent selon quel bouton est appuyé
     renvoyant une valeur différente sur l'entrée analogique
     pour simplifier l'approvisionnement en composants j'ai choisi des resistances identiques
     de 1K en cumul et de 6,8K en "tirage"
     Il en résulte des valeurs un peu arbitraires et mal réparties sur la plage mais suffisament distinctes
     pour que ça marche...
  */



  etat_P1 = analogRead(P1_Bout);
  //Serial.println(etat_P1);

  etat_P2 = analogRead(P2_Bout);
  //Serial.println(etat_P2);
  if (etat_P1 > 850){delay(5);coolDown_P1=true;} //on ne "refroidit" le bouton que si il est relaché (avec un micro delay contre les rebonds)
  if (etat_P2 > 850){delay(5);coolDown_P2=true;}
  
  //Si la valeur de P1 est passée sous 850 (avec les résistances utilisées dans notre pont diviseur)
  //Et que le bouton a "refroidit"
  //c'est qu'un bouton de P1 a été appuyé :
  if ((etat_P1 < 850)&&(coolDown_P1)) {
    //On enregistre alors le temps du joueur P1
    //Serial.println("Appui P1");
    //temps_P1 = millis();
    if (choix_P1 == 'Z') {
      temps_P1 = millis();
    }

    //les fourchettes sont à adapter pour caque bouton selon les résistances choisies dans le pont diviseur
    if ((etat_P1 < 350) && (etat_P1 > 280)) { //la valeur tourne autour de 310
      choix_P1 = 'R';
      coolDown_P1 = false;
      // Serial.print(" ROUGE = ");
      // Serial.println(etat_P1);
    }
    if ((etat_P1 < 160) && (etat_P1 > 110)) { //la valeur tourne autoure de 130
      choix_P1 = 'G';
      coolDown_P1 = false;
      // Serial.print(" VERT = ");
      //Serial.println(etat_P1);
    }
    if (etat_P1 < 100)  { //valeur à zéro
      choix_P1 = 'B';
      coolDown_P1 = false;
      //Serial.print(" BLEU = ");
      // Serial.println(etat_P1);
    }
    if ((etat_P1 < 260) && (etat_P1 > 180)) { //valeur autour de 230
      choix_P1 = 'Y';
      coolDown_P1 = false;
      //Serial.print(" JAUNE = ");
      //Serial.println(etat_P1);
    }
  }
 

  if (nbJoueurs == 2) { //si on a deux joueurs
    //Si la valeur de P2 est passée sous 850 (avec les résistances utilisées dans notre pont diviseur)
    //c'est qu'un bouton de P2 a été appuyé :
    if ((etat_P2 < 850)&&(coolDown_P2)) {
      //On enregistre alors le temps du joueur P2
      if (choix_P2 == 'Z') {
        temps_P2 = millis();
      }

      //Serial.println("       Appui P2");
      //les fourchettes sont à adapter pour caque bouton selon les résistances choisies dans le pont diviseur
      if ((etat_P2 < 350) && (etat_P2 > 280)) {
        choix_P2 = 'R';
        coolDown_P2 = false;
        // Serial.print(" ROUGE = "); Serial.println(etat_P2);
      }
      if ((etat_P2 < 160) && (etat_P2 > 110)) {
        choix_P2 = 'G';
        coolDown_P2 = false;
        //Serial.print(" VERT = "); Serial.println(etat_P2);
      }
      if (etat_P2 < 100) {
        choix_P2 = 'B';
        coolDown_P2 = false;
        //Serial.print(" BLEU = "); Serial.println(etat_P2);
      }
      if ((etat_P2 < 260) && (etat_P2 > 180)) {
        choix_P2 = 'Y';
        coolDown_P2 = false;
        //Serial.print(" JAUNE = "); Serial.println(etat_P2);
      }
    }
  }

  if (nbJoueurs == 1) {
    //Joueur unique, on s'occupe du choix de l'ordi en tant que P2 (en fonction de son taux de réussite)

    randNumber = random(1, 10);           //l'ordi choisis un nombre entre 1 et 10
    if (randNumber >= tauxReussiteOrdi) { //si on depasse son taux de succes
      if (cible != 'R') {
        choix_P2 = 'R'; //il choisi R si ce n'est pas égal à la cible (et donc se trompe)
      }
      if (cible = 'R') {
        choix_P2 = 'B';
      } //à moins que la cible soit R auquel cas il choisis B (et donc se trompe)
    }

    else {
      choix_P2 = cible; //autrement c'est qu'il doit réussir et choisis donc la bonne cible
    }


    //Enfin on s'occupe du temps de réaction de l'ordi en tant que P2 (en fonction du niveau de difficulté)

    //temps_P2=(millis()+random(100, 600)); //assez dur niveau 4
    //temps_P2=(millis()+random(200, 700)); //un peu dur mais ok adulte niveau 3
    //temps_P2 = (millis() + random(500, 1000)); //moyen gaetan niveau 2
    //temps_P2=(millis()+random(500, 1400)); //facile gaetan Niveau 1

    temps_P2 = (millis() + random(1500, 2000));

    //
  }
}

void ReactionBoutonsCible() {

  //temps_P1 = 1000;
  //temps_P2 = 1000;
  Serial.print("Choix P1 = ");
  Serial.print(choix_P1);
  Serial.print(" Temps = ");
  Serial.print(temps_P1);

  Serial.print("  ---  Choix P2 = ");
  Serial.print(choix_P2);
  Serial.print(" Temps = ");
  Serial.println(temps_P2);


  //On compare le temps de P1 et de P2

  //Si P1 et P2 sont exaequos :
  if (temps_P1 == temps_P2) {
    if ((choix_P1 == choix_P2) && (choix_P1 == cible)) { //si les deux ont juste :
      Serial.println("les deux gagnent");
      laserBi ('W', 'W', 10, 0);   //on tire un laser bilateral blanc
      laserBi ('K', 'K', 10, 0);
      sonWow();       //on fait un son remarquable
      P1_score++;     //chacun marque un point
      P2_score++;
    }
    if ((choix_P1 != choix_P2) && (choix_P1 == cible)) { //si seulement le joueur 1 a juste
      Serial.println("Joueur 1 gagne");
      laserBi (choix_P1, choix_P2, 10, 1);
      laserBi ('K', 'K', 10, 1);
      sonWow();
      P1_score += 2;  //P1 marque deux point
    }
    if ((choix_P1 != choix_P2) && (choix_P2 == cible)) { //si seulement le joueur 2 a juste
      Serial.println("Joueur 2 gagne");
      laserBi (choix_P1, choix_P2, 10, 2);
      laserBi ('K', 'K', 10, 2);
      sonWow();
      P2_score += 2;  //P2 marque deux point
    }
    if ((choix_P1 != cible) && (choix_P2 != cible)) { //si les deux ont tord
      Serial.println("les deux perdent");
      laserBi (choix_P1, choix_P2, 10, 0);
      laserBi ('K', 'K', 10, 0);
      sonErreur(); sonErreur();
    }
  }

  //Si P1 est le plus rapide :
  if (temps_P1 < temps_P2) {
    Serial.println(" P1 plus rapide ");
    //On tire un laser de la couleur choisie
    colorWipe((codeCouleur(choix_P1)), 10, 1);
    colorWipe((codeCouleur('K')), 10, 1); //Puis on efface la bande
    //Si c'était une erreur :
    if (choix_P1 != cible) {
      sonErreur();
      if (choix_P2 == cible) { //du coup on regarde si le P2 avait bon
        colorWipe((codeCouleur('W')), 10, 2); //Il lire un laser Blanc de "contre-attaque"
        colorWipe((codeCouleur('K')), 10, 2);       //Qu'on efface ensuite
        sonJuste();
        P2_score++;                                   //P2 marque un point
        P1_score--;                                   //P1 perds un point
      }
    }

    //Si c'était bien la cible :
    if (choix_P1 == cible) {
      P1_score += 2;  //P1 marque un point
      sonJuste();
    }

  }

  //Si P2 est le plus rapide :
  if (temps_P1 > temps_P2) {
    Serial.println(" P2 plus rapide ");
    //On tire un laser de la couleur choisie
    colorWipe((codeCouleur(choix_P2)), 10, 2);
    colorWipe((codeCouleur('K')), 10, 2);       //Qu'on efface ensuite

    //Si c'était une erreur :
    if (choix_P2 != cible) {
      sonErreur();
      if (choix_P1 == cible) { //du coup on regarde si le P1 avait bon
        colorWipe((codeCouleur('W')), 10, 1); //Il lire un laser Blanc de "contre-attaque"
        colorWipe((codeCouleur('K')), 10, 1);       //Qu'on efface ensuite
        sonJuste();
        P1_score++;                                   //P1 marque un point
        P2_score--;                                   //P2 perds un point
      }
    }

    //Si c'était bien la cible :
    if (choix_P2 == cible) {
      P2_score += 2;  //P2 marque un point
      sonJuste();
    }


  }

  // Apres ce calcul de score on s'assure que personne n'es tombé en négatif :
  if (P1_score <= 0) {
    P1_score = 0; //si le score de P1 est devenu négatif on le remet à 0
  }
  if (P2_score <= 0) {
    P2_score = 0; //si le score de P2 est devenu négatif on le remet à 0
  }

  //Puis on remet le choix des joueurs sur une valeur bidon "Z" :
  choix_P1 = 'Z';
  choix_P2 = 'Z';
}

void victoire(int joueur, int scoreJoueur) {
  //score.clear();
  rainbow(1);
  delay(300);
  buzz.play(523, 100);
  delay(100);
  buzz.play(659, 100);
  delay(100);
  buzz.play(784, 100);
  delay(100);
  buzz.play(1046, 100);
  delay(300);
  buzz.play(784, 100);
  delay(100);
  buzz.play(1046, 300);
  delay(300);
  score.blink();
  score.blink();
  rainbow(1);
  score.clear();
  if (joueur == 0) {
    //score.print (scoreJoueur);
    rainbow(1); rainbow(1); rainbow(1);
    score.print("    EGALITE    ");
  }
  if (joueur == 1) {
    //score.print (scoreJoueur);
    rainbowCycle(0, 2, 2);
    score.print("    JOUEUR 1 GAGNE    ");
  }
  // score.clear();
  if (joueur == 2) {
    //score.setCursor(1,2);
    //score.print (scoreJoueur);
    rainbowCycle(0, 2, 1);
    // score.blink();
    score.print("    JOUEUR 2 GAGNE    ");
  }

  score.clear();

}

void recommencer(int sens) {
  /*On remet les scores à zéro et on efface le tableau et la bonde de NeoPixels
     dans la direction du gagnant
  */
  P1_score = 0;
  P2_score = 0;
  strip.begin();
  colorWipe(strip.Color(0, 0, 0), 10, sens);
  strip.show();
  score.printDualCounter(P1_score, P2_score);
  partieEnCours = false;
}

void sonGenerique() {
  delay(300);
  buzz.play(1318, 100);
  delay(100);
  buzz.play(1760, 100);
  delay(100);
  buzz.play(1318, 100);
  delay(100);
  buzz.play(1760, 100);
  delay(100);
  buzz.play(1318, 700);
  delay(1500);
  buzz.play(1046, 500);
  delay(500);
  buzz.play(1175, 500);
  delay(500);
  buzz.play(880, 700);
  delay(1000);
}

void sonBlop(int joueur) {
  buzz.play(400*joueur, 100);
  delay(20);
  buzz.play(200*joueur, 100);
  delay(20);
  buzz.play(100*joueur, 100);
  delay(50);
}

void sonBlip(int joueur) {
  buzz.play(500*joueur, 100);
  delay(20);
  buzz.play(1000*joueur, 100);
  delay(20);
  buzz.play(1600*joueur, 100);
  delay(50);
}

void sonErreur() {
  buzz.play(400, 100);
  delay(100);
  buzz.play(200, 100);
  delay(100);
  buzz.play(100, 100);
  delay(200);
}

void sonJuste() {
  buzz.play(500, 100);
  delay(100);
  buzz.play(1000, 100);
  delay(100);
  buzz.play(1600, 100);
  delay(200);
}

void sonWow() {
  int j = 0;
  delay(30);
  for (int i = 523; i <= 2046; i += 100) {
    buzz.play(i, 10);
    delay(5);
  }

  delay(100);
  //buzz.play(1200,70);
  //delay(100);
  for (int i = 784; i <= 1932; i += 100) {
    buzz.play(i, 10);
    delay(5);
  }
  for (int i = 1932; i >= 1100; i -= 50) {
    buzz.play(i, 20);
    //buzz.play((1200),100);
    delay((5 + j));
    j++;
  }
  delay(500);
}


uint32_t codeCouleur(char c) {
  if (c == 'R') {
    return (strip.Color(0, 255, 0));
  }
  if (c == 'G') {
    return (strip.Color(255, 0, 0));
  }
  if (c == 'B') {
    return (strip.Color(0, 0, 255));
  }
  if (c == 'Y') {
    return (strip.Color(255, 255, 0));
  }
  if (c == 'K') {
    return (strip.Color(0, 0, 0));
  }
  if (c == 'W') {
    return (strip.Color(100, 100, 100));
  }
}

void blinkBande(int blinks, int wait) {   //Pour faire clignotter la bande de Neopixels
  int tabLedsActuel[strip.numPixels()];   //on enregistre dans un tableau l'etat actuel de la bande Neopixels
  for (uint16_t i = 0;  i < strip.numPixels(); i++) {
    tabLedsActuel[i] = strip.getPixelColor(i);
  }

  for (uint16_t j = 0; j < blinks; j++) {
    for (uint16_t i = 0;  i < strip.numPixels(); i++) { //on mets tous les pixels au noir
      strip.setPixelColor(i, codeCouleur('K'));
    }
    strip.show();                                       //on affiche le noir
    delay(wait);                                        //on attend le temps choisi
    for (uint16_t i = 0;  i < strip.numPixels(); i++) { //on remet tous les pixels dans l'état enregistré
      strip.setPixelColor(i, tabLedsActuel[i]);
    }
    strip.show();                                       //on affiche les couleurs
    delay(wait);                                        //on attend le temps choisi
  }
}


void afficheTabCouleurs() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
    strip.setPixelColor(i, (codeCouleur(tabLeds[i])));
    strip.show();
  }
}

void effaceCouleurSel(char coulToDel) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
    if (tabLeds[i] = coulToDel) {
      strip.setPixelColor(i, (codeCouleur('K')));
      strip.show();
      buzz.play (240, 10);
      delay(100);
    }
  }
}

void afficheCouleurSel(char coulToShow, uint8_t wait, int sens) {
  if (sens == 0) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
      if (tabLeds[i] == coulToShow) {
        strip.setPixelColor(i, (codeCouleur(coulToShow)));
        //buzz.play (240, 10);
        delay(wait);
      }
    }
    strip.show();
  }
  if (sens == 1) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
      if (tabLeds[i] == coulToShow) {
        strip.setPixelColor(i, (codeCouleur(coulToShow)));
        strip.show();
        buzz.play (240, 10);
        delay(wait);
      }
    }
  }

  if (sens == 2) {
    for (uint16_t i = strip.numPixels(); i > 0; i--) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
      if (tabLeds[i - 1] == coulToShow) {
        strip.setPixelColor(i - 1, codeCouleur(coulToShow));
        strip.show();
        buzz.play (240, 10);
        delay(wait);
      }
    }
  }
}

void laisseCouleurSel(char coulToLeave, uint8_t wait, int sens) {
  if (sens == 1) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
      if (tabLeds[i] != coulToLeave) {
        strip.setPixelColor(i, (codeCouleur('K')));
        strip.show();
        buzz.play (240, 10);
        delay(wait);
      }
    }
  }

  if (sens == 2) {
    for (uint16_t i = strip.numPixels(); i > 0; i--) {  //on fait croitre la variable i de 0 au nombre de pixels - 1 pour parcourir chaque led
      if (tabLeds[i - 1] != coulToLeave) {
        strip.setPixelColor(i - 1, codeCouleur('K'));
        strip.show();
        buzz.play (240, 10);
        delay(wait);
      }
    }
  }
}

void laserBi(char cP1, char cP2, uint8_t wait, int sens) {
  /*Permet de remplir un à un les pixels de couleur "c" à la vitesse "wait" dans la direction "sens"
     permet du coup également d'effacer les pixel en les remplissant de noir
     Pour avoir un son de laser en même temps que son affichage on est contraint de gérer le son ici
     et de le produire en même temps qu'on remplit les pixels (sauf quand il s'agit d'un effacement)
  */

  if (sens == 0) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) { //on fait décroitre la variable i du nombre de pixels à 1
      strip.setPixelColor(i - 1, (codeCouleur(cP1)));                //on assigne la couleur au pixel i-1 (car l'index commence à 0)
      strip.setPixelColor((strip.numPixels() - i), (codeCouleur(cP2)));
      strip.show();                                 //on affiche les pixels

      if (cP1 != 'K') {                         //si on n'est pas en train d'effacer les pixels (assigner du noir)
        buzz.play(300 + (i * 100), 10);
        delay(10);
        buzz.play(1900 - (i * 100), 10);
      }
      delay(wait);                                  //on attend un délai qui définit la vitesse du laser
    }
  }


  if (sens == 1) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) { //on fait décroitre la variable i du nombre de pixels à 1
      strip.setPixelColor(i - 1, (codeCouleur(cP1)));                //on assigne la couleur au pixel i-1 (car l'index commence à 0)
      if (i < 8) {
        strip.setPixelColor((strip.numPixels() - i), (codeCouleur(cP2)));
      }
      strip.show();                                 //on affiche les pixels

      if (cP1 != 'K') {                         //si on n'est pas en train d'effacer les pixels (assigner du noir)
        buzz.play(300 + (i * 100), 10);
        delay(10);
        buzz.play(1900 - (i * 100), 10);
      }
      delay(wait);                                  //on attend un délai qui définit la vitesse du laser
    }
  }

  if (sens == 2) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) { //on fait décroitre la variable i du nombre de pixels à 1
      if (i < 8) {
        strip.setPixelColor(i - 1, (codeCouleur(cP1)));                //on assigne la couleur au pixel i-1 (car l'index commence à 0)
      }
      strip.setPixelColor((strip.numPixels() - i), (codeCouleur(cP2)));

      strip.show();                                 //on affiche les pixels

      if (cP1 != 'K') {                         //si on n'est pas en train d'effacer les pixels (assigner du noir)
        buzz.play(300 + (i * 100), 10);
        delay(10);
        buzz.play(1900 - (i * 100), 10);
      }
      delay(wait);                                  //on attend un délai qui définit la vitesse du laser
    }
  }
}


void colorWipe(uint32_t c, uint8_t wait, int sens) {
  /*Permet de remplir un à un les pixels de couleur "c" à la vitesse "wait" dans la direction "sens"
     permet du coup également d'effacer les pixel en les remplissant de noir
     Pour avoir un son de laser en même temps que son affichage on est contraint de gérer le son ici
     et de le produire en même temps qu'on remplit les pixels (sauf quand il s'agit d'un effacement)
  */
  if (sens == 0) {
    laserBi('K', 'K', 10, 0);
  }
  if (sens == 2) {                                      //Si le sens est égal à 1
    for (uint16_t i = strip.numPixels(); i > 0; i--) { //on fait décroitre la variable i du nombre de pixels à 1
      strip.setPixelColor(i - 1, c);                //on assigne la couleur au pixel i-1 (car l'index commence à 0)
      strip.show();                                 //on affiche les pixels

      if (c != (0, 0, 0)) {                         //si on n'est pas en train d'effacer les pixels (assigner du noir)
        buzz.play(300 + (i * 100), 20);               //on fait un bruit qui décroit en fréquence
      }
      delay(wait);                                  //on attend un délai qui définit la vitesse du laser
    }
  }
  if (sens == 1) {                                     //Si le sens est égal à 0
    for (uint16_t i = 0; i < strip.numPixels(); i++) { //on fait croitre la variable i de 0 au nombre de pixels
      strip.setPixelColor(i, c);                    //on assigne la couleur au pixel i
      strip.show();                                 //on affiche les pixels

      if (c != (0, 0, 0)) {                         //si on n'est pas en train d'effacer les pixels (assigner du noir)
        buzz.play(300 + (i * 100), 20);               //on fait un bruit qui croit en fréquence
      }
      delay(wait);                                  //on attend un délai qui définit la vitesse du laser
    }
  }

}


//Tableau de correction gamma pour le NeoPixel
byte neopix_gamma[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};


//Différents effets possibles avec le NeoPixel :
void pulseWhite(uint8_t wait) {
  for (int j = 0; j < 256 ; j++) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
    }
    delay(wait);
    strip.show();
  }

  for (int j = 255; j >= 0 ; j--) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
    }
    delay(wait);
    strip.show();
  }
}

void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = 0;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for (int k = 0 ; k < rainbowLoops ; k ++) {

    for (int j = 0; j < 256; j++) { // 5 cycles of all colors on wheel

      for (int i = 0; i < strip.numPixels(); i++) {

        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);

        redVal = red(wheelVal) * float(fadeVal / fadeMax);
        greenVal = green(wheelVal) * float(fadeVal / fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal / fadeMax);

        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );

      }

      //First loop, fade in!
      if (k == 0 && fadeVal < fadeMax - 1) {
        fadeVal++;
      }

      //Last loop, fade out!
      else if (k == rainbowLoops - 1 && j > 255 - fadeMax ) {
        fadeVal--;
      }

      strip.show();
      delay(wait);
    }

  }



  delay(500);


  for (int k = 0 ; k < whiteLoops ; k ++) {

    for (int j = 0; j < 256 ; j++) {

      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
      }
      strip.show();
    }

    delay(2000);
    for (int j = 255; j >= 0 ; j--) {

      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
      }
      strip.show();
    }
  }

  delay(500);


}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) {

  if (whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int head = whiteLength - 1;
  int tail = 0;

  int loops = 3;
  int loopNum = 0;

  static unsigned long lastTime = 0;


  while (true) {
    for (int j = 0; j < 256; j++) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if ((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ) {
          strip.setPixelColor(i, strip.Color(0, 0, 0, 255 ) );
        }
        else {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }

      }

      if (millis() - lastTime > whiteSpeed) {
        head++;
        tail++;
        if (head == strip.numPixels()) {
          loopNum++;
        }
        lastTime = millis();
      }

      if (loopNum == loops) return;

      head %= strip.numPixels();
      tail %= strip.numPixels();
      strip.show();
      delay(wait);
    }
  }

}

void fullWhite() {

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 255 ) );
  }
  strip.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, uint8_t loops, int sens) {
  uint16_t i, j;
  if (sens == 1) {
    for (j = 256 * loops; j > 0; j--) {
      for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      }
      strip.show();
      delay(wait);
    }
  }
  if (sens == 2) {
    for (j = 0; j < 256 * loops; j++) {
      for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      }
      strip.show();
      delay(wait);
    }

  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}





