/*
  PROGRAMA SERVIDOR PARA CONEXÃO SOCKETS EM SERVIDOR DE ARQUIVOS
  Programadores: Claudio André Korb
                 Rafael Canal
  Instituição: Universidade Federal de Santa Catarina
  Disciplina: Sistemas Operacionais

  Big thanks to Brian "Beej Jorgensen" Hall for his amazing guide to Network Programming
  Available at: http://beej.us/guide/bgnet/html/multi/index.html
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "definicoes.h"

#define PORT 8979
#define BACKLOG 5

pthread_mutex_t mutex;                                                          //Mutex utilizado para controle de condições de corrida
FILE* sistema_arq;
int strcmpst1nl (const char * s1, const char * s2);
void * at_connection(void *socket_fd);

int criarArquivo(int new_socket, char *diretorio);
int criarDiretorio(int new_socket, char *diretorio);
int procuraInodeLivre();
int procuraBlocoLivre();
int sinalizaBloco(int index);
int buscaInodePorNome(char* nome);

int main(int argc, char const * argv[])
{
  int socket_fd;                             //SOCKET DO SERVIDOR
  int socket_client;                          //SOCKET CLIENTE
  int tam;
  int *new_socket;                           //ATRIBUÍDO PARA CADA NOVA CONEXÃO
  struct sockaddr_in server, client;
  sistema_arq = fopen("sistema.bin", "r+b");
  if(!sistema_arq)
  {
    printf("Erro: Abrir arquivo bin\n");
  }

  //CRIANDO INODE E BLOCO PARA ROOT
  inode inodeRoot;
  bloco blocoRoot;
  memset(blocoRoot.conteudo, 0, sizeof(blocoRoot.conteudo));
  memset(inodeRoot.nome, 0, sizeof(inodeRoot.nome));
  memset(inodeRoot.blocos, 0, sizeof(inodeRoot.blocos));
  inodeRoot.numBlocos = 0;
  strcpy(inodeRoot.nome, "root");
  inodeRoot.blocos[0] = INICIO_DADOS + 1*TAMANHO_BLOCO; //PULA O BLOCO NUMERO 0
  inodeRoot.numBlocos++;

  //ESCREVENDO O BLOCO COMO OCUPADO NA LISTA DE CONTROLE
  sinalizaBloco(1);

  //ESCREVENDO O INODE DA ROOT NO ARQUIVO
  fseek(sistema_arq, INICIO_INODES, SEEK_SET);
  fwrite(&inodeRoot, TAMANHO_INODE, 1, sistema_arq);

  count_inodes++;
  count_blocos++;

  fclose(sistema_arq);

//CRIANDO O SOCKET PARA O SERVIDOR
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);      //AF_INET IPv4 SOCK_STREAM TCP
  if(socket_fd == -1)
  {
    printf("Erro: Create socket");
  }
  printf("Socket criado\n");
  //============================================================================

  //ADICIONANDO O ENDEREÇO AO SOCKET
  server.sin_family = AF_INET;                    //IPV4
  server.sin_addr.s_addr = INADDR_ANY;            //PREENCHE MEU IP AUTOMATICAMENTE
  server.sin_port = htons(PORT);
  //============================================================================

  //LIGANDO O SOCKET À PORTA 8979
  if(bind(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    printf("Erro: Bind");
    return 1;
  }
  printf("Bind ok\n");
  //============================================================================
  pthread_mutex_init(&mutex, NULL);
  //ESPERANDO POR CONEXÕES
  while(1){
    listen(socket_fd, BACKLOG);
    printf("Esperando conexoes...\n");
  //============================================================================
    tam = sizeof(struct sockaddr_in);
    while(socket_client = accept(socket_fd, (struct sockaddr*)&client, (socklen_t*)&tam)) //ACEITANDO A CONEXÃO
    {
      struct in_addr ipAddr = client.sin_addr;
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &ipAddr, ip, INET_ADDRSTRLEN);
      printf("Cliente conectado: %s\n", ip);
      pthread_t w_thread;
      new_socket = (int*)malloc(sizeof(int));
      *new_socket = socket_client;

      if(pthread_create(&w_thread, NULL, at_connection, (void*)new_socket) < 0)
      {
        printf("Erro: create thread");
        return 1;
      }
      printf("Thread criada\n");
    }
    if(socket_client < 0)
    {
      printf("Erro: accept");
      return 1;
    }
    pthread_mutex_destroy(&mutex);
}
return 0;
}

int strcmpst1nl (const char * s1, const char * s2)
{
  char s1c;                                     //FUNÇÃO USADA PARA COMPARAR DUAS STRINGS, SENDO QUE
  do                                            // A SEGUNDA TERMINA EM CARACTERE NULO
    {
      s1c = *s1;
      if (s1c == '\n')
          s1c = 0;
      if (s1c != *s2)
          return 1;
      s1++;
      s2++;
    } while (s1c);
  return 0;
}

void * at_connection(void* socket_fd)
{
  int valread;
  char opt[1024] = "";
  char mensagem[1024] = "";
  char diretorioAtual[1024] = "root";
  int new_socket = *(int*)socket_fd;

  strcpy(mensagem, "\nBem vindo ao servidor de arquivos!\nComandos: \ncreate -- Criar Arquivo \ndelete -- Deletar Arquivo \nwrite -- Escrever no Arquivo\nread -- Mostrar Conteudo do Arquivo\nmkdir -- Criar Diretorio \nrmdir -- Remover Diretorio \ndir -- Mostrar conteudo do diretorio\ncd -- Trocar de Diretorio\nhelp -- Comandos \nclose -- Encerrar conexao\n");
  send(new_socket, mensagem, strlen(mensagem), 0);

  while(1)
  {
    valread = read(new_socket, opt, 1024);
    if(strcmpst1nl(opt, "create") == 0)
    {
      criarArquivo(new_socket, diretorioAtual);
    }else{
      if(strcmpst1nl(opt, "mkdir") == 0)
      {
        criarDiretorio(new_socket, diretorioAtual);
      }
    }
  }
}

int criarDiretorio(int new_socket, char* diretorio)
{
  char buffer[1024] = "";
  char mensagem[1024] = "";
  int valread;

  strcpy(mensagem, "Digite o nome do diretorio");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  if(valread = read(new_socket, buffer, 20) == -1)
  {
      strcpy(mensagem, "Nome do diretorio invalido!");
      send(new_socket, mensagem, strlen(mensagem), 0);
      memset(mensagem, 0, sizeof(mensagem));
      return -1;
  }

  inode novoDiretorio;
  novoDiretorio.tipo = 0;
  novoDiretorio.vago = 1;
  memset(novoDiretorio.nome, 0, sizeof(novoDiretorio.nome));
  memset(novoDiretorio.blocos, 0, sizeof(novoDiretorio.blocos));
  novoDiretorio.numBlocos = 0;
  strcpy(novoDiretorio.nome, buffer);

  //PROCURANDO INODE LIVRE
  int posicao = procuraInodeLivre();

  //PROCURANDO BLOCO LIVRE
  int novoBloco = procuraBlocoLivre();
  if(novoBloco == -1)
  {
    strcpy(mensagem, "Nao foi possivel encontrar um bloco disponivel");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -2;
  }
  //ADICIONANDO O BLOCO AO INODE
  sinalizaBloco(novoBloco);
  novoDiretorio.blocos[0] = INICIO_DADOS + (novoBloco)*TAMANHO_BLOCO;
  novoDiretorio.numBlocos++;

  FILE * sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    strcpy(mensagem, "Erro: abrir arquivo");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -1;
  }

  //ESCREVENDO O INODE NO ARQUIVO
  fseek(sistema, posicao, SEEK_SET);
  fwrite(&novoDiretorio, sizeof(novoDiretorio), 1, sistema);

  //ADICIONANDO O INODE AO DIRETORIO ONDE FOI CRIADO
  int pos = buscaInodePorNome(diretorio);
  if(pos == -1)
  {
    strcpy(mensagem, "Erro: diretorio raiz invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -3;
  }

  fseek(sistema, pos, SEEK_SET);
  inode diretorioAtual;
  fread(&diretorioAtual, sizeof(inode), 1, sistema);
  diretorioAtual.blocos[diretorioAtual.numBlocos++] = posicao;
  fseek(sistema, pos, SEEK_SET);
  fwrite(&diretorioAtual, sizeof(diretorioAtual), 1, sistema);

  strcpy(mensagem, "Diretorio criado com sucesso");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));
  fclose(sistema);
  return 0;

}


int criarArquivo(int new_socket, char* diretorio)
{
  char buffer[1024] = "";
  char mensagem[1024] = "";
  int valread;

  strcpy(mensagem, "Digite o nome do arquivo:");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  if(valread = read(new_socket, buffer, 20) == -1)
  {
    strcpy(mensagem, "Nome do arquivo invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -1;
  }

  inode novoArquivo;
  memset(novoArquivo.nome, 0, sizeof(novoArquivo.nome));
  memset(novoArquivo.blocos, 0, sizeof(novoArquivo.blocos));
  novoArquivo.numBlocos = 0;
  novoArquivo.tipo = 1;
  novoArquivo.vago = 1;
  strcpy(novoArquivo.nome, buffer);

  //PROCURA UM INODE LIVRE
  int posicao = procuraInodeLivre();
  if(posicao == -1)
  {
    strcpy(mensagem, "Nao foi possivel encontrar inode livre!");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 1;
  }

  //ADIIONAR UM BLOCO DE MEMORIA AO NOVO ARQUIVO
  int novoBloco = procuraBlocoLivre();                                      // PROCURA UM BLOCO DE MEMORIA LIVRE NA LISTA
  if(novoBloco == -1)
  {
    strcpy(mensagem, "Problema ao achar bloco livre!");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -3;
  }

  sinalizaBloco(novoBloco);                                                //SINALIZA QUE O NOVO BLOCO ESTÁ OCUPADO
  novoArquivo.blocos[0] = INICIO_DADOS + (novoBloco)*TAMANHO_BLOCO;      //ATRIBUI O BLOCO LIVRE AO INODE
  novoArquivo.numBlocos++;

  FILE* sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    strcpy(mensagem, "Nao foi possivel criar o arquivo");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 2;
  }

  //ESCREVENDO O INODE NO ARQUIVO
  fseek(sistema, posicao, SEEK_SET);
  fwrite(&novoArquivo, sizeof(novoArquivo), 1, sistema);
  fclose(sistema);

  //ADICIONANDO O INODE CRIADO AO INODE DO DIRETORIO ONDE FOI CRIADO
  int pos = buscaInodePorNome(diretorio);
  if(pos == -1)
  {
    strcpy(mensagem, "Erro ao criar arquivo: Diretorio invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 4;
  }

  sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    strcpy(mensagem, "Erro ao criar arquivo");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 5;
  }

  fseek(sistema, pos, SEEK_SET);
  inode diretorioAtual;
  fread(&diretorioAtual, sizeof(inode), 1, sistema);
  diretorioAtual.blocos[diretorioAtual.numBlocos++] = posicao;
  fseek(sistema, pos, SEEK_SET);
  fwrite(&diretorioAtual, sizeof(diretorioAtual), 1, sistema);

  snprintf(mensagem, sizeof(mensagem), "Arquivo criado no inode: %d", posicao-TAMANHO_INODE);
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  fclose(sistema);

  return 0;
}

int procuraInodeLivre()
{
  FILE * sistema = fopen("sistema.bin", "rb");
  fseek(sistema, INICIO_INODES, SEEK_SET);
  inode candidato;
  if(!sistema)
  {
    return -1;
  }

  int i = 0;
  while(i < MAX_INODES)
  {
    fread(&candidato, sizeof(inode), 1, sistema);
    if(candidato.vago == 0)
    {
      fclose(sistema);
      return (INICIO_INODES + i*TAMANHO_INODE);
    }
    i++;
  }

  fclose(sistema);
  return -1;
}

int procuraBlocoLivre()
{
  FILE * sistema = fopen("sistema.bin", "rb");
  char candidato;

  if(!sistema)
  {
    return -1;
  }

  int i = 1;
  fseek(sistema, INICIO_CONTROLE_BLOCOS+1, SEEK_SET);
  while(i < MAX_BLOCOS)
  {
    fread(&candidato, sizeof(char), 1, sistema);
    if(candidato == 0)
    {
      return(i);
      fclose(sistema);
    }else{
      i++;
    }
  }
}

int sinalizaBloco(int index)
{
  FILE * sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    return -1;
  }
  fseek(sistema, INICIO_CONTROLE_BLOCOS + index, SEEK_SET);
  char flag = 1;
  fwrite(&flag, sizeof(char), 1, sistema);
  fclose(sistema);
  return 0;
}

int buscaInodePorNome(char* nome)
{
  FILE * sistema = fopen("sistema.bin", "rb");
  fseek(sistema, INICIO_INODES, SEEK_SET);
  int i = 0;
  while(i < MAX_INODES)
  {
    inode candidato;
    fread(&candidato, sizeof(inode), 1, sistema);
    if(strcmp(nome, candidato.nome) == 0)
    {
      return (INICIO_INODES + i*TAMANHO_INODE);
    }
  }
  return -1;
}
