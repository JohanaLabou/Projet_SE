#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#define ARTICLE "classique"
#define SERVEUR "sw1"
#define ACHETEUR "Philippe"
#define TRANSPORTEUR "Xavier"
#define QteRouleauStock 100
#define SURFACE 25
#define PrixRouleau 10
#define PALETTE 63
#define MSGSIZE 256

void acheteur(int pSEcriture[], int pSLecture[], int pTEcriture[], int pTLecture[]);
void serveur(int pAEcriture[], int pALecture[], int pTEcriture[]);
void transporteur(int pAEcriture[], int pALecture[], int pSLecture[]);
char* concat(const char *s1, const char *s2);
char* getMontantFacture();
char* getNombrePalettes();

char* msg1 = "Bonjour, je souhaiterais avoir l'article "; // Q1
char* msg2 = "Le nombre de rouleaux en stock disponible pour cet article est "; // Q2
char* msg3 = "Je souhaite passer une commande pour une surface de "; // Q3
char msg4[MSGSIZE] = "Commande recue. Nombre total de palettes: "; // Q4
char msg4Bis[MSGSIZE] = "Montant total de la facture: "; // Q4
char* msg6 = "J'accuse r\u00e9ception de votre paiment. Le montant de la transaction \u00e9tait de: "; // Q6
char* msg7 = "Voici un bon de livraison. Pour rappel, le nombre de palettes de l'article choisi est: "; // Q7
char* msg7Bis = "Voici un autre bon de livraison pour cette meme commande "; // Q7
char* msg8 = "Bonjour, je suis votre livreur. Voici les 2 bons de livraison"; // Q8
char* msg9 = "Merci. Je vous rends un bon sign\u00e9"; // Q9

int main(int argc, char **argv){
    pid_t pidAcheteur;
    pid_t pidServeurWeb;
    pid_t pidTransporteur;

    int tabAcheteurServeur[2];
    int tabServeurAcheteur[2];
    int tabTransporteurAcheteur[2];
    int tabAcheteurTransporteur[2];
    int tabServeurTransporteur[2];

    int pipeAcheteurServeur = pipe(tabAcheteurServeur); //creation du pipe: c'est un tableau de 2 cases ou chaque case est un identifient fichier
    if (pipeAcheteurServeur < 0) {
        printf("ACHETEUR. Tube non support\u00e9. Fin\n");
        exit(1);   // si pipe() est executee avec succes alors la fonction renvoie 0, -1 sinon
    }
    printf("ACHETEUR. Activation du mode non bloquant.\n");
    if (fcntl(tabAcheteurServeur[0], F_SETFL, O_NONBLOCK) < 0) {
    /*  Fonction qui rend la lecture du pipe non bloquante (asynchrone)
        Dans ce cas read() dans un tube vide ne bloque pas et renvoie -1.
        Ce n'est pas considere comme fin de fichier, mais une erreur
    */
        printf("ACHETEUR. Mode non bloquant indisponible. Fin\n");
        exit(2);
    }

    int pipeServeurAcheteur = pipe(tabServeurAcheteur);
    if (pipeServeurAcheteur < 0){
        printf("SERVEUR. Tube non support\u00e9. Fin\n");
        exit(3);
    }
    printf("SERVEUR. Activation du mode non bloquant.\n");
    if (fcntl(tabServeurAcheteur[0], F_SETFL, O_NONBLOCK) < 0) {
        printf("SERVEUR. Mode non bloquant indisponible. Fin\n");
        exit(4);
    }

    int pipeTransporteurAcheteur = pipe(tabTransporteurAcheteur);
    if (pipeTransporteurAcheteur < 0){
        printf("TRANSPORTEUR. Tube non support\u00e9. Fin\n");
        exit(5);
    }
    printf("TRANSPORTEUR. Activation du mode non bloquant.\n");
    if (fcntl(tabTransporteurAcheteur[0], F_SETFL, O_NONBLOCK) < 0) {
        printf("TRANSPORTEUR. Mode non bloquant indisponible. Fin\n");
        exit(6);
    }

    int pipeAcheteurTransporteur = pipe(tabAcheteurTransporteur);
    if (pipeAcheteurTransporteur < 0){
        printf("ACHETEUR. Tube non support\u00e9. Fin\n");
        exit(7);
    }
    printf("ACHETEUR. Activation du mode non bloquant.\n");
    if (fcntl(tabAcheteurTransporteur[0], F_SETFL, O_NONBLOCK) < 0) {
        printf("ACHETEUR. Mode non bloquant indisponible. Fin\n");
        exit(8);
    }

    int pipeServeurTransporteur = pipe(tabServeurTransporteur);
    if (pipeServeurTransporteur < 0){
        printf("SERVEUR. Tube non support\u00e9. Fin\n");
        exit(9);
    }
    printf("SERVEUR. Activation du mode non bloquant.\n");
    if (fcntl(tabServeurTransporteur[0], F_SETFL, O_NONBLOCK) < 0) {
        printf("SERVEUR. Mode non bloquant indisponible. Fin\n");
        exit(10);
    }
    
    printf("\n------------------------------\n");
    printf("\nD\u00e9but des communications.\n");

    pidAcheteur = fork(); // creation du processus fils
    if (pidAcheteur < 0){ // on teste s'il y a eu un pb
        printf("ACHETEUR. Fork impossible.\n");
        exit(11);
    } else if (pidAcheteur == 0) { // on rentre dans le processus fils
        acheteur(tabAcheteurServeur, tabServeurAcheteur, tabAcheteurTransporteur, tabTransporteurAcheteur);

    } else {
        pidServeurWeb = fork();
        if (pidServeurWeb == -1){
            printf("SERVEUR. Fork impossible.\n");
            exit(12);
        } else if (pidServeurWeb == 0) {
            serveur(tabServeurAcheteur, tabAcheteurServeur, tabServeurTransporteur);

        } else {
            pidTransporteur = fork();
            if (pidTransporteur == -1){
                printf("TRANSPORTEUR. Fork impossible.\n");
                exit(13);
            } else if (pidTransporteur == 0){
                transporteur(tabTransporteurAcheteur, tabAcheteurTransporteur, tabServeurTransporteur);
            }
        }
    }
    return 0;
}


void acheteur(int pSEcriture[], int pSLecture[], int pTEcriture[], int pTLecture[]){
    close(pSEcriture[0]);
    close(pSLecture[1]);
    close(pTEcriture[0]);
    close(pTLecture[1]);

    int nread;
    char buf[MSGSIZE];
    char* str3Bis; // pour utiliser la fonction sprintf()

    write(pSEcriture[1], concat(msg1, ARTICLE), MSGSIZE); // Question 1: l'acheteur saisit le nom d'un article
    
    nread = read(pSLecture[0],buf, MSGSIZE); // on lit la Q2
    printf("[ACHETEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", ACHETEUR, SERVEUR, buf);

    sprintf(str3Bis, "%d", SURFACE); // on convertit la surface en string
    write(pSEcriture[1], concat(msg3, str3Bis), MSGSIZE); // Question 3: l'acheteur saisit la surface souhaitee

    nread = read(pSLecture[0],buf, MSGSIZE); // Q4: on lit le message du serveur
    printf("[ACHETEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", ACHETEUR, SERVEUR, buf);

    nread = read(pSLecture[0],buf, MSGSIZE); // Q4: on lit le message du serveur
    printf("[ACHETEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", ACHETEUR, SERVEUR, buf);

    // Question 5: l'acheteur paie en saisissant son numero de carte bancaire et son cryptogramme
    int crypto;
    printf("Veuillez saisir votre num\u00e9ro de carte bancaire (16 chiffres) suivi de votre cryptogramme (3 chiffres): ");
    fflush(stdout);
    scanf("[%d]", &crypto);
    write(pSEcriture[1], &crypto, MSGSIZE);

    nread = read(pSLecture[0],buf, MSGSIZE); // Q6: on lit le message du serveur
    printf("[ACHETEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", ACHETEUR, SERVEUR, buf);

    nread = read(pTLecture[0],buf, MSGSIZE); // Q8: on lit le msg du transporteur
    printf("[ACHETEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", ACHETEUR, SERVEUR, buf);

    write(pTEcriture[1], msg9, MSGSIZE); // Q99: l'acheteur signe un des bons qu'il remet au livreur
}


void serveur(int pAEcriture[], int pALecture[], int pTEcriture[]){
    close(pAEcriture[0]);
    close(pALecture[1]);
    close(pTEcriture[0]);

    int nread;
    char buf[MSGSIZE];
    
    // CA NE FONCTIONNE PAS

    while (1) { // on fait une boucle infinie
        nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q1
        switch (nread) { 
        case -1:
            if (errno == EAGAIN){ 
                printf("\nTube vide.\n");
                sleep(1);
                break; 
            } else {
                perror("read");
                exit(100);
            }
        case 0: // Si nread == 0, ca veut dire qu'on est arrivés à la fin du buffer
            close(pALecture[0]); // on ferme le pipe
            printf("[SERVEUR]. Fin.\n");
            exit(0); 
        default:    
            printf("\n[SERVEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", SERVEUR, ACHETEUR, buf);
            break;
        }
    }

    char* str2Bis; // pour utiliser la fonction sprintf() et convertir un int en str
    sprintf(str2Bis, "%d", QteRouleauStock);
    write(pAEcriture[1], concat(msg2, str2Bis), MSGSIZE); // Question 2: le serveur web transmet la quantite disponible en stock

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q3
    printf("[SERVEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", SERVEUR, ACHETEUR, buf);

    write(pAEcriture[1], concat(msg4, getNombrePalettes()), MSGSIZE); // Question 4: le serveur transmet le nombre de palettes a l'acheteur
    write(pAEcriture[1], concat(msg4Bis, getMontantFacture()), MSGSIZE); // Question 4: le serveur transmet le montant de la facture a l'acheteur

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q5
    printf("[SERVEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", SERVEUR, ACHETEUR, buf);

    write(pAEcriture[1], concat(msg6, getMontantFacture()), MSGSIZE); // Question 6: le serveur web envoie un accuse de reception du paiement a l'acheteur, rappelant le montant total de la transaction

    write(pTEcriture[1], concat(msg7, getNombrePalettes()), MSGSIZE); // Question 7: le serveur web transmet un bon de livraison comprenant le nombre de palettes de l'article choisi
    write(pTEcriture[1], msg7Bis, MSGSIZE); // Q7: en double exemplaire au transporteur
}

void transporteur(int pAEcriture[], int pALecture[], int pSLecture[]){
    close(pAEcriture[0]);
    close(pALecture[1]);
    close(pSLecture[1]);

    int nread;
    char buf[MSGSIZE];
    
    nread = read(pSLecture[0], buf, MSGSIZE); // On lit la Q7
    printf("[TRANSPORTEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", TRANSPORTEUR, SERVEUR, buf);

    nread = read(pSLecture[0], buf, MSGSIZE); // On lit la Q7
    printf("[TRANSPORTEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", TRANSPORTEUR, SERVEUR, buf);

    write(pAEcriture[1], msg8, MSGSIZE); // Question 8: le transporteur livre à l'acheteur en lui remettant les 2 bons

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q9
    printf("[TRANSPORTEUR][%s]. Message recu de %s. Contenu = \"%s\".\n", TRANSPORTEUR, ACHETEUR, buf);
}

char* getNombrePalettes(){
    printf("%s. Calcul du nombre de palettes...\n", SERVEUR);
    int compteur = 1;
    int n = QteRouleauStock;
    char* nbPalettes;

    if (n <= PALETTE){
        sprintf(nbPalettes, "%d", compteur);
        return nbPalettes;
    } else {
        while (n > PALETTE){
            n = n-PALETTE;
            compteur ++;
        }
        sprintf(nbPalettes, "%d", compteur);
        return nbPalettes;
    }
}

char* getMontantFacture(){
    printf("%s. Calcul du montant de la facture...\n", SERVEUR);
    int prixTotal;
    char* prixFacture = NULL;

    prixFacture= (char*) malloc((10*sizeof(char)));
    prixTotal = (SURFACE * PrixRouleau);
    sprintf(prixFacture, "%d", prixTotal);
    return prixFacture;
}

char* concat(const char *s1, const char *s2){
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

