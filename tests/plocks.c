/*************************************************************************
 *                                                                       *
 * Copyright (C) 2009 Dmitry E. Oboukhov <unera@debian.org>              *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                       *
 *************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_rwlock_t rwlock;
pthread_mutex_t mutex;

void * thread(void *data)
{
	fprintf(stderr, "Started thread %u\n", pthread_self());
	for(;;) {
		int res;
		fprintf(stderr, "%u get rdlock...\n", pthread_self());
/*                 pthread_mutex_lock(&mutex); */
		res = pthread_rwlock_rdlock(&rwlock);
/*                 pthread_mutex_unlock(&mutex); */
		fprintf(stderr, "%u got rdlock: %d\n", pthread_self(), res);

		fprintf(stderr, "%u get wrlock...\n", pthread_self());
/*                 pthread_mutex_lock(&mutex); */
		pthread_rwlock_unlock(&rwlock);
		res = pthread_rwlock_wrlock(&rwlock);
/*                 pthread_mutex_unlock(&mutex); */
		fprintf(stderr, "%u got wrlock...: %d\n", pthread_self(), res);

		pthread_rwlock_unlock(&rwlock);

		fprintf(stderr, "%u unlocked\n", pthread_self());
	}
}

int main(void)
{
	pthread_t t1, t2;
	pthread_rwlock_init(&rwlock, 0);
	pthread_mutex_init(&mutex, 0);
	pthread_create(&t1, 0, thread, 0);
	pthread_create(&t2, 0, thread, 0);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
}
