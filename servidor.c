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
int trocaDiretorio(int new_socket, char *diretorio);
int removeArquivo(int new_socket, char* diretorio);
int listaConteudo(int new_socket, char* diretorio);
int escreveNoArquivo(int new_socket, char* diretorio);
int mostraConteudo(int new_socket, char* diretorio);

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
  inode inodeRoot = {};
  //memset(inodeRoot, 0, sizeof(inodeRoot));
  memset(inodeRoot.nome, 0, sizeof(inodeRoot.nome));
  strcpy(inodeRoot.nome, "root");
  inodeRoot.tipo = 0;
  inodeRoot.vago = 1;
  inodeRoot.pai = -1;
  inodeRoot.bloco = INICIO_DADOS;

  //ESCREVENDO O BLOCO COMO OCUPADO NA LISTA DE CONTROLE
  sinalizaBloco(0);
  int temp = -1;
  //ESCREVENDO O INODE DA ROOT NO ARQUIVO
  fseek(sistema_arq, INICIO_INODES, SEEK_SET);
  fwrite(&inodeRoot, sizeof(inodeRoot), 1, sistema_arq);
  fseek(sistema_arq, INICIO_DADOS, SEEK_SET);
  fwrite(&temp, sizeof(int), 1, sistema_arq);
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

  strcpy(mensagem, "\nBem vindo ao servidor de arquivos!\nComandos: \ncreate -- Criar Arquivo \ndelete -- Deletar Arquivo \nwrite -- Escrever no Arquivo\nread -- Mostrar Conteudo do Arquivo\nmkdir -- Criar Diretorio \n \ndir -- Mostrar conteudo do diretorio\ncd -- Trocar de Diretorio\nhelp -- Comandos \nclose -- Encerrar conexao\n");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

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
      }else{
        if(strcmpst1nl(opt, "cd") == 0)
        {
          trocaDiretorio(new_socket, diretorioAtual);
        }else{
          if(strcmpst1nl(opt, "delete") == 0)
          {
            removeArquivo(new_socket, diretorioAtual);
          }else{
            if(strcmpst1nl(opt, "dir") == 0)
            {
              listaConteudo(new_socket, diretorioAtual);
            }else{
              if(strcmpst1nl(opt, "write") == 0)
              {
                escreveNoArquivo(new_socket, diretorioAtual);
              }else{
                if(strcmpst1nl(opt, "read") == 0)
                {
                  mostraConteudo(new_socket, diretorioAtual);
                }else{
                  if(strcmpst1nl(opt, "help") == 0)
                  {
                    int ajuda(int new_socket);
                  }else{
                    strcpy(mensagem, "Comando invalido!");
                    send(new_socket, mensagem, strlen(mensagem), 0);
                    memset(mensagem, 0, sizeof(mensagem));
                  }
                }
              }
            }
          }
        }
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

  //PROCURA O DIRETORIO CORRENTE NO ARQUIVO
  int pos = buscaInodePorNome(diretorio);
  if(pos == -1)
  {
    strcpy(mensagem, "Erro: diretorio raiz invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -3;
  }

  inode novoDiretorio;
  int novoBloco = procuraBlocoLivre();
  novoDiretorio.bloco = INICIO_DADOS + novoBloco*TAMANHO_BLOCO;
  novoDiretorio.tipo = 0;
  novoDiretorio.vago = 1;
  novoDiretorio.pai = pos;
  int temp = -1;
  buffer[strlen(buffer)-1] = '\0';
  memset(novoDiretorio.nome, 0, sizeof(novoDiretorio.nome));
  strcpy(novoDiretorio.nome, buffer);
  sinalizaBloco(novoBloco);
  //PROCURANDO INODE LIVRE
  int posicao = procuraInodeLivre();

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
  fseek(sistema, novoDiretorio.bloco, SEEK_SET);
  fwrite(&temp, sizeof(int), 1 ,sistema);
//=========ADICIONANDO O INODE AO DIRETORIO ONDE FOI CRIADO===============================

  //PROCURA O INODE DO DIRETORIO ATUAL

  fseek(sistema, pos, SEEK_SET);
  inode diretorioAtual;
  bloco blocoAtual;
  fread(&diretorioAtual, sizeof(inode), 1, sistema);
  //PROCURA O BLOCO DE MEMORIA DO DIRETORIO ATUAL
  fseek(sistema, diretorioAtual.bloco, SEEK_SET);
  int i = 0;
  while(i < 50)
  {
    int candidato;
    int ultimo = -1;
    fread(&candidato, sizeof(int), 1, sistema);
    if(candidato == ultimo)
    {
      fseek(sistema, diretorioAtual.bloco + i*sizeof(int), SEEK_SET);
      fwrite(&posicao, sizeof(int), 1, sistema);
      fwrite(&ultimo, sizeof(int), 1, sistema);
      break;
    }
    i++;
  }
//===============================================================================

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
  //LENDO O NOME DO ARQUIVO DO CLIENTE=====================
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
  //=========================================================
  //PROCURANDO INODE DO DIRETORIO ATUAL NO ARQUIVO
  int pos = buscaInodePorNome(diretorio);
  if(pos == -1)
  {
    strcpy(mensagem, "Erro ao criar arquivo: Diretorio invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 4;
  }
  //CRIANDO INODE PRO NOVO ARQUIVO
  inode novoArquivo;
  memset(novoArquivo.nome, 0, sizeof(novoArquivo.nome));
  novoArquivo.tipo = 1;
  novoArquivo.vago = 1;
  novoArquivo.pai = pos;
  buffer[strlen(buffer)-1] = '\0';
  strcpy(novoArquivo.nome, buffer);
  //PROCURANDO UM BLOCO DE MEMORIA LIVRE
  int novoBloco = procuraBlocoLivre();
  novoArquivo.bloco = INICIO_DADOS + novoBloco*TAMANHO_BLOCO;
  sinalizaBloco(novoBloco);

  //PROCURA UM INODE LIVRE
  int posicao = procuraInodeLivre();
  if(posicao == -1)
  {
    strcpy(mensagem, "Nao foi possivel encontrar inode livre!");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 1;
  }

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
  sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    strcpy(mensagem, "Erro ao criar arquivo");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return 5;
  }

  //LE O INODE DO DIRETORIO ATUAL
  fseek(sistema, pos, SEEK_SET);
  inode diretorioAtual;
  bloco blocoAtual;
  fread(&diretorioAtual, sizeof(inode), 1, sistema);
  //PROCURA O BLOCO DE MEMORIA DO DIRETORIO ATUAL
  fseek(sistema, diretorioAtual.bloco, SEEK_SET);
  int i = 0;
  //ESCREVE O NOVO ARQUIVO NA ULTIMA POSICAO
    //A ULTIMA POSICAO É REPRESENTADA POR FF FF FF FF (-1)
  while(i < 50)
  {
    int candidato;
    int ultimo = -1;
    fread(&candidato, sizeof(int), 1, sistema);
    if(candidato == ultimo)
    {
      fseek(sistema, diretorioAtual.bloco + i*sizeof(int), SEEK_SET);
      fwrite(&posicao, sizeof(int), 1, sistema);
      fwrite(&ultimo, sizeof(int), 1, sistema);
      break;
    }
    i++;
  }
  strcpy(mensagem, "Arquivo criado com sucesso");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  fclose(sistema);

  return 0;
}

int removeArquivo(int new_socket, char* diretorio)
{
  FILE * sistema;
  char buffer[1024] = "";
  char mensagem[1024] = "";
  int valread;
  //RECEBE O NOME DO ARQUIVO DO CLIENTE
  strcpy(mensagem, "Digite o nome do arquivo");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  if(valread = read(new_socket, buffer, 1024) == -1)
  {
    strcpy(mensagem, "Nome invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
  }

//========= REMOVENDO A REFERENCIA DO ARQUIVO NO DIRETORIO =====================
  int posicao_diretorio_raiz = buscaInodePorNome(diretorio);
  inode diretorioRaiz;
  sistema = fopen("sistema.bin", "r+b");
  //LE O DIRETORIO ATUAL
  fseek(sistema, posicao_diretorio_raiz, SEEK_SET);
  fread(&diretorioRaiz, sizeof(inode), 1, sistema);
  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);

  int count = 0;
  int node = 0;
  int i = 0;
  inode candidato;
  //SALVANDO OS INODES DO DIRETORIO ATUAL EM UM VETOR---------------------------
  while(node != -1)
  {
    fread(&node, sizeof(int), 1, sistema);
    count++;
  }
  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);
  int *nodes = (int*)malloc(count*sizeof(int));
  i = 0;
  while(i < count)
  {
    fread(&node, sizeof(int), 1, sistema);
    nodes[i] = node;
    i++;
  }
  //----------------------------------------------------------------------------
  //----------- PROCURA A REFERENCIA DO INODE NO DIRETORIO ---------------------
  i = 0;
  while(i < count)
  {
    fseek(sistema, nodes[i], SEEK_SET);
    fread(&candidato, sizeof(inode), 1, sistema);
    if(strcmpst1nl(buffer, candidato.nome) == 0)
    {
      break;
    }else{
      i++;
    }
  }
  int posicao_candidato = nodes[i];
  //----------------------------------------------------------------------------
  //-------- REMOVE A REFERENCIA E 'PUXA' TODOS OS OUTROS PRA FRENTE -----------
  int j = i;
  while(j <= count - 2)
  {
    nodes[j] = nodes[j+1];
    j++;
  }
  nodes[count-1] = 0;
  //----------------------------------------------------------------------------
  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);
  fwrite(nodes, sizeof(nodes), 1, sistema);
  free(nodes);
//==============================================================================

  //------------MARCANDO O BLOCO COMO LIBERADO NO MAPA DE BITS------------------
  int zeros[50] = {};
  char zero = 0;
  int byte = (candidato.bloco - INICIO_DADOS)/TAMANHO_BLOCO;
  fseek(sistema, candidato.bloco, SEEK_SET);
  fwrite(zeros, sizeof(zeros), 1, sistema);
  fseek(sistema, byte, SEEK_SET);
  fwrite(&zero, sizeof(zero), 1, sistema);
  //----------------------------------------------------------------------------
  //-------------------------REMOVENDO O INODE----------------------------------
  fseek(sistema, posicao_candidato, SEEK_SET);
  fread(&candidato, sizeof(inode), 1, sistema);

  candidato.vago = 0;

  fseek(sistema, posicao_candidato, SEEK_SET);
  fwrite(&candidato.vago, sizeof(char), 1, sistema);

  strcpy(mensagem, "Arquivo removido com sucesso");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));
  fclose(sistema);

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
    i++;
  }
  return -1;
}

int trocaDiretorio(int new_socket, char *diretorio)
{
  FILE * sistema = fopen("sistema.bin", "r+b");
  char mensagem[1024] = "";
  char buffer[1024] = "";
  int valread;
  //RECEBE O NOME DO DIRETORIO DO CLIENTE
  strcpy(mensagem, "Digite o nome do diretorio");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));
  if(valread = read(new_socket, buffer, 20) == -1)
  {
    strcpy(mensagem, "Nome invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
  }

  //PROCURA O DIRETORIO ATUAL NO ARQUIVO
  int posicao_diretorio_raiz = buscaInodePorNome(diretorio);
  inode diretorioAtual;
  fseek(sistema, posicao_diretorio_raiz, SEEK_SET);
  fread(&diretorioAtual, sizeof(inode), 1, sistema);

  //CASO O USUARIO QUEIRA VOLTAR
  if(strcmpst1nl(buffer, "..") == 0)
  {
    int numero_pai = diretorioAtual.pai;
    //O DIRETORIO 'ROOT' NAO TEM PAI
    if(numero_pai == -1)
    {
      strcpy(mensagem, "Diretorio destino nao existe");
      send(new_socket, mensagem, strlen(mensagem), 0);
      memset(mensagem, 0, sizeof(mensagem));
      return -1;
    }else{
      //VOLTA PARA O DIRETORIO PAI
      inode pai;
      fseek(sistema, numero_pai, SEEK_SET);
      fread(&pai, sizeof(inode), 1, sistema);
      snprintf(mensagem, sizeof(mensagem), "Entrou no diretorio %s", pai.nome);
      send(new_socket, mensagem, strlen(mensagem), 0);
      memset(mensagem, 0, sizeof(mensagem));
      //ALTERA A VARIAVEL DIRETORIO
      memset(diretorio, 0, sizeof(diretorio));
      strcpy(diretorio, pai.nome);
      return 0;
    }
  }

  //==========PROCURA O DIRETORIO DE DESTINO NO DIRETORIO ATUAL=================
  inode candidato;
  int posicao_candidato;

  fseek(sistema, diretorioAtual.bloco, SEEK_SET);
  fread(&posicao_candidato, sizeof(int), 1, sistema);
  int i = 0;

  while(posicao_candidato != -1)
  {
    fseek(sistema, posicao_candidato, SEEK_SET);
    fread(&candidato, sizeof(inode), 1, sistema);
    if(strcmpst1nl(buffer, candidato.nome) == 0)
    {
      snprintf(mensagem, sizeof(mensagem), "Entrou no diretorio %s", candidato.nome);
      send(new_socket, mensagem, strlen(mensagem), 0);
      memset(mensagem, 0, sizeof(mensagem));
      //ALTERA A VARIAVEL 'DIRETORIO'
      memset(diretorio, 0, sizeof(diretorio));
      strcpy(diretorio, candidato.nome);
      return 0;
    }
    i++;
    fseek(sistema, diretorioAtual.bloco + i*4, SEEK_SET);
    fread(&posicao_candidato, sizeof(int), 1, sistema);
  }
  strcpy(mensagem, "Diretorio nao existe");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));
  return -2;
}

int listaConteudo(int new_socket, char* diretorio)
{
  FILE * sistema;
  char mensagem[1024] = "";
  char buffer[1024] = "";
  int valread;
  //PROCURA O DIRETORIO ATUAL NO ARQUIVO
  inode diretorioAtual;
  int local_diretorioAtual = buscaInodePorNome(diretorio);
  sistema = fopen("sistema.bin", "rb");
  fseek(sistema, local_diretorioAtual, SEEK_SET);
  fread(&diretorioAtual, sizeof(inode), 1, sistema);

  //------------------ESCREVE OS INODES DO DIRETORIO EM UM VETOR----------------
  int* conteudo;
  int count = 0;
  int node;
  fseek(sistema, diretorioAtual.bloco, SEEK_SET);
  while(node != -1)
  {
    fread(&node, sizeof(int), 1, sistema);
    count++;
  }
  conteudo = (int*)malloc(count*sizeof(int));
  int i;
  fseek(sistema, diretorioAtual.bloco, SEEK_SET);

  while(i < count)
  {
    fread(&conteudo[i], sizeof(int), 1, sistema);
    i++;
  }
  //----------------------------------------------------------------------------
  i = 0;
  inode atual;
  //------------ COPIA OS CONTEUDOS DO DIRETORIO PRA MENSAGEM ------------------
  while(i < count)
  {
    fseek(sistema, conteudo[i], SEEK_SET);
    fread(&atual, sizeof(inode), 1, sistema);
    char aux[21];
    snprintf(aux, sizeof(aux), "%s\n", atual.nome);
    strcat(mensagem, aux);
    i++;
  }
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));
  //----------------------------------------------------------------------------
  fclose(sistema);
}

int escreveNoArquivo(int new_socket, char* diretorio)
{
  FILE * sistema;
  char mensagem[1024] = "";
  char buffer[1024] = "";
  int valread;
  //===== RECEBE O NOME DO CLIENTE ================
  strcpy(mensagem, "Digite o nome do arquivo");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  if(valread = read(new_socket, buffer, 20) == 0)
  {
    strcpy(mensagem, "Nome invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -1;
  }
  //=================================================
  sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    strcpy(mensagem, "Erro ao abrir arquivo");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -2;
  }
  //------------------ PROCURA O DIRETORIO ATUAL NO ARQUIVO --------------------
  int posicao_diretorio_raiz;
  inode diretorioRaiz;

  posicao_diretorio_raiz = buscaInodePorNome(diretorio);

  fseek(sistema, posicao_diretorio_raiz, SEEK_SET);
  fread(&diretorioRaiz, sizeof(inode), 1, sistema);

  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);
  //----------------------------------------------------------------------------
  //--------------- ESCREVE OS INODES DO DIRETORIO EM UM VETOR------------------
  int node;
  int * conteudo;
  int count = 0;
  while(node != -1)
  {
    fread(&node, sizeof(int), 1, sistema);
    count++;
  }
  conteudo = (int*)malloc(count*sizeof(int));
  int i;
  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);
  while(i < count)
  {
    fread(&conteudo[i], sizeof(int), 1, sistema);
    i++;
  }
  i = 0;
  //----------------------------------------------------------------------------
  //------------------------ PROCURA O ARQUIVO NOS INODES ----------------------
  inode candidato;
  while(i < count)
  {
    fseek(sistema, conteudo[i], SEEK_SET);
    fread(&candidato, sizeof(inode), 1, sistema);
    if(strcmpst1nl(buffer, candidato.nome) == 0)
    {
      break;
    }else{
      i++;
    }
  }
  if(i == count)
  {
    strcpy(mensagem, "Arquivo inexistente");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -3;
  }
  fseek(sistema, candidato.bloco, SEEK_SET);
  //----------------------------------------------------------------------------
  //------------------ RECEBE O CONTEUDO DO CLIENTE ----------------------------
  strcpy(mensagem, "O que deseja escrever?");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));
  memset(buffer, 0, sizeof(buffer));

  if(valread = read(new_socket, buffer, 50) == 0)
  {
    strcpy(mensagem, "Conteudo invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -4;
  }
  //----------------------------------------------------------------------------
  //------------------------ ESCREVE O CONTEUDO NO ARQUIVO ---------------------
  buffer[strlen(buffer) - 1] = '\0';
  char str[TAMANHO_BLOCO] = "";
  strcpy(str, buffer);

  fwrite(&str, sizeof(str), 1, sistema);
  strcpy(mensagem, "Conteudo escrito");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  fclose(sistema);

  return 0;
}

int mostraConteudo(int new_socket, char* diretorio)
{
  FILE * sistema;
  char mensagem[1024] = "";
  char buffer[1024] = "";
  int valread;
  //============ RECEBE O NOME DO ARQUIVO DO CLIENTE ==========================
  strcpy(mensagem, "Digite o nome do arquivo");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  if(valread = read(new_socket, buffer, 20) == 0)
  {
    strcpy(mensagem, "Nome invalido");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -1;
  }
  //===========================================================================

  sistema = fopen("sistema.bin", "r+b");
  if(!sistema)
  {
    strcpy(mensagem, "Erro ao abrir arquivo");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -2;
  }

  //---------------------- PROCURA O DIRETORIO ATUAL ---------------------------
  int posicao_diretorio_raiz;
  inode diretorioRaiz;

  posicao_diretorio_raiz = buscaInodePorNome(diretorio);

  fseek(sistema, posicao_diretorio_raiz, SEEK_SET);
  fread(&diretorioRaiz, sizeof(inode), 1, sistema);

  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);
  // ----------------- SALVA OS INODES DO DIRETORIO EM UM VETOR ----------------
  int node;
  int * conteudo;
  int count = 0;
  while(node != -1)
  {
    fread(&node, sizeof(int), 1, sistema);
    count++;
  }
  conteudo = (int*)malloc(count*sizeof(int));
  int i;
  fseek(sistema, diretorioRaiz.bloco, SEEK_SET);
  while(i < count)
  {
    fread(&conteudo[i], sizeof(int), 1, sistema);
    i++;
  }
  i = 0;
  // ---------------------------------------------------------------------------
  //----------- PROCURA O INODE DO ARQUIVO NOS INODES DO DIRETORIO -------------
  inode candidato;
  while(i < count)
  {
    fseek(sistema, conteudo[i], SEEK_SET);
    fread(&candidato, sizeof(inode), 1, sistema);
    if(strcmpst1nl(buffer, candidato.nome) == 0)
    {
      break;
    }else{
      i++;
    }
  }
  if(i == count)
  {
    strcpy(mensagem, "Arquivo inexistente");
    send(new_socket, mensagem, strlen(mensagem), 0);
    memset(mensagem, 0, sizeof(mensagem));
    return -3;
  }
  //----------------------------------------------------------------------------
  //--------------------- PROCURA O BLOCO DO ARQUIVO ---------------------------
  fseek(sistema, candidato.bloco, SEEK_SET);
  //-------------------- LENDO AS INFORMACOES DO BLOCO -------------------------
  fread(&mensagem, TAMANHO_BLOCO, 1, sistema);
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof(mensagem));

  fclose(sistema);
}

int ajuda(int new_socket)
{
  char mensagem[1024] = "";
  strcpy(mensagem, "\nBem vindo ao servidor de arquivos!\nComandos: \ncreate -- Criar Arquivo \ndelete -- Deletar Arquivo \nwrite -- Escrever no Arquivo\nread -- Mostrar Conteudo do Arquivo\nmkdir -- Criar Diretorio \n \ndir -- Mostrar conteudo do diretorio\ncd -- Trocar de Diretorio\nhelp -- Comandos \nclose -- Encerrar conexao\n");
  send(new_socket, mensagem, strlen(mensagem), 0);
  return 0;
}
