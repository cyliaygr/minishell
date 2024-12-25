#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> /* exit */
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readcmd.c"

//minishell questions 1 à 5
int main(){
    struct cmdline *commande; 
  
    
    do{
        // afficher l'invite de commande
        printf("sh-3.2$ ");
        commande = readcmd(); 
    
        //Teste si la commande est vide
        if (commande->seq[0] == NULL){
            printf("La commande est vide\n");
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

            } else {

            // Creation du processus fils qui permettra de ne pas recouvrir le code qui suit le exec.
                pid_t pidFils = fork();
                
                // si le pid < 0, cela signifie qu'il a mal été crée
                if (pidFils < 0) {
                    printf("ECHEC Erreur lors de la création du processus fils \n");
                    return EXIT_FAILURE;
                }

                else if (pidFils == 0) {		/* fils */
                    execvp(commande->seq[0][0],commande->seq[0]);
                    printf("ECHEC Erreur lors de la créa du proc fils \n");
                    return EXIT_FAILURE; 
                }
                else {		/* père */
                    int status;
                    if (commande->backgrounded != NULL) {
                        /*On fait rien, le père n'attend pas (tâche de fond)*/
                    }else {
                        waitpid(pidFils, &status, 0);
                        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) { // si le fils a terminé
                        }
                        else {
                        printf("ECHEC Erreur lors de l'execution\n");
                        }
                    }
                }   
            }       
        }
    
    
    }while(1);
    
    return EXIT_SUCCESS; /* -> exit(EXIT_SUCCESS); pour le père */
}
    
    
    
