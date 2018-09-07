#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 8080

int main(int argc, char const * argv[])
{
  int server_fd, new_socket, valread;         //server_fd = servidor, valread = buffer de entrada
  struct sockaddr_in adress;                  //endereços
  int opt = 1;                                //Usado para setsockopt()
  int addrlen = sizeof(adress);
  char buffer[1024] = {0};                    //Buffer onde será armazenada mensagem de entrada
  char *hello = "Hello from server!";         //Mensagem a ser enviada pelo servidor

  if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)    // AF_INET = IPV4 --     SOCK_STREAM = TCP --    0 = PROTOCOLO IP
  {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))   //ETAPA OPCIONAL
  {
    perror("setsocketopt");
  }

  adress.sin_family      = AF_INET;             //FAMILIA DE ENDEREÇOS IPV4
  adress.sin_addr.s_addr = INADDR_ANY;          //IP DO SERVIDOR SERÁ LOCALHOST???
  adress.sin_port        = htons(PORT);         //PORTA DO SERVIDOR -> PORT = 8080    htons = host to network short
                                                //                                     (converte o método de guardar informação)
  if(bind(server_fd, (struct sockaddr *)&adress, sizeof(adress)) < 0)     // LIGA O SERVIDOR AO ENDEREÇO
  {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if(listen(server_fd, 3) < 0)              // SERVIDOR ESPERA POR CONEXÕES, TAMANHO MÁXIMO DE FILA = 3
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  if((new_socket = accept(server_fd, (struct sockaddr *)&adress, (socklen_t*)&addrlen)) < 0)    //EM UMA CONEXÃO
  {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  valread = read(new_socket, buffer, 1024);       // LE A MENSAGEM DO CLIENTE
  printf("%s\n", buffer);                         // ESCREVE A MENSAGEM DO CLIENTE
  send(new_socket, hello, strlen(hello), 0);      // ENVIA A MENSAGEM AO CLIENTE
  printf("Hello message sent\n");
  return 0;

}
