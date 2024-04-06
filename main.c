/*
* Trabalho 01 - Sistemas Operacionais I
* Grupo 4
*
* Integrantes:
* - Arthur Santos Braga             nUSP: 13677156
* - Karine Cerqueira Nascimento     nUSP: 13718404 
* - Letícia Crepaldi da Cunha       nUSP: 11800879
* - Lucas Carvalho Freiberger Stapf nUSP: 11800559
* - Viktor Sérgio Ferreira          nUSP: 11800570
*
* Compilação/Execução
*
* - Makefile:
*   -> Compilar: make all
*   -> Executar: ./main arg0 arg1 arg2 arg3 arg4 arg5 arg6 arg7
*
* - Sem make:
*   -> Compilar: gcc main.c -pthread -o main
*   -> Executar: ./main arg0 arg1 arg2 arg3 arg4 arg5 arg6 arg7
*/

#include <stdio.h>
#include <stdlib.h>

int qte_materia_prima, qte_unidades_enviadas, tempo_envio, tempo_fabricacao, qte_max_armazenamento, qte_compra, delay_compras;

// ======================= Rank 0 ======================= //

int main(int argc, char *argv[]) {

  if (argc == 8) {

    qte_materia_prima = atoi(argv[1]);
    qte_unidades_enviadas = atoi(argv[2]);
    tempo_envio = atoi(argv[3]);
    tempo_fabricacao = atoi(argv[4]);
    qte_max_armazenamento = atoi(argv[5]);
    qte_compra = atoi(argv[6]);
    delay_compras = atoi(argv[7]);

  } else {

    printf("Numero invalido de argumentos.\n");
    exit(0);

  }

  return EXIT_SUCCESS;
}


// ======================= Rank 1 ======================= //

// ======================= Rank 2 ======================= //

// ======================= Rank 3 ======================= //

// ======================= Rank 4 ======================= //

// ======================= Rank 5 ======================= //







