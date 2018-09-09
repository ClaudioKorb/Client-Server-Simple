/*
  PROGRAMA SERVIDOR PARA CONEXÃO SOCKETS EM SERVIDOR DE ARQUIVOS
  Programador: Claudio André Korb
  Instituição: Universidade Federal de Santa Catarina
  Disciplina: Sistemas Operacionais

  Big thanks to Brian "Beej Jorgensen" Hall for his amazing guide to Network Programming
  Available at: http://beej.us/guide/bgnet/html/multi/index.html
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 8080

int strcmpst1nl (const char * s1, const char * s2);

int main(int argc, char const * argv[])
{
  int server_fd, new_socket, valread;                                           //server_fd = servidor, valread = codigo de leitura
  struct sockaddr_in adress;                                                    //endereços
  int opt = 1;                                                                  //Usado para setsockopt()
  int addrlen = sizeof(adress);
  char buffer[1024] = {0};                                                      //Buffer onde será armazenada mensagem de entrada
  char *hello = "Hello from server!";                                           //Mensagem a ser enviada pelo servidor

  if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)                        // AF_INET = IPV4 --     SOCK_STREAM = TCP --    0 = PROTOCOLO IP
  {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))        //ETAPA OPCIONAL
  {
    perror("setsocketopt");
  }

  adress.sin_family      = AF_INET;                                             //FAMILIA DE ENDEREÇOS IPV4
  adress.sin_addr.s_addr = INADDR_ANY;                                          //IP DO SERVIDOR SERÁ LOCALHOST???
  adress.sin_port        = htons(PORT);                                         //PORTA DO SERVIDOR -> PORT = 8080    htons = host to network short
                                                                                //                                     (converte o método de guardar informação)
  if(bind(server_fd, (struct sockaddr *)&adress, sizeof(adress)) < 0)           // LIGA O SERVIDOR AO ENDEREÇO
  {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if(listen(server_fd, 3) < 0)                                                  // SERVIDOR ESPERA POR CONEXÕES, TAMANHO MÁXIMO DE FILA = 3
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  if((new_socket = accept(server_fd, (struct sockaddr *)&adress,
                          (socklen_t*)&addrlen)) < 0)                           //EM UMA CONEXÃO
  {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  char mensagem[1024];
  strcpy(mensagem, "Bem vindo ao servidor de arquivos!\nComandos: create -- Criar Arquivo / delete -- Deletar Arquivo \n");
  send(new_socket, mensagem, strlen(mensagem), 0);
//POSSIVEL CRIAR E DELETAR ARQUIVOS
  while(1)
  {
    valread = read(new_socket, buffer, 1024);
    if(strcmpst1nl(buffer, "create") == 0)
    {
      memset(mensagem, '\0', strlen(mensagem));                                 //LIMPANDO OS BUFFERS --- DESNECESSÁRIO??
      memset(buffer, '\0', strlen(buffer));                                     //LIMPANDO OS BUFFERS

      strcpy(mensagem, "Digite o nome do arquivo: ");
      send(new_socket, mensagem, strlen(mensagem), 0);

      if(valread = read(new_socket, buffer, 32) == -1){
        strcpy(mensagem, "Nome do arquivo invalido!");
        send(new_socket, mensagem, strlen(mensagem), 0);
        exit(0);
      }

      buffer[strlen(buffer)-1] = '\0';                                          //REMOVENDO ULTIMO CARACTERE
      char nome[40];
      snprintf(nome, sizeof(nome), "%s.txt", buffer);                           //ADICIONANDO A EXTENSAO .TXT AO NOME
      FILE* new_file = fopen(nome, "w");                                        //CRIANDO O ARQUIVO
      if(new_file == NULL){                                                     //CHECANDO ERRO
        strcpy(mensagem, "Falha ao criar arquivo!");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }else{
        strcpy(mensagem, "Arquivo criado com sucesso");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }
    }
    else if (strcmpst1nl(buffer, "delete") == 0)
    {
      strcpy(mensagem, "Digite o nome do arquivo a ser deletado: ");
      send(new_socket, mensagem, strlen(mensagem), 0);
      if(valread = read(new_socket, buffer, 32) == -1){
        strcpy(mensagem, "Nome do arquivo invalido!");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }
      buffer[strlen(buffer)-1] = '\0';                                          //DELETANDO O ULTIMO CARACTERE
      char nome[40];
      snprintf(nome, sizeof(nome), "%s.txt", buffer);                           //INSERINDO A EXTENSAO .TXT AO NOME
      if(remove(nome) == -1){                                                   //REMOVENDO O ARQUIVO
        strcpy(mensagem, "Falha ao deletar arquivo");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }else{
        strcpy(mensagem, "Arquivo excluido com sucesso");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }
    }
  }
  return 0;

}

int strcmpst1nl (const char * s1, const char * s2)
{
  char s1c;
  do
    {
      s1c = *s1;
      if (s1c == '\n')
          s1c = 0;
      if (s1c != *s2)
          return 1;
      s1++;
      s2++;
    } while (s1c); /* already checked *s2 is equal */
  return 0;
}
