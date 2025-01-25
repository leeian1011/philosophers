#ifndef PHILO_H
# define THINK "is thinking."
# define LIFT_FORK "has lifted a fork."
# define EATING "is eating."
# define SLEEPING "is sleeping."
# define DIED "has starved to death in agonizing and excruciating hunger."
# define REAPING "is a reaping reaper."
# include <pthread.h>
# include <stdbool.h>
# include <sys/time.h>

typedef struct s_forks {
  int len;
  pthread_mutex_t *fork_mutex;
} t_forks;

typedef struct s_id {
  int uid;
  pthread_mutex_t id_mutex;
  struct timeval *time;
  pthread_mutex_t *time_mutex;
} t_id;

typedef struct s_logger {
  int (*log)(const char *, ...);
  pthread_mutex_t logger_mutex;
} t_logger;

typedef struct s_greaper {
  bool death;
  pthread_mutex_t death_mutex;
} t_greaper;

typedef struct s_philo {
  int ttd;
  int tte;
  int tts;
  t_forks *forks;
  t_id *id;
  t_logger *logger;
  t_greaper *grim_reaper;
} t_philo;

#endif
