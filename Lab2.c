#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>


void initialize_shared_memory(void);
void initialize_semaphores(void);
void wait_for_children(int children_amount);
void play_soccer(char equipo);
void wait_for_ball(void);
int ask_for_cancha(char my_equipo);
int ask_for_ball(void);
void score_goal(char my_equipo);
void release_ball(void);
void release_cancha(char equipo);

static int *cancha_a_id;
static int *cancha_b_id;
static int *bola_id;
static int *cancha_a_goles;
static int *cancha_b_goles;
static int *match_finished;

sem_t cancha_a_id_sem;
sem_t cancha_b_id_sem;
sem_t bola_id_sem;
sem_t cancha_a_goles_sem;
sem_t cancha_b_goles_sem;
sem_t match_finished_sem;

int main(){
  initialize_shared_memory();
  initialize_semaphores();

  int children_amount = 2;
  pid_t children_id[children_amount];

  /* Start children. */
  for (int i = 0; i < children_amount; i++) {
    char temp_equipo = 'A';
    if(i >= (children_amount/2)){
      temp_equipo = 'B';
    }
    if ((children_id[i] = fork()) < 0) {
      perror("Fork Error");
      abort();
    } else if (children_id[i] == 0) {
      play_soccer(temp_equipo);
      exit(0);
    }
  }

  int match_time = 5;//5*60;
  sleep(match_time);
  sem_wait(&match_finished_sem);
  *match_finished = 1;
  sem_post(&match_finished_sem);

  wait_for_children(children_amount);
}

void initialize_shared_memory(){
  cancha_a_id = mmap(NULL, sizeof *cancha_a_id, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  cancha_b_id = mmap(NULL, sizeof *cancha_b_id, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  bola_id = mmap(NULL, sizeof *bola_id, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  cancha_a_goles = mmap(NULL, sizeof *cancha_a_goles, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  cancha_b_goles = mmap(NULL, sizeof *cancha_b_goles, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  match_finished = mmap(NULL, sizeof *match_finished, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *cancha_a_id = 0;
  *cancha_b_id = 0;
  *bola_id = 0;
  *cancha_a_goles = 0;
  *cancha_b_goles = 0;
  *match_finished = 0;
}

void initialize_semaphores(){
  sem_init(&cancha_a_id_sem,1,1);
  sem_init(&cancha_b_id_sem,1,1);
  sem_init(&bola_id_sem,1,1);
  sem_init(&cancha_a_goles_sem,1,1);
  sem_init(&cancha_b_goles_sem,1,1);
  sem_init(&match_finished_sem,1,1);
}

void destroy_sempahores(){
  sem_destroy(&cancha_a_id_sem);
  sem_destroy(&cancha_b_id_sem);
  sem_destroy(&bola_id_sem);
  sem_destroy(&cancha_a_goles_sem);
  sem_destroy(&cancha_b_goles_sem);
  sem_destroy(&match_finished_sem);
}

void wait_for_children(int children_amount){
  /* Wait for children to exit. */
  int status;
  pid_t pid;
  while (children_amount > 0) {
    pid = wait(&status);
    printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
    --children_amount;  // TODO(pts): Remove pid from the children_id array.
  }
  destroy_sempahores();
}

void play_soccer(char equipo){
  while(! *match_finished){
    wait_for_ball();
    if(ask_for_ball()){
      for(int i=0;i<3;i++){
        if(ask_for_cancha(equipo)){
          score_goal(equipo);
          break;
        }else{
          release_ball();
        }
      }
    }
  }
}

void wait_for_ball(){
  printf("Child %d waiting for ball\n", getpid());
  int tiempo_espera = rand() % 2 +1;
  sleep(tiempo_espera);
  printf("Child %d waiting finished for ball\n", getpid());
}

int ask_for_ball(){
  //printf("Child %d asking for ball\n", getpid());
  int got_ball = 0;
  sem_wait(&bola_id_sem);
  if(*bola_id == 0){
    *bola_id = getpid();
    got_ball = 1;
  }
  sem_post(&bola_id_sem);
  //printf("Child %d request for ball = %d\n", getpid(), got_ball);
  return got_ball;
}

int ask_for_cancha(char my_equipo){
  printf("Child %d asking for cancha\n", getpid());
  int got_cancha = 0;

  //The "cancha" is 'a' by default
  sem_t cancha_id_sem = cancha_a_id_sem;
  int *cancha_id = cancha_a_id;

  //If it's 'b', it's assigned to 'b'
  if (my_equipo == 'B'){
    cancha_id_sem = cancha_b_id_sem;
    cancha_id = cancha_b_id;
  }

  sem_wait(&cancha_id_sem);
  if(*cancha_id == 0){
    *cancha_id = getpid();
    got_cancha = 1;
  }
  sem_post(&cancha_id_sem);
  printf("Child %d finished asking for cancha = %d\n", getpid(), got_cancha);
  return got_cancha;
}

void score_goal(char my_equipo){
  sem_t cancha_goles_sem = cancha_a_goles_sem;
  int *cancha_goles = cancha_a_goles;
  char enemy_equipo = 'B';
  if (my_equipo == 'B'){
    cancha_goles_sem = cancha_b_goles_sem;
    cancha_goles = cancha_b_goles;
    enemy_equipo = 'A';
  }
  sem_wait(&cancha_goles_sem);
  *cancha_goles++;
  printf("Child %d of team %c scored. Current score = %d\n", getpid(), my_equipo, *cancha_goles);
  sem_post(&cancha_goles_sem);
  release_ball();
  release_cancha(enemy_equipo);
}

void release_ball(){
  sem_wait(&bola_id_sem);
  *bola_id = 0;
  sem_post(&bola_id_sem);
}

void release_cancha(char equipo){
  sem_t cancha_sem = cancha_a_id_sem;
  int *cancha_id = cancha_a_id;
  if (equipo == 'B'){
    cancha_sem = cancha_b_id_sem;
    cancha_id = cancha_b_id;
  }
  sem_wait(&cancha_sem);
  *cancha_id = 0;
  sem_post(&cancha_sem);
}


/*TODO
cambiar equipo -> team
prototipos

Issues:
 - Semaforo ball no esta sirviendo. Varios procesos obtienen la bola al mismo tiempo
 - El proceso x no puede obtener la cancha pues el proceso x la tiene. No tiene sentido que el mismo proceso no pueda obtener la cancha porque el mismo la tiene
    - Posiblemente linea 150 crea un sem_t y al asignarlo no asigna el mismo semaforo sino que crea una copia. Puede ser esto.

*/
