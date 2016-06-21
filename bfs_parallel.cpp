#include<pthread.h>
#include<unistd.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<queue>
#include<list>
#include<stdlib.h>
#include<stdio.h>
#include<string>
#include<time.h>
#include<semaphore.h>
#define NUM_THREADS     10
#define BUFF_SIZE       1000

using namespace std;


int nv, ed;
list<int> FQ;
pthread_mutex_t mux, update;
pthread_mutex_t thd;
pthread_cond_t cond;
list<int> Q;
list<int> *th;
list<int> *adj;
int *visited;
int th_complete;
int die = 1;
struct timespec start, finish;
double elapsed;
int s = 1;

void BFSserial(int s)
{
	Q.clear();
        for(int i=0;i<=nv;i++)
                visited[i] = 0;

        Q.push_back(s);
        visited[s] = 1;

        while(!Q.empty())
        {
                int u = Q.front();
                //cout << u << "   ";cin.ignore();
                Q.pop_front();
                for(list<int>::iterator i = adj[u].begin(); i!=adj[u].end(); ++i)
                {
                        int vv = *i;
                        if(visited[vv] == 0)
                        {
                                visited[vv] = 1;
                                Q.push_back(vv);
                        }
                }
        }
}

void BFSparallelOpenMP(int s)
{
	Q.clear();
        for(int i=0;i<=nv;i++)
                visited[i] = 0;

        list<int> Q;
        Q.push_back(s);
        visited[s] = 1;
        
	list<int>::iterator j;
        while(!Q.empty())
        {
                int u = Q.front();
                //cout << u << "   ";
                Q.pop_front();
		int size = adj[u].size();
		j = adj[u].begin();
		#pragma omp parallel for schedule(dynamic, BUFF_SIZE)
                for(int k=0; k<size; k++)
                {
			//cout <<  "*  ";
                        int vv = *j;

				if(visited[vv] == 0)
                        	{
                               		visited[vv] = 1;
					#pragma omp critical (update)
					Q.push_back(vv);
                        	}
				#pragma omp critical (iterate)
					++j;
			

                }
        }
}



void t_pool(void *x)
{
        int id = (int)x;
        while(die)
        {
                pthread_mutex_lock(&mux);
                        while(1)
                        {
                                pthread_cond_wait(&cond, &mux);
                                break;
                        }
                pthread_mutex_unlock(&mux);
                list<int>::iterator i;
                for(i = th[id].begin(); i!=th[id].end(); ++i)
                {
                        int ver = *i;
                        list<int>::iterator j;
                        for(j = adj[ver].begin(); j!=adj[ver].end(); ++j)
                        {
                                int v = *j;
                                if(visited[v] == 0)
                                {
                                        //pthread_mutex_lock(&update);
                                        visited[v] = 1;
                                        Q.push_back(v);
                                        //pthread_mutex_unlock(&update);
                                }
                        }
                }
                pthread_mutex_lock(&thd);
                th_complete++;
                pthread_mutex_unlock(&thd);
        }
	pthread_exit(NULL);

}

void wakeSignal()
{
        pthread_mutex_lock(&mux);
                pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mux);
}

void BFSparallelPthreads(int s)
{
	Q.clear();
        for(int i = 0;i<=nv;i++)
                visited[i] = 0;
        Q.push_back(s);
        visited[s] = 1;
        int th_count = 0;
        while(1)
        {
                th_complete = 0;
                while(!Q.empty())
                {
                        int temp = Q.front();
                        th[th_count%(NUM_THREADS-1)].push_back(temp);
                        Q.pop_front();
                        //cout << temp << "   ";
                        th_count++;
                }
                th_count = th_count%(NUM_THREADS-1);
                wakeSignal();
                while(th_complete!=(NUM_THREADS-1));
                if(Q.empty())
                {
                        break;
                }
        }

}
void takeInput(char *s)
{
        fstream fin; int rc, u;
        fin.open(s, fstream::in);
        stringstream ss;
        char buff[BUFF_SIZE];
        char bkp[BUFF_SIZE];
        fin.getline(buff, BUFF_SIZE);

        ss << buff;
        ss.getline(buff,10,',');
        nv = atoi(buff);

        adj = new list<int>[nv+1];
        for(int i=0;i<nv;i++)
        {
                fin.getline(buff, BUFF_SIZE);

                stringstream sts(buff);
                sts.getline(buff,10,',');
                u = atoi(buff);
                int temp;
                while(1)
                {
                        sts.getline(buff,10,',');
                        temp = atoi(buff);
                        if(!temp)
                                break;
                        adj[u].push_back(temp);
                }
        }

}


void openMP()
{
	clock_gettime(CLOCK_REALTIME, &start);
        BFSparallelOpenMP(s);
        clock_gettime(CLOCK_REALTIME, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        printf("OpenMP implementation time: %f\n", elapsed);
}

void serial()
{
	clock_gettime(CLOCK_REALTIME, &start);
        BFSserial(s);
        clock_gettime(CLOCK_REALTIME, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        printf("Serial implementation time: %f\n", elapsed);

}

void pthd()
{
	clock_gettime(CLOCK_REALTIME, &start);
        BFSparallelPthreads(s);
        clock_gettime(CLOCK_REALTIME, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        printf("Pthread implementation time: %f\n", elapsed);
}

int main(int argc, char *argv[])
{
        pthread_t p[NUM_THREADS];
        th = new list<int>[NUM_THREADS];
        pthread_mutex_init(&mux, NULL);
        pthread_cond_init(&cond, NULL);
        pthread_mutex_init(&thd, NULL);
        pthread_mutex_init(&update, NULL);


        takeInput(argv[1]);
        for(int i=0;i<NUM_THREADS;i++)
        {
                pthread_create(&p[i], NULL, t_pool, (void *)i);
        }
	
        cout << "Grapph of " << nv << " vertices, awaiting proceed input";
        cin.ignore();
	int s = 1;	
	visited = new int[nv+1];
	
	serial();
	pthd();
	openMP();

	die = 0;
        return 0;
}

