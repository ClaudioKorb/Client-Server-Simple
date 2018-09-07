#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080


int main(int argc, char const *argv[])
{
  struct sockaddr_in adress;                            // ENDEREÇO DO CLIENTE
  int sock = 0, valread;                                //SOCKET PARA O CLIENTE
  struct sockaddr_in serv_addr;                         // ENDEREÇO DO SERVIDOR
  char *hello = "Hello from client";
  char buffer[1024] = {0};

  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo("127.0.0.1", "http", &hints, &res) != 0){
    printf("Nao foi possivel identificar o servidor\n");
    return -1;
  }


  if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)      // AF_INET = IPV4, SOCK_STREAM = TCP, 0 = PROTOLOCO IP
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  memset(&serv_addr, '0', sizeof(serv_addr));         // PREENCHE SERV_ADDR COM '0'

  serv_addr.sin_family = AF_INET;                    // FAMILIA IPV4
  serv_addr.sin_port = htons(PORT);                  // PORT DO SERVIDOR

  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)    // CONVERTENDO O IP DE 'CHAR' PARA IPV4
  {                                                                //pton = presentation to network
    printf("\nInvalid adress / Adress not supported \n");          // Retorna -1 em erro e 0 se o ip não está no formato certo
    return -1;
  }

  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) > 0)   //CONECTANDO COM O SERVIDOR
  {
    printf("\nConnection failed \n");
    return -1;
  }

  send(sock, hello, strlen(hello), 0);              //ENVIA mensagem
  printf("Hello message sent\n");
  valread = read(sock, buffer, 1024);               //LE MENSAGEM DE CHEGADA
  printf("%s\n", buffer);                           //IMPRIME MENSAGEM DE CHEGADA
  return 0;
}
