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
 *   -> Executar: ./main arg0 arg1 arg2 arg3 arg4 arg5 arg6
 *
 * - Sem make:
 *   -> Compilar: gcc main.c -pthread -o main
 *   -> Executar: ./main arg0 arg1 arg2 arg3 arg4 arg5 arg6
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Definindo constantes e os ranks das threads.
#define TRUE 1
#define FALSE 0
#define BUFFER_PEDIDOS 10

typedef struct pedido_t {
  int finalizado;            // Indica se o pedido foi finalizado
int requisitado;              // Quantidade de canetas requisitadas
int recebido;                 // Quantidade de canetas recebidas
struct tm *hora_requisicao;   // Hora em que o pedido foi feito
} pedido_t;

// Argumentos de entrada, variáveis globais
int qtde_max_materia_prima;     // Quantidade máxima de matéria-prima disponível
int qtde_unidades_envio;        // Quantidade de unidades enviadas por interação do depósito de matéria-prima
int tempo_envio_mp;             // Tempo entre cada envio de matéria-prima
int tempo_fabricacao;           // Tempo de fabricação de cada caneta
int capacidade_maxima_deposito; // Capacidade máxima do depósito de canetas
int qtde_max_canetas_compra;    // Quantidade máxima de canetas a serem compradas por pedido
int tempo_espera_compra;        // Tempo de espera entre cada compra

// Variáveis de condição e de sincronização
pthread_mutex_t mut_slots_disponiveis, mut_canetas_a_fabricar, mut_pedidos,
    mut_materia_prima;
pthread_cond_t cond_dep_canetas_controle;
sem_t sem_pedido_caneta, sem_pedido_atendido, sem_compra_realizada,
    sem_materia_prima;
int slots_disponiveis, canetas_a_fabricar, materia_prima_enviada,
    qtde_materia_prima_disponivel;
pedido_t pedidos[BUFFER_PEDIDOS]; // Buffer para armazernar os pedidos

// Rank 5 - Comprador
void *funcao_comprador(void *arg) {
  
  int canetas_compradas = 0; // Quantidade de canetas compradas
  int compra = 0;            // ID do pedido de compra

  while (TRUE) {
    // Solicita a compra da quantidade qtde_max_canetas_compra de canetas por interação até que tenha matéria prima suficiente,
    // quando a diferença entre os dois for menor que qtde_max_canetas_compra, o comprador solicita essa quantidade menor, a diferença
    pthread_mutex_lock(&mut_pedidos);
    int dif = qtde_max_materia_prima - canetas_compradas;
    if (dif > qtde_max_canetas_compra)
      pedidos[compra % BUFFER_PEDIDOS].requisitado = qtde_max_canetas_compra;
    else
      pedidos[compra % BUFFER_PEDIDOS].requisitado = dif;
    pedidos[compra % BUFFER_PEDIDOS].recebido = 0;
    pedidos[compra % BUFFER_PEDIDOS].finalizado = FALSE;
    time_t tempo_atual;
    time(&tempo_atual);
    pedidos[compra % BUFFER_PEDIDOS].hora_requisicao = localtime(&tempo_atual);
    pthread_mutex_unlock(&mut_pedidos);

    sem_post(&sem_pedido_caneta);
    sem_wait(&sem_pedido_atendido);

    pthread_mutex_lock(&mut_pedidos);
    pedidos[compra % BUFFER_PEDIDOS].finalizado = TRUE;
    canetas_compradas += pedidos[compra % BUFFER_PEDIDOS].recebido;

    compra++;
    pthread_mutex_unlock(&mut_pedidos);

    sem_post(&sem_compra_realizada);
    sleep(tempo_espera_compra);
  }

  return NULL;
}

// Rank 4 - Depósito de Canetas
void *funcao_deposito_canetas(void *arg) {

  int canetas_compradas = 0; // Quantidade de canetas compradas
  int compra = 0;            // ID do pedido de compra

  // Armazena as canetas e envia para o comprador a quantidade solicitada, se disponível, ou
  // a quantidade máx em estoque quando solicitado
  while (TRUE) {
    // Notifica a thread Controle.
    pthread_cond_signal(&cond_dep_canetas_controle);

    // Recebimento de pedidos e envio de canetas para a thread Comprador.
    sem_wait(&sem_pedido_caneta);

    pthread_mutex_lock(&mut_pedidos);
    pthread_mutex_lock(&mut_slots_disponiveis);
    int canetas_disponiveis = capacidade_maxima_deposito - slots_disponiveis;
    if (canetas_disponiveis >= pedidos[compra % BUFFER_PEDIDOS].requisitado) {
      pedidos[compra % BUFFER_PEDIDOS].recebido =
          pedidos[compra % BUFFER_PEDIDOS].requisitado;
    } else {
      pedidos[compra % BUFFER_PEDIDOS].recebido = canetas_disponiveis;
    }

    slots_disponiveis += pedidos[compra % BUFFER_PEDIDOS].recebido;
    canetas_compradas += pedidos[compra % BUFFER_PEDIDOS].recebido;
    compra++;
    pthread_mutex_unlock(&mut_slots_disponiveis);
    pthread_mutex_unlock(&mut_pedidos);

    sem_post(&sem_pedido_atendido);
  }
}

// Rank 3 - Controle
void *funcao_controle(void *arg) {
  // Se o Depósito de canetas tiver slots disponíveis, o controle libera o envio de matéria prima e a fabricação de canetas
  while (TRUE) {
    // Controle da fabrica de canetas
    pthread_mutex_lock(&mut_slots_disponiveis);
    while (slots_disponiveis == 0) {
      pthread_cond_wait(&cond_dep_canetas_controle, &mut_slots_disponiveis);
    }

    // Controlar materia do depósito
    sem_post(&sem_materia_prima);

    if (pthread_mutex_trylock(&mut_canetas_a_fabricar) == 0) {
      canetas_a_fabricar = slots_disponiveis;
      pthread_mutex_unlock(&mut_canetas_a_fabricar);
    }
    pthread_mutex_unlock(&mut_slots_disponiveis);
  }
}

// Rank 2 - Célula de Fabricação de Canetas
void *funcao_celula_fabricacao(void *arg) {
  int materia_prima = 0; // Matéria prima na Fabricação de canetas

  // Se possuir matéria prima em estoque e o controle sinalizou que tem canetas a produzir, ele produz as canetas
  while (TRUE) {
    // Receber a matéria prima
    pthread_mutex_lock(&mut_materia_prima);
    materia_prima += materia_prima_enviada;
    pthread_mutex_unlock(&mut_materia_prima);

    if (materia_prima > 0) {
      pthread_mutex_lock(&mut_canetas_a_fabricar);

      while (canetas_a_fabricar > 0 && materia_prima > 0) {
        sleep(tempo_fabricacao);
        materia_prima--; // Diminui em 1 a quantidade de matéria prima
        pthread_mutex_lock(&mut_slots_disponiveis);
        slots_disponiveis--; // Enviando 1 caneta ao depósito de 
        pthread_mutex_unlock(&mut_slots_disponiveis);
        canetas_a_fabricar--; // Diminui em 1 a quantidade de canetas a fabricar
      }
      pthread_mutex_unlock(&mut_canetas_a_fabricar);
    }
  }
}

// Rank 1 - Depósito de matéria prima
void *funcao_deposito_materia_prima(void *arg) {
  while (TRUE) {
    // Aguarda o Controle liberar o depósito de materia prima e envia a quantidade 
    // qtde_unidades_envio para a célula de fabricação de canetas cada vez que solicitado ou a quantidade restante disponível
    sem_wait(&sem_materia_prima);

    pthread_mutex_lock(&mut_materia_prima);
    if (qtde_materia_prima_disponivel == 0) {
      pthread_mutex_unlock(&mut_materia_prima);
      break;
    }
    if (qtde_materia_prima_disponivel >= qtde_unidades_envio) {
      materia_prima_enviada = qtde_unidades_envio;
      qtde_materia_prima_disponivel -= qtde_unidades_envio;
    } else {
      materia_prima_enviada = qtde_materia_prima_disponivel;
      qtde_materia_prima_disponivel -= qtde_materia_prima_disponivel;
    }
    pthread_mutex_unlock(&mut_materia_prima);

    sleep(tempo_envio_mp);
  }
  return NULL;
}

// Função do Criador
void criador() {
  // Cria as threads
  pthread_t deposito_caneta, deposito_mp, celula_fabricacao, controle,
      comprador;

  pthread_create(&deposito_caneta, NULL, funcao_deposito_canetas, NULL);
  pthread_create(&controle, NULL, funcao_controle, NULL);
  pthread_create(&deposito_mp, NULL, funcao_deposito_materia_prima, NULL);
  pthread_create(&celula_fabricacao, NULL, funcao_celula_fabricacao, NULL);
  pthread_create(&comprador, NULL, funcao_comprador, NULL);

  int canetas_compradas = 0;
  int compra = 0;

  // Enquanto a quantidade de canetas compradas for menor que a quantidade máx de matéria prima, exibe os pedidos realizados e as
  // quantidades pedidas e recebidas pelo comprador
  while (canetas_compradas < qtde_max_materia_prima) {

    sem_wait(&sem_compra_realizada);
    pthread_mutex_lock(&mut_pedidos);

    canetas_compradas += pedidos[compra % BUFFER_PEDIDOS].recebido;
    pedidos[compra % BUFFER_PEDIDOS].finalizado = FALSE;
    printf("[Pedido %03d - %02d:%02d:%02d] Canetas requisitadas: %04d "
           "\t|\tCanetas "
           "recebidas: %04d \t(%04d/%04d)\n",
           compra, pedidos[compra % BUFFER_PEDIDOS].hora_requisicao->tm_hour,
           pedidos[compra % BUFFER_PEDIDOS].hora_requisicao->tm_min,
           pedidos[compra % BUFFER_PEDIDOS].hora_requisicao->tm_sec,
           pedidos[compra % BUFFER_PEDIDOS].requisitado,
           pedidos[compra % BUFFER_PEDIDOS].recebido, canetas_compradas,
           qtde_max_materia_prima);
    compra++;

    pthread_mutex_unlock(&mut_pedidos);
    fflush(0);
  }
}

// Rank 0 - Criador
int main(int argc, char *argv[]) {
  // Recebe os argumentos de entrada
  if (argc == 8) {
    qtde_max_materia_prima = atoi(argv[1]);
    qtde_unidades_envio = atoi(argv[2]);
    tempo_envio_mp = atoi(argv[3]);
    tempo_fabricacao = atoi(argv[4]);
    capacidade_maxima_deposito = atoi(argv[5]);
    qtde_max_canetas_compra = atoi(argv[6]);
    tempo_espera_compra = atoi(argv[7]);

  } else {
    printf("Número invalido de argumentos.\n");
    exit(0);
  }

  // Inicialização dos semáforos e variáveis de condição.
  pthread_mutex_init(&mut_slots_disponiveis, NULL);
  pthread_mutex_init(&mut_canetas_a_fabricar, NULL);
  pthread_mutex_init(&mut_pedidos, NULL);
  pthread_mutex_init(&mut_materia_prima, NULL);
  sem_init(&sem_pedido_caneta, 0, 0);
  sem_init(&sem_pedido_atendido, 0, 0);
  sem_init(&sem_compra_realizada, 0, 0);
  sem_init(&sem_materia_prima, 0, 0);
  pthread_cond_init(&cond_dep_canetas_controle, NULL);
  
  slots_disponiveis = capacidade_maxima_deposito;
  canetas_a_fabricar = slots_disponiveis;
  qtde_materia_prima_disponivel = qtde_max_materia_prima;

  printf("Iniciando thread Criador\n");
  criador();
  printf("Encerrando thread Criador\n");
  return 0;
}
