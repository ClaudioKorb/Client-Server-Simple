#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8080


int main(int argc, char const *argv[])
{
  struct sockaddr_in adress;                            // ENDEREÇO DO CLIENTE
  int sock = 0, valread;                                //SOCKET PARA O CLIENTE
  struct sockaddr_in serv_addr;                         // ENDEREÇO DO SERVIDOR
  char *hello = "Hello from client";
  char buffer[1024] = {0};

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)      // AF_INET = IPV4, SOCK_STREAM = TCP, 0 = PROTOLOCO IP
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  memset(&serv_addr, '0', sizeof(serv_addr));         // PREENCHE SERV_ADDR COM '0'

  serv_addr.sin_family = AF_INET;                    // FAMILIA IPV4
  serv_addr.sin_port = htons(PORT);                  // PORT DO SERVIDOR

  if(inet_pton(AF_INET, "10.0.0.102", &serv_addr.sin_addr) <= 0)    // CONVERTENDO O IP DE 'CHAR' PARA IPV4
  {
    printf("\nInvalid adress / Adress not supported \n");
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
