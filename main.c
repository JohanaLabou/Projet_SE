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
#define MSGSIZE 256

void acheteur(int pSEcriture[], int pSLecture[], int pTEcriture[], int pTLecture[]);
void serveur(int pAEcriture[], int pALecture[], int pTEcriture[]);
void transporteur(int pAEcriture[], int pALecture[], int pSLecture[]);
char* concat(const char *s1, const char *s2);
char* getMontantFacture();
char* getNombrePalettes();

char* msg1 = "Bonjour, je souhaiterais avoir l'article "; // Q1
char* msg2 = "Le nombre de rouleaux en stock disponibles pour cet article est "; // Q2
char* msg3 = "Je souhaite passer une commande pour une surface de "; // Q3
char msg4[MSGSIZE] = "Commande recue. Nombre total de palettes: "; // Q4
char msg5[MSGSIZE] = "Montant total de la facture: "; // Q4
char* msg6 = ""; // Question 5
char* msg7 = "J'accuse reception de votre paiment. Le montant de la transaction etait de: "; // Q6
char* msg8 = "Voici un bon de livraison. Pour rappel, le nombre de palettes de l'article choisi est: "; // Q7
char* msg9 = "Voici un autre bon de livraison pour cette meme commande "; // Q7
char* msg10 = "Bonjour, je suis votre livreur. Voici les 2 bons de livraison"; // Q8
char* msg11 = "Merci. Je vous rends un bon signe"; // Q9

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
                        /*      p[0] ? contient le fichier descripteur de l'extremite de lecture
                                p[1] ? contient le fichier descripteur de l'extremite d'ecriture
                        */
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
    char* msg3Bis;

    write(pSEcriture[1], concat(msg1, ARTICLE), MSGSIZE); // Question 1: l'acheteur saisit le nom d'un article
    
    //nread = read(pSLecture[0], buf, MSGSIZE); // Q2 on lit la reponse du serveur
    //printf("ACHETEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    /*for (int i = 0; i < 2; i++) {
        nread = read(pSLecture[0], buf, MSGSIZE); // On lit le message du serveur de la Q2
        switch (nread){
            case -1:
                if (errno == EAGAIN){
                    printf("Tube vide\n");
                    sleep(1);
                    break;
                } else {
                    perror("Lecture\n");
                    exit(100);
                }
            default:
                printf("ACHETEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);
                break;
        }
    }*/

    sprintf(str3Bis, "%d", SURFACE); // on convertit la surface en string
    msg3Bis = concat(msg3, str3Bis); // on concatene ce string avec msg3 pour avoir le msg complet
    write(pSEcriture[1], msg3Bis, MSGSIZE); // Question 3: l'acheteur saisit la surface souhaitee

    nread = read(pSLecture[0],buf, MSGSIZE); // on lit la Q4
    printf("ACHETEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    nread = read(pSLecture[0],buf, MSGSIZE); // on lit la Q4
    printf("ACHETEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    /* Question 5: l'acheteur paie en saisissant son numero de carte bancaire et son cryptogramme */
    
    int crypto;
    printf( "Veuillez saisir votre cryptogramme : " );
    fflush( stdout );
    scanf("[%d]", &crypto);
    
    length = strlen (crypto);
    for (i=0;i<length; i++)
    {
        if (!isdigit(input[i]))
        {
            printf ("Entrez un nombre\n");
            exit(1);
        }
        if(length>3){
            printf("Entrez un cryptogramme de 3 chiffres");
        }
    }
    
    int numeroCarte;
    printf( "Veuillez saisir votre numéro de carte : " );
    fflush( stdout );
    scanf("[%d]", &numeroCarte);
    
    length = strlen (numeroCarte);
    for (i=0;i<length; i++)
    {
        if (!isdigit(input[i]))
        {
            printf ("Entrez un nombre de 16 chiffres\n");
            exit(1);
        }
        if(length != 16){
            printf("Entrez un cryptogramme de 3 chiffres");
        }
    }
    
    write(pSEcriture[1], &crypto, MSGSIZE);

    nread = read(pSLecture[0],buf, MSGSIZE); // on lit la Q6
    printf("ACHETEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    nread = read(pTLecture[0],buf, MSGSIZE); // on lit la Q8
    printf("ACHETEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    write(pTEcriture[1], msg11, MSGSIZE); // Question 9: l'acheteur signe un des bons qu'il remet au livreur
}


void serveur(int pAEcriture[], int pALecture[], int pTEcriture[]){
    close(pAEcriture[0]);
    close(pALecture[1]);
    close(pTEcriture[0]);

    int nread;
    char buf[MSGSIZE];
    char* str2Bis; // pour utiliser la fonction sprintf() et convertir un int en str
    char* msg2Bis;

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q1
    printf("\nSERVEUR. Message recu de l'acheteur. Contenu = \"%s\".\n", buf);

    sprintf(str2Bis, "%d", QteRouleauStock);
    msg2Bis = concat(msg2, str2Bis);
    write(pAEcriture[1], msg2Bis, MSGSIZE); // Question 2: le serveur web transmet la quantite disponible en stock

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q3
    printf("SERVEUR. Message recu de l'acheteur. Contenu = \"%s\".\n", buf);

    strcat(msg4, getNombrePalettes());
    write(pAEcriture[1], msg4, MSGSIZE); // Question 4: le serveur transmet le nombre de palettes a l'acheteur

    strcat(msg5, getMontantFacture());
    write(pAEcriture[1], msg5, MSGSIZE); // Question 4: le serveur transmet le montant de la facture a l'acheteur

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q5
    printf("SERVEUR. Message recu de l'acheteur. Contenu = \"%s\".\n", buf);

    strcat(msg7, getMontantFacture());
    write(pAEcriture[1], msg7, MSGSIZE); // Question 6: le serveur web envoie un accuse de reception du paiement a l'acheteur, rappelant le montant total de la transaction

    strcat(msg8, getNombrePalettes());
    write(pTEcriture[1], msg8, MSGSIZE); // Question 7: le serveur web transmet un bon de livraison comprenant le nombre de palettes de l'article choisi
    write(pTEcriture[1], msg9, MSGSIZE); // Q7: en double exemplaire au transporteur

}

void transporteur(int pAEcriture[], int pALecture[], int pSLecture[]){
    close(pAEcriture[0]);
    close(pALecture[1]);
    close(pSLecture[1]);

    int nread;
    char buf[MSGSIZE];

    /*
    nread = read(pSLecture[0], buf, MSGSIZE); // On lit la Q7
    printf("TRANSPORTEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    nread = read(pSLecture[0], buf, MSGSIZE); // On lit la Q7
    printf("TRANSPORTEUR. Message recu du serveur. Contenu = \"%s\".\n", buf);

    write(pAEcriture[1], msg10, MSGSIZE); // Question 8: le transporteur livre à l'acheteur en lui remettant les 2 bons

    nread = read(pALecture[0], buf, MSGSIZE); // On lit la Q9
    printf("TRANSPORTEUR. Message recu du transporteur. Contenu = \"%s\".\n", buf);
    */

}

char* getNombrePalettes(){
    printf("SERVEUR. Calcul du nombre de palettes...\n");
    int compteur = 1;
    int n = QteRouleauStock;
    char *nbPalettes;

    if (n <= 63){
        sprintf(nbPalettes, "%d", compteur);
        return nbPalettes;
    } else {
        while (n > 63){
            n = n-63;
            compteur ++;
        }
        sprintf(nbPalettes, "%d", compteur);
        return nbPalettes;
    }
}

char* getMontantFacture(){
    printf("SERVEUR. Calcul du montant de la facture...\n");
    int prixTotal;
    char *prixFacture = NULL;

    prixFacture= (char*) malloc((10*sizeof(char)));
    prixTotal = (SURFACE * PrixRouleau);
    sprintf(prixFacture, "%d", prixTotal);
    return prixFacture;
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

