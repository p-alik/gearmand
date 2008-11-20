/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"

/* Initialize a task structure. */
gearman_task_st *gearman_task_create(gearman_st *gearman,
                                     gearman_task_st *task)
{
  if (task == NULL)
  {
    task= malloc(sizeof(gearman_task_st));
    if (task == NULL)
      return NULL;

    memset(task, 0, sizeof(gearman_task_st));
    task->options|= GEARMAN_TASK_ALLOCATED;
  }
  else
    memset(task, 0, sizeof(gearman_task_st));

  task->gearman= gearman;

  if (gearman->task_list)
    gearman->task_list->prev= task;
  task->next= gearman->task_list;
  gearman->task_list= task;
  gearman->task_count++;

  return task;
}

/* Free a task structure. */
void gearman_task_free(gearman_task_st *task)
{
  if (task->gearman->task_list == task)
    task->gearman->task_list= task->next;
  if (task->prev)
    task->prev->next= task->next;
  if (task->next)
    task->next->prev= task->prev;
  task->gearman->task_count--;

  if (task->options & GEARMAN_TASK_ALLOCATED)
    free(task);
}

/* Get task attributes. */
char *gearman_task_job_handle(gearman_task_st *task)
{
  return task->job_handle;
}

char *gearman_task_function(gearman_task_st *task)
{
  return (char *)task->packet.arg[0];
}

char *gearman_task_uuid(gearman_task_st *task)
{
  return (char *)task->packet.arg[1];
}
