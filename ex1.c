#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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

// Estrutura para armazenar informações sobre quais os grupos ocupam certa posicao
typedef struct {
    int ocupadas;
    int grupo[2];
} GradeGrupo;

// Variáveis globais
pthread_mutex_t mutex;
pthread_cond_t cond;
GradeGrupo grade_grupo[MAX_N][MAX_N];

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
void entra(int x, int y, int tid, int grupo) {
    pthread_mutex_lock(&mutex);
    while (grade_grupo[x][y].ocupadas == 2 || (grade_grupo[x][y].ocupadas == 1 && grade_grupo[x][y].grupo[0] == grupo)) {
        pthread_cond_wait(&cond, &mutex);
    }
    if(grade_grupo[x][y].ocupadas == 0) {
        grade_grupo[x][y].grupo[0] = grupo; // Marca o grupo da thread que ocupa a posição
        grade_grupo[x][y].ocupadas = 1;
    } else {
        grade_grupo[x][y].grupo[1] = grupo; // Marca o grupo da thread que ocupa a posição
        grade_grupo[x][y].ocupadas = 2;
    }
    pthread_mutex_unlock(&mutex);
}

// Função para sair de uma posição
void sai(int x, int y, int grupo) {
    pthread_mutex_lock(&mutex);
    if(grade_grupo[x][y].ocupadas == 1) {
        grade_grupo[x][y].grupo[0] = 0; // Libera a posição
        grade_grupo[x][y].ocupadas = 0;
    } else {
        if(grade_grupo[x][y].grupo[0] == grupo) {
            grade_grupo[x][y].grupo[0] = grade_grupo[x][y].grupo[1];
            grade_grupo[x][y].grupo[1] = 0; // Libera a posição
        } else {
            grade_grupo[x][y].grupo[1] = 0; // Libera a posição
        }
        grade_grupo[x][y].ocupadas = 1;
    }
    pthread_cond_broadcast(&cond); // Libera as threads bloqueadas aguardando a posição
    pthread_mutex_unlock(&mutex);
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
        entra(pos_atual.x, pos_atual.y, trajeto->id, trajeto->grupo);

        // Sai da posição anterior (exceto na primeira posição do trajeto)
        if (i > 0) {
            sai(pos_anterior.x, pos_anterior.y, trajeto->grupo);
        }

        // Simula o tempo de permanência na posição
        passa_tempo(trajeto->id, pos_atual.x, pos_atual.y, pos_atual.tempo);

        // Atualiza a posição anterior
        pos_anterior = pos_atual;

        // Sai da posição anterior caso seja a última posição do trajeto
        if(i == trajeto->num_posicoes-1) {
            sai(pos_anterior.x, pos_anterior.y, trajeto->grupo);
        }
    }

    // Libera a memória alocada para o trajeto
    free(trajeto->trajeto);

    pthread_exit(NULL);
}

int main() {
    int N, n_threads;
    Trajeto trajetos[MAX_THREADS];
    pthread_t threads[MAX_THREADS];

    // Inicializa como 0 a estrutura para armazenar informações sobre quais os grupos ocupam certa posicao
    for(int iterator = 0; iterator < MAX_N; iterator++) {
        for(int jterator = 0; jterator < MAX_N; jterator++) {
            grade_grupo[iterator][jterator].grupo[0] = 0;
            grade_grupo[iterator][jterator].grupo[1] = 0;
        }
    }

    // Inicializa as variáveis de exclusão mútua e de condição para cada posição na grade
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

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
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}