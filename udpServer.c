
  //Adicionar controle para escolher entre recepcao ou envio
  //Solicitar reenvio das mensagens
  //Adicionar escolha para tamanho das mensagens
/*Fiquei com uma duvida no trabalho e em relacao ao tamanho da PDU da camada de enlace, e que la fala que e pra solicitar pro usuario 
o tamanho do quadro, so que como serao dois programas rodando no caso os dois teria que informar o mesmo tamanho? Ou a gente pode definir
o tamanho?*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MSG 100
#define BODY 10
#define SIZE_FRAME 10
#define SIZE_CABECALHO 5
#define PORTA_CLIENT 4001
#define PORTA_SERVER 4002

void sendMsg(char *mensagem);
int getEndMsg(int sd, struct sockaddr_in endCli);
int server(int sd, struct sockaddr_in endCli);
void sendEndMsg(char *msg);
void client(int sd, struct sockaddr_in endCli);

typedef struct mensagemCompleta {
  char texto[1000];
  int verifica[99];
} MENSAGEMCOMPLETA;

int cont=0, verificaMensagem=1;
char ip[16];
MENSAGEMCOMPLETA mensagemCompleta;

int main() {
  // Inicializa as variaveis - SERVIDOR
  char mensagem[MAX_MSG];
  int sd, rc, confirmMsg = 1, verificaServer = 1;
  struct sockaddr_in endCli;   /* Vai conter identificacao do cliente */ 
  struct sockaddr_in endServ;  /* Vai conter identificacao do servidor local */

  // Realiza a leitura do ip do outro servidor/cliente
  printf("Digite o ip do servidor: ");
  scanf("%s", ip);

  // Verifica se o IP foi digitado
  if(strcmp(ip, "") == 0){
    printf("\nDados incorretos!!!%s", ip);
    return 0;
  }

  // Inicializa o socket
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sd<0) {
    printf("Nao pode abrir o socket \n");
    exit(1);  
  }

  // Configura o Servidor para receber as mensagens do outro cliente
  endServ.sin_family 	  = AF_INET;
  endServ.sin_addr.s_addr = inet_addr(ip); 
  endServ.sin_port 	  = htons(PORTA_SERVER);

  rc = bind (sd, (struct sockaddr *) &endServ,sizeof(endServ));
  if(rc<0) {
    printf("Nao pode fazer bind na porta %d \n", PORTA_SERVER);
    exit(1); 
  }

  printf("Conectado no IP: %s, porta SERVER: %d, porta CLIENT: %d\n", ip, PORTA_SERVER, PORTA_CLIENT);

  while(1) {
    while(verificaServer)
      verificaServer = server(sd, endCli);
    client(sd, endCli);
    verificaServer = 1;
  }
  return 0;
}

void client(int sd, struct sockaddr_in endCli){
  char mensagem[MAX_MSG];
  int verificaServer = 1, confirmMsg = 1;

  while(1){
    printf("Digite a mensagem (Digite '.' para sair): ");
    scanf("\n");
    fgets(mensagem, MAX_MSG, stdin);

    // Realiza o envio da mensagem e aguarda a confirmacao do recebimento
    while(confirmMsg){
      sendMsg(mensagem);
      confirmMsg = getEndMsg(sd, endCli);
    }
    confirmMsg = 1;

    if(strcmp(mensagem, ".\n") == 0)
      exit(1);
    else if(strcmp(mensagem, ".1\n") == 0){
      return;
    }
  }
}

int server(int sd, struct sockaddr_in endCli){
  char mensagem[MAX_MSG];
  int n, tam_Cli, i, j, qtdFrames, frame;

  if(cont>qtdFrames+15){
    sendEndMsg(".erro");
  }
    
  memset(mensagem,0x0,MAX_MSG);
  tam_Cli = sizeof(endCli);
  n = recvfrom(sd, mensagem, MAX_MSG, 0, (struct sockaddr *) &endCli, &tam_Cli);
  if(n<0) {
    printf("Nao pode receber dados \n");
    return 1;
  }

  qtdFrames = ((mensagem[0]-48)*10) + mensagem[1]-48;
  frame = (((mensagem[2]-48)*10) + mensagem[3]-48);

  j=0;
  for(i=4;i<strlen(mensagem);i++){
    mensagemCompleta.texto[frame*BODY+j] = mensagem[i];
    j++;
  }
  mensagemCompleta.verifica[frame] = 1;

  for(i=0;i<qtdFrames;i++){
    if(mensagemCompleta.verifica[i] != 1)
      verificaMensagem = 0;
  }
    
  if(verificaMensagem && mensagem[4]!='.'){
    printf("%s: ", inet_ntoa(endCli.sin_addr));
    for(i=0;i<qtdFrames*BODY;i++){
      printf("%c", mensagemCompleta.texto[i]);
    }
    for(i=0;i<98;i++){
      mensagemCompleta.verifica[i] = 0;
    }
    cont = 0;
    sendEndMsg(".ok");
  }
  verificaMensagem = 1;
  cont++;

  if(mensagem[4]=='.' && mensagem[5]=='1'){
    sendEndMsg(".ok");
    return 0;
  }
  else{
    if(mensagem[4]=='.'){
      sendEndMsg(".ok");
      exit(1);
    }
    return 1;
  }
}

//Envia mensagem de confirmacao que recebeu todas as mensagens
void sendEndMsg(char *msg){
  int sd, rc;
  struct sockaddr_in ladoCli;
  struct sockaddr_in ladoServ;  /* dados do servidor remoto */

  ladoServ.sin_family      = AF_INET;
  ladoServ.sin_addr.s_addr = inet_addr(ip);
  ladoServ.sin_port      = htons(PORTA_CLIENT);

  ladoCli.sin_family   = AF_INET;
  ladoCli.sin_addr.s_addr= htonl(INADDR_ANY);  
  ladoCli.sin_port       = htons(0); /* usa porta livre entre (1024-5000)*/

  sd = socket(AF_INET,SOCK_DGRAM,0);
  if(sd<0) {
    printf("%s: não pode abrir o socket \n",ip);
    return; 
  }

  rc = bind(sd, (struct sockaddr *) &ladoCli, sizeof(ladoCli));
  if(rc<0) {
    printf("%s: não pode fazer um bind da porta\n", ip);
    return; 
  }

  rc = sendto(sd, msg, strlen(msg), 0,(struct sockaddr *) &ladoServ, sizeof(ladoServ));
  if(rc<0) {
    printf("%s: nao pode enviar dados!!!\n",ip);
    close(sd);
    return; 
  }
}

// Funcao responsavel pelo envio da mensagem
void sendMsg(char *msg) {
  // Inicializa as variaveis - CLIENTE
  char frame[SIZE_CABECALHO + SIZE_FRAME];
  int sd, rc, i, j, lenMsg, qtdFrames, resto, temp, tempQtdFrames;
  struct sockaddr_in ladoCli;   /* dados do cliente local   */
  struct sockaddr_in ladoServ;  /* dados do servidor remoto */

  // Configura o cliente
  ladoServ.sin_family      = AF_INET;
  ladoServ.sin_addr.s_addr = inet_addr(ip);
  ladoServ.sin_port      = htons(PORTA_CLIENT);

  ladoCli.sin_family   = AF_INET;
  ladoCli.sin_addr.s_addr= htonl(INADDR_ANY);  
  ladoCli.sin_port       = htons(0); /* usa porta livre entre (1024-5000)*/

  sd = socket(AF_INET,SOCK_DGRAM,0);
  if(sd<0) {
    printf("%s: não pode abrir o socket \n",ip);
    exit(1); 
  }

  rc = bind(sd, (struct sockaddr *) &ladoCli, sizeof(ladoCli));
  if(rc<0) {
    printf("%s: não pode fazer um bind da porta\n", ip);
    exit(1); 
  }

  // Responsavel pela quebra da mensagem em quadros
  lenMsg = strlen(msg);
  qtdFrames = lenMsg / SIZE_FRAME;
  resto = lenMsg % SIZE_FRAME;

  tempQtdFrames = qtdFrames;
  if(resto>0)
    tempQtdFrames = qtdFrames + 1;

  for(i=0;i<qtdFrames;i++) {
    // Adiciona o cabecalho
    if(qtdFrames<10){
      frame[0] = '0';
      frame[1] = tempQtdFrames + '0';
    }
    else {
      temp = tempQtdFrames / 10;
      frame[0] = temp + '0';

      temp = tempQtdFrames % 10;
      frame[1] = temp + '0';
    }

    if(i<10){
      frame[2] = '0';
      frame[3] = i + '0';
    }
    else {
      temp = i / 10;
      frame[2] = temp + '0';

      temp = i % 10;
      frame[3] = temp + '0';
    }
      
    for(j=0;j<SIZE_FRAME;j++)
      frame[j+4] = msg[(i*SIZE_FRAME)+j];

    // Envia os quadros da mensagem
    rc = sendto(sd, frame, strlen(frame), 0,(struct sockaddr *) &ladoServ, sizeof(ladoServ));
    if(rc<0) {
      printf("%s: nao pode enviar dados!!!\n",ip);
      close(sd);
      exit(1); 
    }
  }

  if(resto>0) {
    // Adiciona o cabecalho
    if(tempQtdFrames<10){
      frame[0] = '0';
      frame[1] = tempQtdFrames + '0';

      frame[2] = '0';
      frame[3] = (tempQtdFrames-1) + '0';
    }
    else {
      temp = tempQtdFrames / 10;
      frame[0] = temp + '0';
      temp = (tempQtdFrames-1) / 10;
      frame[2] = temp + '0';

      temp = tempQtdFrames % 10;
      frame[1] = temp + '0';
      temp = (tempQtdFrames-1) % 10;
      frame[3] = temp + '0';
    }

    for(j=0;j<resto;j++)
      frame[j+4] = msg[(qtdFrames*SIZE_FRAME)+j];
      
    // Envia os quadros da mensagem
    rc = sendto(sd, frame, resto+4, 0,(struct sockaddr *) &ladoServ, sizeof(ladoServ));
    if(rc<0) {
      printf("%s: nao pode enviar dados!!!\n",ip);
      close(sd);
      exit(1); 
    }
  }
}

int getEndMsg(int sd, struct sockaddr_in endCli) {
  char mensagem[MAX_MSG];
  int n, tam_Cli;

  while(1) {  
    memset(mensagem,0x0,MAX_MSG);
    tam_Cli = sizeof(endCli);
    n = recvfrom(sd, mensagem, MAX_MSG, 0, (struct sockaddr *) &endCli, &tam_Cli);
    if(n<0) {
      continue;
    }
    if(strcmp(mensagem, ".ok") == 0)
      return 0;
    else
      return 1;
  }
}