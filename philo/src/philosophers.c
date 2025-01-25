#include "../includes/philosophers.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <bits/pthreadtypes.h>
#include <string.h>

bool consult_greaper(int id, t_philo *env)
{
  bool diagnosis;

  pthread_mutex_lock(&env->grim_reaper->death_mutex);
  diagnosis = env->grim_reaper->death;
  pthread_mutex_unlock(&env->grim_reaper->death_mutex);
  return (diagnosis);
}

unsigned long time_to_ms(struct timeval time)
{
  return ((time.tv_sec * 1000UL) + (time.tv_usec / 1000UL));
}

void update_time(int id, t_philo *env)
{
  pthread_mutex_lock(&env->id->time_mutex[id]);
  gettimeofday(&env->id->time[id], NULL);
  pthread_mutex_unlock(&env->id->time_mutex[id]);
}

void log_action(int id, t_philo *env, char *action)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  pthread_mutex_lock(&env->logger->logger_mutex);
  env->logger->log("%lu %i %s\n", time_to_ms(now), id, action);
  pthread_mutex_unlock(&env->logger->logger_mutex);
}

int first_fork_index(int id, int count)
{
  if (id == count - 1)
    return (0);
  return (id);
}

int second_fork_index(int id, int count)
{
  if (id == count - 1)
    return (id);
  else if (id == 0)
    return (count - 1);
  else
    return (id + 1);
}

bool lift_first_fork(int id, t_philo *env)
{
  if (consult_greaper(id, env))
    return (true);
  pthread_mutex_lock(&env->forks->fork_mutex[first_fork_index(id, env->forks->len)]);
  if (consult_greaper(id, env))
  {
    pthread_mutex_unlock(&env->forks->fork_mutex[first_fork_index(id, env->forks->len)]);
    return (true);
  }
  log_action(id, env, LIFT_FORK);
  return (0);
}

bool lift_second_fork(int id, t_philo *env)
{
  if (consult_greaper(id, env))
  {
    pthread_mutex_unlock(&env->forks->fork_mutex[first_fork_index(id, env->forks->len)]);
    return (true);
  }
  pthread_mutex_lock(&env->forks->fork_mutex[second_fork_index(id, env->forks->len)]);
  if (consult_greaper(id, env))
  {
    pthread_mutex_unlock(&env->forks->fork_mutex[first_fork_index(id, env->forks->len)]);
    return (true);
  }
  else
  {
    log_action(id, env, LIFT_FORK);
    return (0);
  }
}

bool drop_fork(int id, t_philo *env)
{
  pthread_mutex_unlock(&env->forks->fork_mutex[first_fork_index(id, env->forks->len)]);
  pthread_mutex_unlock(&env->forks->fork_mutex[second_fork_index(id, env->forks->len)]);
  return (consult_greaper(id, env));
}

void retrieve_id(int *id, t_philo *env)
{
  pthread_mutex_lock(&env->id->id_mutex);
  *id = env->id->uid++ - 1;
  pthread_mutex_unlock(&env->id->id_mutex);
}

void *work(void *metadata)
{
  int id;
  t_philo *env;

  env = (t_philo *)metadata;
  retrieve_id(&id, env);
  update_time(id, env);
  while (1)
  {
    if (consult_greaper(id, env))
      break;
    log_action(id, env, THINK);
    if (lift_first_fork(id, env))
      break ;
    if (lift_second_fork(id, env))
      break ;
    log_action(id, env, EATING);
    usleep(env->tte * 1000);
    if (drop_fork(id, env))
      break;
    update_time(id, env);
    log_action(id, env, SLEEPING);
    usleep(env->tts * 1000);
  }
  return NULL;
}

t_philo *env_init(int count)
{
  t_philo *env;

  env = malloc(sizeof(t_philo));
  env->id = malloc(sizeof(t_id));
  if (!env->id)
    return (NULL);
  env->id->uid = 1;
  env->id->time = malloc(sizeof(struct timeval) * count);
  memset(env->id->time, 0, sizeof(struct timeval) * count);
  env->id->time_mutex = malloc(sizeof(pthread_mutex_t) * count);
  env->forks = malloc(sizeof(t_forks));
  env->forks->fork_mutex = malloc(sizeof(pthread_mutex_t) * count);
  env->forks->len = count;
  env->logger = malloc(sizeof(t_logger));
  env->logger->log = printf;
  env->grim_reaper = malloc(sizeof(t_greaper));
  env->grim_reaper->death = false;
  env->tte = 200;
  env->tts = 100;
  env->ttd = 310;
  pthread_mutex_init(&env->grim_reaper->death_mutex, NULL);
  pthread_mutex_init(&env->id->id_mutex, NULL);
  pthread_mutex_init(&env->logger->logger_mutex, NULL);
  while (--count >= 0)
  {
    pthread_mutex_init(&env->id->time_mutex[count], NULL);
    pthread_mutex_init(&env->forks->fork_mutex[count], NULL);
  }
  return (env);
}

void env_free(t_philo *env, int count)
{
  while (--count >= 0)
  {
    pthread_mutex_destroy(&env->id->time_mutex[count]);
    pthread_mutex_destroy(&env->forks->fork_mutex[count]);
  }
  pthread_mutex_destroy(&env->logger->logger_mutex);
  pthread_mutex_destroy(&env->id->id_mutex);
  pthread_mutex_destroy(&env->grim_reaper->death_mutex);
  free(env->grim_reaper);
  free(env->logger);
  free(env->forks->fork_mutex);
  free(env->forks);
  free(env->id->time_mutex);
  free(env->id->time);
  free(env->id);
  free(env);
}

void *grim_work(void *metadata)
{
  t_philo *env;
  struct timeval now;
  struct timeval philo;
  int i;

  env = (t_philo *)metadata;
  while (1)
  {
    i = 0;
    while (i < env->forks->len)
    {
      pthread_mutex_lock(&env->id->time_mutex[i]);
      gettimeofday(&now, NULL);
      philo = env->id->time[i];
      pthread_mutex_unlock(&env->id->time_mutex[i]);
      if (time_to_ms(philo) > 0)
      {
        if ((long)(time_to_ms(now) - time_to_ms(philo)) > (long)env->ttd)
        {
          pthread_mutex_lock(&env->grim_reaper->death_mutex);
          env->grim_reaper->death = true;
          pthread_mutex_unlock(&env->grim_reaper->death_mutex);
          log_action(i, env, DIED);
          usleep(env->tts);
          return (NULL);
        }
      }
      i++;
    }
  }
}

// philo_count, time till die, eat time, sleep time
int main ()
{
  pthread_t *philo;
  t_philo *env;
  pthread_t grim_reaper;
  int count;

  count = 4;
  philo = malloc(count * sizeof(pthread_t));

  env = env_init(count);
  while (--count >= 0)
  {
    pthread_create(&philo[count], NULL, work, env);
    pthread_detach(philo[count]);
  }
  pthread_create(&grim_reaper, NULL, grim_work, env);
  pthread_join(grim_reaper, NULL);
  env_free(env, count);
  free(philo);
  return (0);
}
