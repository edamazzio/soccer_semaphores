#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

void open_existing_semaphores(void);
void close_semaphores(void);
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
int get_score_a(void);
int get_score_b(void);

static int *cancha_a_id;
static int *cancha_b_id;
static int *bola_id;
static int *cancha_a_goles;
static int *cancha_b_goles;
static int *match_finished;

#define DEFcancha_a_id_sem     "/name00"
#define DEFcancha_b_id_sem     "/name01"
#define DEFbola_id_sem         "/name02"
#define DEFcancha_a_goles_sem  "/name03"
#define DEFcancha_b_goles_sem  "/name04"
#define DEFmatch_finished_sem  "/name05"

sem_t *cancha_a_id_sem;
sem_t *cancha_b_id_sem;
sem_t *bola_id_sem;
sem_t *cancha_a_goles_sem;
sem_t *cancha_b_goles_sem;
sem_t *match_finished_sem;

int main(){
  initialize_shared_memory();
  initialize_semaphores();

  int match_time = 5*60;
  int children_amount = 10;
  pid_t children_id[children_amount];

  /* Start children. */
  for (int i = 0; i < children_amount; i++) {
    char equipo = 'A';
    if(i >= (children_amount/2)){
      equipo = 'B';
    }
    if ((children_id[i] = fork()) < 0) {
      perror("Fork Error");
      abort();
    } else if (children_id[i] == 0) {
      printf("--Fork: Player with PID %d from team %c created. Its parent is %d\n",getpid(), equipo, getppid() );
      play_soccer(equipo);
      //open_existing_semaphores();
      exit(0);
    }
  }

  sleep(match_time);
  sem_wait(match_finished_sem);
  printf("\nTime is up! Finishing match... Waiting for children to finish. \n\n");
  *match_finished = 1;
  sem_post(match_finished_sem);

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
  cancha_a_id_sem       = sem_open(DEFcancha_a_id_sem   , O_CREAT, 0644, 1);
  cancha_b_id_sem       = sem_open(DEFcancha_b_id_sem   , O_CREAT, 0644, 1);
  bola_id_sem           = sem_open(DEFbola_id_sem       , O_CREAT, 0644, 1);
  cancha_a_goles_sem    = sem_open(DEFcancha_a_goles_sem, O_CREAT, 0644, 1);
  cancha_b_goles_sem    = sem_open(DEFcancha_b_goles_sem, O_CREAT, 0644, 1);
  match_finished_sem    = sem_open(DEFmatch_finished_sem, O_CREAT, 0644, 1);

}

void destroy_sempahores(){
  //printf("unlinkin semaphores\n");
  sem_unlink(DEFcancha_a_id_sem   );
  sem_unlink(DEFcancha_b_id_sem   );
  sem_unlink(DEFbola_id_sem       );
  sem_unlink(DEFcancha_a_goles_sem);
  sem_unlink(DEFcancha_b_goles_sem);
  sem_unlink(DEFmatch_finished_sem);
}

void wait_for_children(int children_amount){
  /* Wait for children to exit. */
  int status;
  pid_t pid;
  while (children_amount > 0) {
    pid = wait(&status);
    //printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
    --children_amount;  // TODO(pts): Remove pid from the children_id array.
  }
  printf ("\n\nMatch finished!\n\nFinal score is:\nA : %d\t\tB : %d\n", *cancha_a_goles, *cancha_b_goles);
  close_semaphores();
  destroy_sempahores();
}

void open_existing_semaphores(){
  cancha_a_id_sem       = sem_open(DEFcancha_a_id_sem   , 0);
  cancha_b_id_sem       = sem_open(DEFcancha_b_id_sem   , 0);
  bola_id_sem           = sem_open(DEFbola_id_sem       , 0);
  cancha_a_goles_sem    = sem_open(DEFcancha_a_goles_sem, 0);
  cancha_b_goles_sem    = sem_open(DEFcancha_b_goles_sem, 0);
  match_finished_sem    = sem_open(DEFmatch_finished_sem, 0);

}

void play_soccer(char equipo){
  open_existing_semaphores();
  int outofwhile = 1;
  sem_wait(match_finished_sem);
  while(! *match_finished){
    sem_post(match_finished_sem);
    wait_for_ball();
    if(ask_for_ball()){
      for(int i=0;i<3;i++){
        if(ask_for_cancha(equipo)){
          score_goal(equipo);
          break;
        }else{
          printf ("--Ball: Ball release by %d. Couldn't get cancha.\n", getpid());
          release_ball();
        }
      }
    }
    sem_wait(match_finished_sem);
  }
  sem_post(match_finished_sem);
}

void close_semaphores(){
  //printf("close semaphores\n");
  sem_close(cancha_a_id_sem   );
  sem_close(cancha_b_id_sem   );
  sem_close(bola_id_sem       );
  sem_close(cancha_a_goles_sem);
  sem_close(cancha_b_goles_sem);
  sem_close(match_finished_sem);
  //printf("closed semaphores\n");
}

void wait_for_ball(){
  ////printf("Child %d waiting for ball\n", getpid());
  srand(getpid());
  int tiempo_espera = rand() % 16 +5;
  //printf("--Wait: Player %d is waiting %d to get the ball\n", getpid(), tiempo_espera);
  sleep(tiempo_espera);
  ////printf("Child %d waiting finished for ball\n", getpid());
}

int ask_for_ball(){
  ////printf("Child %d asking for ball\n", getpid());
  int got_ball = 0;
  sem_wait(bola_id_sem);
  if(*bola_id == 0){
    *bola_id = getpid();
    got_ball = 1;
    printf ("--Ball: Player %d got ball\n", getpid());
  }else{
    //printf ("--Ball: Player %d COULDN'T get ball. Player %d has ball\n", getpid(), *bola_id);
  }
  sem_post(bola_id_sem);
  ////printf("Child %d request for ball = %d\n", getpid(), got_ball);
  return got_ball;
}

int ask_for_cancha(char my_equipo){
  int got_cancha = 0;
  sem_t *cancha_sem;
  int *cancha_id;
  if (my_equipo == 'B'){
    //Pedir la cancha del equipo contrario
    cancha_sem = cancha_a_id_sem;
    sem_wait(cancha_sem);
    cancha_id = cancha_a_id;
    ////printf("Cancha equipo B seteada para ser pedida\n");
  }else
  {
    cancha_sem = cancha_b_id_sem;
    sem_wait(cancha_sem);
    cancha_id = cancha_b_id;
    ////printf("Cancha equipo A seteada para ser pedida\n");
  }
  if(*cancha_id == 0){
    *cancha_id = getpid();
    got_cancha = 1;
    printf ("--Cancha: Player %d from team %c got enemy cancha\n", getpid(), my_equipo);
  }else{
    printf ("Player %d from team %c COULDN'T get cancha. Player %d has cancha\n", getpid(), my_equipo, *cancha_id);
  }
  sem_post(cancha_sem);
  return got_cancha;
}

void score_goal(char my_equipo){
  sem_t *cancha_goles_sem;
  int *cancha_goles;
  char enemy_equipo;

  if (my_equipo == 'B'){
    cancha_goles_sem = cancha_a_goles_sem;
    sem_wait(cancha_goles_sem);
    cancha_goles = cancha_a_goles;
    enemy_equipo = 'A';
  }else{
    cancha_goles_sem = cancha_b_goles_sem;
    sem_wait(cancha_goles_sem);
    cancha_goles = cancha_b_goles;
    enemy_equipo = 'B';

  }
  *cancha_goles +=1;
  sem_post(cancha_goles_sem);
  printf("--Score: Player %d of team %c scored. Current score = A : %d\tB : %d\n", getpid(), my_equipo, get_score_a(), get_score_b());
  release_ball();
  release_cancha(enemy_equipo);
}

int get_score_a(){
  sem_wait(cancha_a_goles_sem);
  int score = *cancha_a_goles;
  sem_post(cancha_a_goles_sem);
  return score;
}
int get_score_b(){
  sem_wait(cancha_b_goles_sem);
  int score = *cancha_b_goles;
  sem_post(cancha_b_goles_sem);
  return score;
}

void release_ball(){
  sem_wait(bola_id_sem);
  *bola_id = 0;
  sem_post(bola_id_sem);
}

void release_cancha(char equipo){
  sem_t *cancha_sem;
  if (equipo == 'B'){
    cancha_sem = cancha_b_id_sem;
    sem_wait(cancha_sem);
    *cancha_b_id=0;
    //printf("Cancha equipo B pertenece a: %d\n",*cancha_a_id);
  }else
  {
    cancha_sem = cancha_a_id_sem;
    sem_wait(cancha_sem);
    *cancha_a_id=0;
    //printf("Cancha equipo A pertenece a: %d\n",*cancha_a_id);
  }
  sem_post(cancha_sem);
  //printf("Cancha from equipo %c has been released by player %d\n", equipo, getpid());
}


/*TODO
 - cambiar spanglish
 - hacer los semaforos en memoria compartida

*/
