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
  int server_fd, new_socket, valread;                                           //server_fd = servidor, valread = buffer de entrada
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

  while(1)
  {
    valread = read(new_socket, buffer, 1024);
    char mensagem[1024];
    if(strcmpst1nl(buffer, "create") == 0)
    {
      strcpy(mensagem, "Voce deseja criar um arquivo");
      printf("USUARIO QUER CRIAR UM ARQUIVO\n");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else if (strcmpst1nl(buffer, "delete") == 0)
    {
      strcpy(mensagem, "Voce deseja deletar um arquivo");
      printf("USUARIO QUER DELETAR UM ARQUIVO\n");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else if(strcmpst1nl(buffer, "exit") == 0)
    {
      printf("USUARIO SAIU\n");
      strcpy(mensagem, "exit");
      send(new_socket, mensagem, strlen(mensagem), 0);
      close(new_socket);
      exit(0);
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
