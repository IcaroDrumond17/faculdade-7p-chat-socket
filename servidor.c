#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

/*
	#######################################################################################

	Trabalho -  Chat Socket
	Professor: Elias - Sistemas Distribuidos

	Grupo
	- Guilherme Alves
	- Ícaro Drumond
	- Marlene Tonel

	Compilar e Executar o arquivo servidor.c 

	-- Compilar
		gcc servidor.c -o servidor.e -lpthread
	
	-- Executar
		./servidor.e
		
	#######################################################################################
*/

// cores ANSI
#define A_RED     "\x1b[31m"
#define A_GREEN   "\x1b[32m"
#define A_YELLOW  "\x1b[33m"
#define A_BLUE    "\x1b[34m"
#define A_MAGENTA "\x1b[35m"
#define A_CYAN    "\x1b[36m"
#define A_GRAY    "\e[0;37m"
#define A_RESET   "\x1b[0m"

#define MAX_CLIENTES 16
#define MAX_BUFFER 256
#define TAM_NOME 32

static _Atomic unsigned int Ncliente = 0;
static int uid = 10;

/* Struct que representa o cliente */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char nome[TAM_NOME];
} cliente_s;

cliente_s *TodosClientes[MAX_CLIENTES];

pthread_mutex_t ClientesMutex = PTHREAD_MUTEX_INITIALIZER;

void pseudoTrim (char* string, int tam) {
	int i;
	for (i = 0; i < tam; i++) { // trim \n
		if (string[i] == '\n') {
			string[i] = '\0';
			break;
		}
	}
}

/* Adiciona clientes a fila */
void adcionaFila(cliente_s *cl){
	pthread_mutex_lock(&ClientesMutex);

	for(int i=0; i < MAX_CLIENTES; ++i){
		if(!TodosClientes[i]){
			TodosClientes[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&ClientesMutex);
}

/* remove clients da fila */
void removeFila(int uid){
	pthread_mutex_lock(&ClientesMutex);

	for(int i=0; i < MAX_CLIENTES; ++i){
		if(TodosClientes[i]){
			if(TodosClientes[i]->uid == uid){
				TodosClientes[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&ClientesMutex);
}

/* Encaminha as mensagem aos outros usuarios */
void enviaMensagem(char *s, int uid){
	pthread_mutex_lock(&ClientesMutex);

	for(int i=0; i<MAX_CLIENTES; ++i){
		if(TodosClientes[i]){
			if(TodosClientes[i]->uid != uid){
				if(write(TodosClientes[i]->sockfd, s, strlen(s)) < 0){
					perror(A_RED"ERRO: write to descriptor failed"A_RESET);
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&ClientesMutex);
}

/* Comunica com o cliente */
void *controlaCliente(void *arg){
	char buffer[MAX_BUFFER];
	char nome[TAM_NOME];
	int flag = 0;

	Ncliente++;
	cliente_s *cli = (cliente_s *)arg;

	/* Trata o nome */
	if(recv(cli->sockfd, nome, TAM_NOME, 0) <= 0 || strlen(nome) <	2 || strlen(nome) >= TAM_NOME-1){
		printf(A_YELLOW"Não entrou o nome.\n"A_RESET);
		flag = 1;
	} else{
		strcpy(cli->nome, nome);
		sprintf(buffer, A_GREEN"O usuario: %s entrou.\n"A_RESET, cli->nome);
		printf("%s", buffer);
		enviaMensagem(buffer, cli->uid);
	}

	bzero(buffer, MAX_BUFFER);

	while(1){
		if (flag) {
			break;
		}

		int receive = recv(cli->sockfd, buffer, MAX_BUFFER, 0);
		if (receive > 0){
			if(strlen(buffer) > 0){
				enviaMensagem(buffer, cli->uid);

				pseudoTrim(buffer, strlen(buffer));
				printf("%s \n", buffer);
			}
		} else if (receive == 0 || strcmp(buffer, "#sair") == 0){
			sprintf(buffer, "%s saiu\n", cli->nome);
			printf("%s", buffer);
			enviaMensagem(buffer, cli->uid);
			flag = 1;
		} else {
			printf(A_RED"ERRO: -1\n"A_RESET);
			flag = 1;
		}

		bzero(buffer, MAX_BUFFER);
	}

	/* Remove cliente da fila e limpa a thread */
	close(cli->sockfd);
	removeFila(cli->uid);
	free(cli);
	Ncliente--;
	pthread_detach(pthread_self());

	return NULL;
}


int main(){

	int servidor = 0, conexao = 0;
	struct sockaddr_in enderecocliente;
	pthread_t tid;

	/* Configurações Socket */
	servidor = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in enderecoservidor = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(1080)
    };

	/* Conecta */
	if(bind(servidor, (struct sockaddr*)&enderecoservidor, sizeof(enderecoservidor)) < 0) {
		perror(A_RED"ERRO: Falhou ao conectar ao socket\n"A_RESET);
		return EXIT_FAILURE;
	}

	/* Escuta */
	if (listen(servidor, 10) < 0) {
		perror(A_RED"ERRO: Falhou ao escutar o socket!\n"A_RESET);
		return EXIT_FAILURE;
	}

	printf(A_GREEN"#### Sala de chat ####\n"A_RESET);

	while(1){
		socklen_t cliente = sizeof(enderecocliente);
		conexao = accept(servidor, (struct sockaddr*)&enderecocliente, &cliente);

		/* Verifica se chegou ao limite de conexoes */
		if((Ncliente + 1) == MAX_CLIENTES){
			printf(A_RED"Numero maximo de pessoas tentaram acessar o sistema!\n\n"A_RESET);
			close(conexao);
			continue;
		}

		/* Configura o cliente */
		cliente_s *cli = (cliente_s *)malloc(sizeof(cliente_s));
		cli->address = enderecocliente;
		cli->sockfd = conexao;
		cli->uid = uid++;

		/* Adiciona o cliente e cria a thread */
		adcionaFila(cli);
		pthread_create(&tid, NULL, &controlaCliente, (void*)cli);

		sleep(1);
	}

	return EXIT_SUCCESS;
}