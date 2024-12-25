#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h> /* exit */
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "readcmd.c"
#include "readcmd.h"

#define CAPACITE 20

//Minishell questions 1 à 6
//Definition des types utiles pour la suite
struct cmdline *commande; 

enum status {
     Running, Suspended
    };
typedef enum status status;

typedef struct {
    pid_t PID;
    status statut;
    int ID;
    char *command;
    } process;
    
typedef process Liste_Proc[CAPACITE];


//Ensemble des fonctions qui permettent de gerer les jobs
Liste_Proc list;
int id = 0;
int commande_fg; 
bool CtrlZ;
pid_t proc_arrplan = 0;
pid_t proc_avant = 0;
int id_proc_avant = 0;

char* Etat(status etat){
    char* statut = "Running";
    
    if (etat == Suspended){
        statut = "Suspended";
    }
    return statut;
}


//Savoir la taille de la liste de processus
int Taille(Liste_Proc list){
    int i=0;
    while(list[i].command != NULL) {
        i++;
        }
    return i;
    }

//Permet d'obtenir l'indice du processus de la liste grace a son identifiant
int getIndice(int id) {
    int i = 0;
    while(list[i].command != NULL && list[i].ID != id){
        i++;
        }
    return i;
    }

// Permet d'obtenir l'indice du processus de la liste grace a son pid
int getIndice_pid(pid_t pid) {
    int i = 0;
    while(list[i].command != NULL && list[i].PID != pid){
        i++;
        }
    return i;
    }

//Permet de savoir si un processus est present ou non
bool estPresent(int id){
    int indice = getIndice(id);
    bool present = false;
    
    if (list[indice].ID == id){
        present = true;
    }
    return present;
}


//Afficher un job
void afficher_proc(process processus){
    char *etat = Etat(processus.statut);
    printf( "[%d]      PID: %d      %s      %s", processus.ID, processus.PID, etat, processus.command);

}

//Afficher la liste des jobs
void list_job(Liste_Proc list){
    int taille = Taille(list);
    for (int i = 0; i< taille; i++){
        afficher_proc(list[i]);
        printf("\n");
        
    }

}
//Créer un nouveau processus
process CreerJob(int id, pid_t pid, status statut, char* commande){
    process p;
    p.PID = pid;
    p.statut = statut;
    p.ID = id;
    p.command = malloc(strlen(commande)*sizeof(char));
    strcpy(p.command, commande);
    return p;
    }

//Ajouter un nouveau processus dans la liste
void AjouterJob(Liste_Proc list, process processus){
    int taille = Taille(list);
    list[taille] = processus;

}

//Supprimer un processus de la liste 
void SupprimerJob(Liste_Proc list, pid_t pid){
    int indice = getIndice_pid(pid);
    free(list[indice].command);
    list[indice].command = NULL;
}

//Changer l'etat d'un job
void SetStatut(Liste_Proc list, pid_t pid, status nouveau_etat) {
    int indice = getIndice_pid(pid);
    list[indice].statut = nouveau_etat;
    }


//Suspendre un job
void stopjob(int id){
    int i = getIndice(id);
    if (list[i].ID == id && list[i].statut == Running) {
        kill(list[i].PID, SIGSTOP);
        SetStatut(list, list[i].PID, Suspended);
        list_job(list);
       
    }
    else {
        printf("Ce processus n'est pas actif ou inexistant \n");
    }
    
}

//Reprendre un job en arriere-plan
void bg(int id){
    int i = getIndice(id);
    if (list[i].ID == id && list[i].statut == Suspended) {
        kill(list[i].PID, SIGCONT);
        SetStatut(list, list[i].PID, Running);
    }
    else {
        printf("Ce processus n'est pas actif ou inexistant  \n");
    }
    
}

//Reprendre un job en avant-plan
void fg(int id){
    int pidj;
    int i = getIndice(id);
    

    if (list[i].ID == id){
        pidj = list[i].PID;
        kill(list[i].PID, SIGCONT);
        SetStatut(list, list[i].PID, Running);
        pause();
    }
    else {
        printf("Ce processus n'existe pas\n");
    }
    
}

// handler sigchld
void handler_SIGCHLD (int sig) {
    int wstatus, fils_termine;
    fils_termine = (int)waitpid(-1,&wstatus,WUNTRACED|WNOHANG|WCONTINUED);
    if (fils_termine == -1){
        perror("waitpid");
        exit(EXIT_FAILURE);
        }
    else if(fils_termine > 0){
        if (WIFEXITED(wstatus)) {
            SupprimerJob(list, fils_termine);
            }
        else if(WIFCONTINUED(wstatus)) {
            SetStatut(list,fils_termine,Running);
            }
        else if(WIFSTOPPED(wstatus)){
            SetStatut(list,fils_termine,Suspended);
            //sj(pid)
            }
        else if WIFSIGNALED(wstatus) {  
        //Traitement de Ctrl-Z
            SupprimerJob(list, fils_termine);
            return;
                
                
        }
          
        }   
        
        
        }
    


// handler SIGTSTP, suspendre proc en avant plan
void handler_SIGTSTP (int sig) {
     if (proc_avant > 0) {
        // Suspendre le processus en avant-plan
        
        stopjob(id_proc_avant);
        printf("Processus en avant-plan suspendu.\n");
        proc_avant = 0;
    }
    
    }

//handler SIGINT
void handler_SIGINT (int sig) {
    if (proc_avant > 0) {
        // Suspendre le processus en avant-plan
        kill(proc_avant, SIGINT);
        SupprimerJob(list, proc_avant);
        printf("Processus en avant-plan terminé.\n");
        proc_avant = 0;
    }
    }


int main(){

    int tube[2];
    int fd, fl; /*desc de fichiers*/
    pid_t pidFils;

    //Association du signal SIGCHLD et son handler
        struct sigaction action;
        sigemptyset(&action.sa_mask);
        action.sa_handler = handler_SIGCHLD;
        action.sa_flags = SA_RESTART;
        sigaction(SIGCHLD, &action, NULL);


    //Ignorer SIGINT SIGSTOP en utilisant le handler par defaut de SIGIGN;
        sigemptyset(&action.sa_mask);
        action.sa_handler = SIG_IGN;
        action.sa_flags = SA_RESTART;
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGTSTP, &action, NULL);


       
    do{
        fflush(stdout);
        printf("sh-3.2$ ");
        commande = readcmd(); 


        
	
    
        if (commande->seq[0] == NULL){
           //printf("La commande est vide\n");
            continue;
        }
        else{
            //On regarde si la commande est exit
            if (strcmp(commande->seq[0][0], "exit")==0){
            return EXIT_SUCCESS;
            }

            //On regarde si la commande est cd
            else if (strcmp(commande->seq[0][0], "cd")==0){
                if (chdir(commande->seq[0][1]) < 0){
                    printf("La commande cd est incorrecte\n");
                }
            }
            //On regarde si la commande est lj
            else if (strcmp(commande->seq[0][0], "lj")==0){
                list_job(list);
            }

            //On regarde si la commande est sj
            else if (strcmp(commande->seq[0][0], "sj")==0){
                int id = atoi(commande->seq[0][1]); //atoi() convertit le début de la chaîne pointée par nptr en entier de type in
                stopjob(id);
                
            }

            //On regarde si la commande est bg
            else if (strcmp(commande->seq[0][0], "bg")==0){
                int id = atoi(commande->seq[0][1]);
                bg(id);
                
            }

            //On regarde si la commande est fg
            else if (strcmp(commande->seq[0][0], "fg")==0){
                int id = atoi(commande->seq[0][1]);
                fg(id);
                
            }

            //On regarde si la commande est susp pour suspendre le minishell
            else if (strcmp(commande->seq[0][0], "susp")==0){
                printf("Suspendre le minishell.\n");
                action.sa_handler = SIG_DFL;
                action.sa_flags = SA_RESTART;
                sigaction(SIGINT, &action, NULL);
                sigaction(SIGTSTP, &action, NULL);
                kill(getpid(), SIGSTOP);
                
            }

             else {

            // Creation du processus fils qui permettra de ne pas recouvrir le code qui suit le exec.
                pidFils = fork();
                
                // si le pid < 0, cela signifie qu'il a mal été crée
                if (pidFils < 0) {
                    printf("ECHEC Erreur lors de la création du processus fils \n");
                    return EXIT_FAILURE;
                }

                else if (pidFils == 0) {		/* fils */

                    // On designore les signaux sigtstp et sigint chez le fils
                    action.sa_handler = SIG_DFL;
                    action.sa_flags = SA_RESTART;
                    sigaction(SIGINT, &action, NULL);
                    sigaction(SIGTSTP, &action, NULL);

                    if(commande->seq[1] != NULL) {
                        
                        pipe(tube);
                        pidFils = fork();
                        if (pidFils == 0){
                            /*fils du fils*/
                            close(tube[1]);
                            dup2(tube[0],0);
                            if(commande->out != NULL) {
                                    fd = open(commande->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                    if (fd == -1) {
                                        printf("Erreur lors de l'ouverture du fichier \n");
                                        return EXIT_FAILURE;
                                    }
                                    dup2(fd, 1);
                                    close(fd);
                                
                            }
                            execvp(commande->seq[1][0],commande->seq[1]);

                        } else {
                            close(tube[0]);
                            dup2(tube[1],1);
                            if(commande->in != NULL) {
                                fl = open(commande->in, O_RDONLY, NULL);
                                dup2(fl, 0);
                                close(fl);
                                    if (fl == -1) {
                                        printf("Erreur lors de l'ouverture du fichier \n");
                                        return EXIT_FAILURE;
                                    }
                                 
                            }
                            /*execvp(commande->seq[1][0],commande->seq[1]);*/

                       }     
                    } else {

                    //Q9 redirection entrée standard > fichier
                    if(commande->in != NULL) {
                        fl = open(commande->in, O_RDONLY, NULL);
                        if (fl == -1) {
                            printf("Erreur lors de l'ouverture du fichier \n");
                            return EXIT_FAILURE;
                        }
                        dup2(fl, 0);
                        close(fl);
                    }

                    //Q9 redirection sortie standard > fichier
                    if(commande->out != NULL) {
                        fd = open(commande->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd == -1) {
                            printf("Erreur lors de l'ouverture du fichier \n");
                            return EXIT_FAILURE;
                        }
                        dup2(fd, 1);
                        close(fd);
                    }
                    }
                    execvp(commande->seq[0][0],commande->seq[0]);
                    printf("ECHEC Erreur lors de la création du proc fils \n");
                    return EXIT_FAILURE; /* 	bonne pratique : 
                                        terminer les processus par un exit explicite */
                }
                else {		/* père */
                    id++;
                    process p;
                    p = CreerJob(id, pidFils, Running, commande->seq[0][0]);
                    AjouterJob(list,p);
                    proc_avant = pidFils;
                    id_proc_avant = id;
                    if (commande->backgrounded == NULL) {
                        pause();
                        /*proc_arrplan = pidFils;*/
                        if (estPresent(id)){
                            SupprimerJob(list, pidFils);
                            id --;
                        }
                    }
                    else {
                         NULL;  
                    }
               }       
            }
        
        }
    
    
    }while(1);
    
 
}
    
    
    
