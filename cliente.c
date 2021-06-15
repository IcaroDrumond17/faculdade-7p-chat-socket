#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

/*
	#######################################################################################

	Trabalho -  Chat Socket
	Professor: Elias - Sistemas Distribuidos

	Grupo
	- Guilherme Alves
	- Ícaro Drumond
	- Marlene Tonel

	Compilar e Executar o arquivo cliente.c 

	-- Compilar
		gcc cliente.c -o cliente.e -lpthread
	
	-- Executar
		./cliente.e IP (PADRÃO - 127.0.0.1) PORT (1080)

	#######################################################################################
*/

#define MAX_BUFFER 256
#define TAM_NOME 32

// cores ANSI
#define A_RED     "\x1b[31m"
#define A_GREEN   "\x1b[32m"
#define A_YELLOW  "\x1b[33m"
#define A_BLUE    "\x1b[34m"
#define A_MAGENTA "\x1b[35m"
#define A_CYAN    "\x1b[36m"
#define A_GRAY    "\e[0;37m"
#define A_RESET   "\x1b[0m"

// Global variables
volatile sig_atomic_t flag = 0;
int servidor = 0;
char nome[TAM_NOME];

void textMenu(){
	printf(A_GREEN "\n################################################" A_RESET);
	printf(A_GRAY"\n\n \t\t - Mundo Chat -");
	printf("\n A melhor plataforma para salas de bate-papo!");
	printf("\n \t Para sair da sala digite [ #sair ]"A_RESET);
	printf(A_GREEN"\n\n################################################\n\n"A_RESET);
}

void alertMSG(char * nome, int op){
	system("clear");
	textMenu();
	printf(A_RED"\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	printf("\n\n \t\t - Aviso! -");
	if(op){
		printf("\n O nome [ %s ] é muito pequeno! Mínimo 3 caracteres.", nome);
	}else{
		printf("\n O nome [ %s ] é muito grande! Máximo 32 caracteres.", nome);
	}
	printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n"A_RESET);
}

void str_overwrite_stdout() {
	printf(A_GREEN"%s", "> "A_RESET);
	fflush(stdout);
}

void pseudoTrim (char* string, int tam) {
	int i;
	for (i = 0; i < tam; i++) { // trim \n
		if (string[i] == '\n') {
			string[i] = '\0';
			break;
		}
	}
}

void garanteSaida(int sig) {
	flag = 1;
}

void enviaMensagem() {	
	char mensagem[MAX_BUFFER] = {};
	char buffer[MAX_BUFFER + TAM_NOME] = {};

	while(1) {
		str_overwrite_stdout();
		fgets(mensagem, MAX_BUFFER, stdin);
		pseudoTrim(mensagem, MAX_BUFFER);

		if (strcmp(mensagem, "#sair") == 0) {
			break;
		} else {
			sprintf(buffer, "%s diz: %s\n", nome, mensagem);
			send(servidor, buffer, strlen(buffer), 0);
		}

		bzero(mensagem, MAX_BUFFER);
		bzero(buffer, MAX_BUFFER + TAM_NOME);
	}
	garanteSaida(2);
}

void recebeMensagem() {
	struct tm * hr;
	time_t segundos;
	
	char mensagem[MAX_BUFFER] = {};
	while (1) {
		int receive = recv(servidor, mensagem, MAX_BUFFER, 0);
		if (receive > 0) {
			time(&segundos);
			hr = localtime(&segundos);
			printf(A_CYAN"[%02d:%02d:%02d]"A_RESET A_GRAY" %s "A_RESET, hr->tm_hour, hr->tm_min, hr->tm_sec, mensagem);
			str_overwrite_stdout();
		} else if (receive == 0) {
			break;
		} else {
			// -1
		}
		memset(mensagem, 0, sizeof(mensagem));
	}
}

int main(int argc, char **argv){
	setlocale(LC_ALL, "Portuguese");
    system("clear");

	int const PORT = atoi(argv[2]);
	int login;
    char const *ip = argv[1];

	signal(SIGINT, garanteSaida);
	
	textMenu();

	do{
	
		printf(A_CYAN"--------------------------------------------------"A_RESET);
		printf(A_GRAY"\n \t\t - CADASTRO -"A_RESET);
		printf(A_CYAN"\n--------------------------------------------------"A_RESET);
		printf(A_YELLOW"\n\n Para continuarmos, por favor insira seu nome:");
		printf("\n >" A_RESET);
		fgets(nome, TAM_NOME, stdin);
		pseudoTrim(nome, strlen(nome));

		login = 0;

		if (strlen(nome) > TAM_NOME){
			alertMSG(nome, 0);
		}else if(strlen(nome) < 3){
			alertMSG(nome, 1);
		}else if(strcmp(nome, "#sair") == 0){
			system("clear");
			printf(A_GRAY"\n Obrigado por utilizar nossos servicos de menssagem!\nVolte mais tarde!\n"A_RESET);
			return 0;
		}else{
			system("clear");
			textMenu();
			login = 1;
			printf(A_YELLOW"------------------------------------------------------");
			printf("\n\n Nome cadastrado com sucesso! Aguardando conexão...");
			printf("\n\n------------------------------------------------------\n\n\n"A_RESET);
		}

	}while(!login);
	

	struct sockaddr_in enderecoservidor;

	/* Configurando o socket */
	servidor = socket(AF_INET, SOCK_STREAM, 0);
	enderecoservidor.sin_family = AF_INET;
	enderecoservidor.sin_addr.s_addr = inet_addr(ip);
	enderecoservidor.sin_port = htons(PORT);


	/* Conexão */
	int err = connect(servidor, (struct sockaddr *)&enderecoservidor, sizeof(enderecoservidor));
	if (err == -1) {
		printf(A_RED"ERRO: connect\n"A_RESET);
		return EXIT_FAILURE;
	}

	/* Enviar nome */
	send(servidor, nome, TAM_NOME, 0);

	system("clear");
	textMenu();

	struct tm * dt;
	time_t segundos;
	time(&segundos);
	dt = localtime(&segundos);
	printf(A_GRAY"#### Bem vindo a sala de chat! %d/%d/%d às %02d:%02d:%02d ####\n\n"A_RESET, 
	dt->tm_mday, dt->tm_mon+1, dt->tm_year+1900, dt->tm_hour, dt->tm_min, dt->tm_sec );

	pthread_t threadmensagem;

	if(pthread_create(&threadmensagem, NULL, (void *) enviaMensagem, NULL) != 0){
		printf(A_RED"ERRO: pthread\n"A_RESET);
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;

	if(pthread_create(&recv_msg_thread, NULL, (void *) recebeMensagem, NULL) != 0){
		printf(A_RED"ERRO: pthread\n"A_RESET);
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			system("clear");
			textMenu();
			printf(A_RED"\nVocê saiu dessa conversa!\n"A_RESET);
			printf(A_GRAY"\nObrigado por compartilhar sua companhia!");
			printf("\nVolte mais tarde!\n\n"A_RESET);
			break;
		}
	}

	close(servidor);

	return EXIT_SUCCESS;
}