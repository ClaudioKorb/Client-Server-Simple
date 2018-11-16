#ifndef DEFINICOES_H
#define DEFINICOES_H

#define INICIO_INODES             200
#define FIM_INODES                8600
#define MAX_INODES                200

#define INICIO_CONTROLE_BLOCOS    0
#define FIM_CONTROLE_BLOCOS       200

#define INICIO_DADOS              8600
#define FIM_DADOS                 9400
#define MAX_BLOCOS                200

int count_inodes = 0;
int count_blocos = 0;


typedef struct bloco
{
  char conteudo[4];
}bloco;

typedef struct inode
{
  char vago;
  char tipo;                 //0 = diretorio        1 = arquivo
  char nome[20];
  int numBlocos;
  int blocos[20];
}inode;

#define TAMANHO_INODE             sizeof(inode)
#define TAMANHO_BLOCO             sizeof(bloco)

#endif
