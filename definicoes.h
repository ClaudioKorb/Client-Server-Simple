#ifndef DEFINICOES_H
#define DEFINICOES_H

#define INICIO_INODES             200
#define FIM_INODES                5000
#define MAX_INODES                200
#define TAMANHO_INODE             32

#define INICIO_CONTROLE_BLOCOS    0
#define FIM_CONTROLE_BLOCOS       200

#define INICIO_DADOS              5000
#define FIM_DADOS                 15000
#define MAX_BLOCOS                200
#define TAMANHO_BLOCO             50

int count_inodes = 0;
int count_blocos = 0;

typedef struct bloco
{
  char conteudo[50];
}bloco;

typedef struct inode
{
  char vago;
  char tipo;                 //0 = diretorio        1 = arquivo
  int pai;
  char nome[20];
  int bloco;
}inode;

#endif
