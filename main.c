#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_N 20
#define MAX_THREADS MAX_N

// Estrutura para armazenar informações sobre cada posição no trajeto de uma thread
typedef struct {
    int x;
    int y;
    int tempo; // Tempo mínimo de permanência em décimos de segundo
} Posicao;

// Estrutura para armazenar informações sobre o trajeto de uma thread
typedef struct {
    int id;
    int grupo;
    int num_posicoes;
    Posicao *trajeto;
} Trajeto;

// Variáveis globais
pthread_mutex_t mutex_grade[MAX_N][MAX_N];
pthread_cond_t cond_grade[MAX_N][MAX_N];
bool grade_ocupada[MAX_N][MAX_N] = {false};

/*********************************************************
 Inclua o código a seguir no seu programa, sem alterações.
 Dessa forma a saída automaticamente estará no formato esperado 
 pelo sistema de correção automática.
 *********************************************************/

void passa_tempo(int tid, int x, int y, int decimos)
{
    struct timespec zzz, agora;
    static struct timespec inicio = {0,0};
    int tstamp;

    if ((inicio.tv_sec == 0)&&(inicio.tv_nsec == 0)) {
        clock_gettime(CLOCK_REALTIME,&inicio);
    } 

    zzz.tv_sec  = decimos/10;
    zzz.tv_nsec = (decimos%10) * 100L * 1000000L; 

    clock_gettime(CLOCK_REALTIME,&agora);
    tstamp = ( 10 * agora.tv_sec  +  agora.tv_nsec / 100000000L )
            -( 10 * inicio.tv_sec + inicio.tv_nsec / 100000000L );

    printf("%3d [ %2d @(%2d,%2d) z%4d\n",tstamp,tid,x,y,decimos);

    nanosleep(&zzz,NULL);

    clock_gettime(CLOCK_REALTIME,&agora);
    tstamp = ( 10 * agora.tv_sec  +  agora.tv_nsec / 100000000L )
            -( 10 * inicio.tv_sec + inicio.tv_nsec / 100000000L );

    printf("%3d ) %2d @(%2d,%2d) z%4d\n",tstamp,tid,x,y,decimos);
}
/*********************** FIM DA FUNÇÃO *************************/

// Função para entrar em uma posição
void entra(int x, int y, int tid) {
    pthread_mutex_lock(&mutex_grade[x][y]);
    while (grade_ocupada[x][y]) {
        pthread_cond_wait(&cond_grade[x][y], &mutex_grade[x][y]);
    }
    grade_ocupada[x][y] = true; // Marca a posição como ocupada pela thread
    pthread_mutex_unlock(&mutex_grade[x][y]);
}

// Função para sair de uma posição
void sai(int x, int y) {
    pthread_mutex_lock(&mutex_grade[x][y]);
    grade_ocupada[x][y] = false; // Libera a posição
    pthread_cond_broadcast(&cond_grade[x][y]); // Libera as threads bloqueadas aguardando a posição
    pthread_mutex_unlock(&mutex_grade[x][y]);
}

// Função para criação e execução das threads
void *thread_function(void *arg) {
    Trajeto *trajeto = (Trajeto *)arg;

    // Inicializa a posição anterior como a primeira posição do trajeto
    Posicao pos_anterior = trajeto->trajeto[0];

    // Percorre o trajeto da thread
    for (int i = 0; i < trajeto->num_posicoes; i++) {
        Posicao pos_atual = trajeto->trajeto[i];

        // Entra na posição atual
        entra(pos_atual.x, pos_atual.y, trajeto->id);

        // Simula o tempo de permanência na posição
        passa_tempo(trajeto->id, pos_atual.x, pos_atual.y, pos_atual.tempo);

        // Sai da posição anterior (exceto na primeira posição do trajeto)
        if (i > 0) {
            sai(pos_anterior.x, pos_anterior.y);
        }

        // Atualiza a posição anterior
        pos_anterior = pos_atual;
    }

    // Libera a memória alocada para o trajeto
    free(trajeto->trajeto);
    free(trajeto);

    pthread_exit(NULL);
}

int main() {
    int N, n_threads;
    Trajeto trajetos[MAX_THREADS];
    pthread_t threads[MAX_THREADS];

    // Inicializa as variáveis de exclusão mútua e de condição para cada posição na grade
    for (int i = 0; i < MAX_N; i++) {
        for (int j = 0; j < MAX_N; j++) {
            pthread_mutex_init(&mutex_grade[i][j], NULL);
            pthread_cond_init(&cond_grade[i][j], NULL);
        }
    }

    // Leitura da entrada
    scanf("%d %d", &N, &n_threads);

    for (int i = 0; i < n_threads; i++) {
        trajetos[i].id = i + 1;
        scanf("%d %d %d", &trajetos[i].id, &trajetos[i].grupo, &trajetos[i].num_posicoes);
        trajetos[i].trajeto = (Posicao *)malloc(trajetos[i].num_posicoes * sizeof(Posicao));
        for (int j = 0; j < trajetos[i].num_posicoes; j++) {
            scanf("%d %d %d", &trajetos[i].trajeto[j].x, &trajetos[i].trajeto[j].y, &trajetos[i].trajeto[j].tempo);
        }
    }

    // Criação das threads
    for (int i = 0; i < n_threads; i++) {
        pthread_create(&threads[i], NULL, thread_function, (void *)&trajetos[i]);
    }

    // Aguarda a finalização das threads
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Libera as variáveis de exclusão mútua e de condição
    for (int i = 0; i < MAX_N; i++) {
        for (int j = 0; j < MAX_N; j++) {
            pthread_mutex_destroy(&mutex_grade[i][j]);
            pthread_cond_destroy(&cond_grade[i][j]);
        }
    }

    return 0;
}