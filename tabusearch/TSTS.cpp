/**********************************************************************************************\
*                                                                                              *
* Two-stage solution-based tabu search for the multidemand multidimensional                    *
* knapsack problem (MDMKP).                                                                    *
* Reference: Xiangjing Lai, Jin-Kao Hao, Dong Yue. Two-stage solution-based tabu               *
* search for the multidemand multidimensional knapsack problem.                                *
* European Journal of Operational Research 274(1): 35-48, 2019                                 *
*                                                                                              *
* Port used for the comparison with the island GA. The search procedures are the               *
* original ones. What changed is only the interface (see the list at the end of                *
* this comment):                                                                               *
*   - the instance is read with the reader of the island GA (reader.cpp/global.cpp)            *
*   - same flags as the island GA: --q --coefs --instancia --seed --tmax                       *
*     plus the two TSTS parameters: --alpha --lambda (and --L, tabu table size)                *
*   - one run per call, seeded with --seed, one result block on standard output                *
*   - stage 1 also stops when the time limit is reached                                        *
*                                                                                              *
* Compile (same flags as the island GA):                                                       *
*   g++ -O2 -Wall -o tsts TSTS.cpp reader.cpp global.cpp                                       *
*                                                                                              *
* Run:                                                                                         *
*   ./tsts --q 1 --coefs mixed --instancia 1 --seed 42 --tmax 10 < instancias/mdmkp_ct{X}.txt  *
*                                                                                              *
*                                                                                              *
* Changes with respect to the original TSTS.cpp:                                               *
*   1. Initializing() removed, readInput() of the island GA used instead.                      *
*   2. main(): the 30 runs inside one call are now 1 run, srand(SEED) instead of               *
*      srand(time(NULL)), no .sol / TSTS.txt files, results printed on stdout.                 *
*   3. L (size of the tabu tables) 2*10^8 -> 10^7 by default. With 2*10^8 each                 *
*      process needed 2.4 GB and zeroed it twice per run.                                      *
*   4. Time is measured with the same wall clock as the island GA (steady_clock).              *
*   5. Tabu_Search (stage 1) stops when the time limit is reached. Before, it only             *
*      stopped after 2*alpha non-improving iterations and could exceed the limit.              *
*   6. Guard against a division by zero when a neighborhood has no candidate move.             *
*   7. Out of bounds reads removed (C[count] in InitiaSol, S[IN]/NS[ON] when compacting).      *
*                                                                                              *
\**********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <chrono>
#include <vector>
#include <math.h>
#include <ctype.h>
#include "global.h"
#include "reader.h"
using namespace std;

typedef struct{
	int *X;
	int *S;
	int *NS;
	int IN, ON;
	int *IW;
	long int f;
} Solution; // solutions
typedef struct Neighbor{
	int f;
	int type;
	int IO;
	int k;
	int x;
	int x1;
	int y1;
} Neighbor; // neighbors

double time_limit;
int f, f_best;
int *P;
int *B;
int **R;  // weights matrix
// N, M, Q, and the instance come from global.h (island GA reader)
Solution SC;
Solution S_BEST;
Solution G_BEST;
int *W1, *W2, *W3;
int *H1, *H2, *H3;
long int L = 10000000;   // size of the tabu tables (--L)
int alpha  = 5000;       // stage 1 stops after 2*alpha non-improving iterations (--alpha)
int lanbda = 100;        // penalty weight (--lambda)

// Seconds on a steady wall clock
double now(){
	return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count();
}

/***********************************************************************************/
/***********************          1. Initializing           ************************/
/***********************************************************************************/
// Parameters, same style as the island GA. Only the ones that exist in TSTS.
bool leerParametros(int argc, char const *argv[]){
	for(int i=1; i<argc; i+=2){
		string flag = argv[i];
		if(i+1 >= argc){
			cout << "Falta el valor de " << flag << "\n";
			return false;
		}
		if(flag == "--q"){
			OPCION_Q = stoi(argv[i+1]);
		}
		else if(flag == "--coefs"){
			OPCION_COEFS = argv[i+1];
		}
		else if(flag == "--instancia"){
			INSTANCIA = stoi(argv[i+1]);
		}
		else if(flag == "--seed"){
			SEED = stoi(argv[i+1]);
		}
		else if(flag == "--tmax"){
			TMAX = stod(argv[i+1]);
		}
		else if(flag == "--alpha"){
			alpha = stoi(argv[i+1]);
		}
		else if(flag == "--lambda"){
			lanbda = stoi(argv[i+1]);
		}
		else if(flag == "--L"){
			L = stol(argv[i+1]);
		}
		else{
			cout << flag << " no es un parámetro válido\n";
			return false;
		}
	}
	if(alpha < 1 || lanbda < 0 || TMAX < 0 || L < 1000){
		cout << "Se pide: alpha >= 1, lambda >= 0, tmax >= 0, L >= 1000\n";
		return false;
	}
	return true;
}

// Copies the instance read by readInput() (vectors) to the arrays used by TSTS
void copiarInstancia()
{
	P = new int [N];
	B = new int [M+Q];
	R = new int *[M+Q];
	for(int i=0;i<M+Q;i++){
		R[i] = new int [N];
		B[i] = bound[i];
		for(int j=0;j<N;j++) R[i][j] = coeff[i][j];
	}
	for(int j=0;j<N;j++) P[j] = profit[j];
}

void AssignMemery()
{
	int i;
	SC.X      = new int [N];
	SC.S      = new int [N];
	SC.NS     = new int [N];
	SC.IW     = new int [M+Q];
	S_BEST.X  = new int [N];
	S_BEST.S  = new int [N];
	S_BEST.NS = new int [N];
	S_BEST.IW = new int [M+Q];
	G_BEST.X  = new int [N];
	G_BEST.S  = new int [N];
	G_BEST.NS = new int [N];
	G_BEST.IW = new int [M+Q];

	W1 = new int [N];
	W2 = new int [N];
	W3 = new int [N];
	H1 = new int [L];
	H2 = new int [L];
	H3 = new int [L];
	for(i=0;i<N;i++)
	{
		W1[i] = (int) pow(i+1,1.9);
		W2[i] = (int) pow(i+1,2.1);
		W3[i] = (int) pow(i+1,2.3);
	}
}
/*****************************************************************************/
/*****************          2. Initial Solution       ************************/
/*****************************************************************************/
void InitiaSol(Solution &S)
{
	int i,j,k;
	int r, p, count;
	int *C;
	int c1,c2;
	int assign, assign1;
	C = new int [N];
	for(i=0;i<N;i++) C[i] = i;
	for(i=0;i<N;i++) S.X[i] = 0;
	for(i=0;i<N;i++) S.S[i] = -1;
	for(i=0;i<N;i++) S.NS[i] = -1;
	for(i=0;i<M+Q;i++) S.IW[i] = 0;
	S.IN = 0;
	S.ON = N;
	count  = N;
	S.f = 0;
	c1 = 0;
	c2 = 0;
	for(k=0;k<N;k++)
	{
		r = rand()%count;
		p = C[r];

		assign = 0;
		for(i=0;i<M;i++)
		{
			if(S.IW[i]+R[i][p] > B[i]) {assign = 1;  break; }
		}

		assign1 = 0;
		for(i=M;i<M+Q;i++)
		{
			if(S.IW[i]+R[i][p] < B[i]) {assign1 = 1;  break; }
		}

		if(assign1 == 1 || assign == 0)
		{
			S.X[p] = 1;
			for(i=0;i<M+Q;i++) S.IW[i] += R[i][p];
			S.IN ++;
			S.ON --;
			S.S[c1] = p; c1++;
			S.f += P[p];
		}
		else
		{
			S.NS[c2] = p; c2++;
		}

		for(j=r+1;j<count;j++) C[j-1] = C[j];
		count --;
	}
	delete [] C;
}

/*****************************************************************************/
/************************    5. Certification of the solution   **************/
/*****************************************************************************/
// Returns 0 if S satisfies every constraint, 1 otherwise. Recomputes IW from X.
int proof(Solution &S)
{
	int i,j;
	int assign;
	for(i=0; i<M+Q; i++) S.IW[i] = 0;
	for(i=0;i<M+Q;i++)
		for(j=0;j<N;j++)
			S.IW[i] += S.X[j]*R[i][j];

	assign = 0;
	for(i=0;i<M;i++)
		if(S.IW[i]>B[i]) assign = 1;
	for(i=M;i<M+Q;i++)
		if(S.IW[i]<B[i]) assign = 1;
	return assign;
}

/*****************************************************************************/
/*****************     4. Solution-based Tabu Search  procedures *************/
/*****************************************************************************/
//-----------------------------------------------------------------------------
// 4.1 Build initial information for the tabu lists
//-----------------------------------------------------------------------------
void Build_Information()
{
	long int i;
	for(i=0;i<L;i++)
	{
		H1[i]=0;
		H2[i]=0;
		H3[i]=0;
	}
}
//-----------------------------------------------------------------------------
// 4.2 Tabu Search with the union neighborhood of N1 and N2
//-----------------------------------------------------------------------------
void Tabu_Search(Solution &S)
{
	int i, j, k, k0, x, y, v, u, iter ;
	int select, swap;
	int non_improve = 0 ;  // the stop condition of TS
	long int tabu_best_fc, best_fc, f_c ;
	int num_tabu_best, num_best;  // the number of tabu neighbors and non-tabu neighbors
	Neighbor best[ 50 ];
	Neighbor tabu_best[ 50 ];
	int Hx1, Hx2, Hx3;
	int Hx11, Hx22, Hx33;
	double current_time, starting_time;
	long int sum_penalty;
	long int delt;
	int flag;

	Build_Information();
	Hx1 = 0;
	Hx2 = 0;
	Hx3 = 0;
	for(i=0;i<N;i++)
	{
		Hx1 += W1[i]*(S.X[i]);
		Hx2 += W2[i]*(S.X[i]);
		Hx3 += W3[i]*(S.X[i]);
	}

	f = S.f;
	f_best = -999999;
	S_BEST.IN = S.IN;
	S_BEST.ON = S.ON;
	S_BEST.f= S.f;
	for(i=0;i<N;i++)   S_BEST.S[i]  = S.S[i];
	for(i=0;i<N;i++)   S_BEST.NS[i] = S.NS[i];
	for(i=0;i<N;i++)   S_BEST.X[i]  = S.X[i];
	for(i=0;i<M+Q;i++) S_BEST.IW[i] = S.IW[i];

	iter = 0;
	starting_time = now();
	current_time = 0;
	flag  = 0;
	// (change 5) the loop also stops when the time limit is reached
	while( (non_improve < 2*alpha || flag == 0) && current_time < time_limit )
	{
		tabu_best_fc = -999999999;
		best_fc = -999999999;
		num_tabu_best = 0;
		num_best = 0 ;
		//1). the add neighborhodd
		for(k = 0; k < S.ON; k ++) // add a term
		{
			j = S.NS[k];
			sum_penalty = 0;
			for(i=0;i<M;i++)   if(S.IW[i] + R[i][j] > B[i]) sum_penalty += (S.IW[i] + R[i][j] - B[i]);
			for(i=M;i<M+Q;i++) if(S.IW[i] + R[i][j] < B[i]) sum_penalty += (B[i] - S.IW[i] - R[i][j]);
			delt = -1.0*lanbda*sum_penalty;

			Hx11 = Hx1 + W1[j];
			Hx22 = Hx2 + W2[j];
			Hx33 = Hx3 + W3[j];
			f_c = S.f + P[j];

			if(H1[Hx11%L]==1 && H2[Hx22%L]==1 && H3[Hx33%L]==1)  // if it is tabu
			{
				if( f_c + delt > tabu_best_fc )
				{
					tabu_best[ 0 ].x = j ;
					tabu_best[ 0 ].type = 1 ;
					tabu_best[ 0 ].IO = 0;
					tabu_best[ 0 ].k = k;
					tabu_best[ 0 ].f = f_c;
					tabu_best_fc = f_c + delt;
					num_tabu_best = 1 ;
				}
				else if( f_c + delt == tabu_best_fc && num_tabu_best < 50 )
				{
					tabu_best[ num_tabu_best ].x    = j;
					tabu_best[ num_tabu_best ].type = 1;
					tabu_best[ num_tabu_best ].IO = 0;
					tabu_best[ num_tabu_best ].k = k;
					tabu_best[ num_tabu_best ].f = f_c;
					num_tabu_best++ ;
				}
			}
			else
			{
				if( f_c + delt > best_fc )
				{
					best[ 0 ].x = j;
					best[ 0 ].type = 1;
					best[ 0 ].IO = 0;
					best[ 0 ].k = k;
					best[ 0 ].f = f_c ;
					best_fc  = f_c + delt;
					num_best = 1 ;
				}
				else if( f_c + delt == best_fc && num_best < 50 )
				{
					best[ num_best ].x = j;
					best[ num_best ].type = 1 ;
					best[ num_best ].IO = 0;
					best[ num_best ].k = k;
					best[ num_best ].f = f_c;
					num_best++ ;
				}
			}
		}
		//2) the drop neighborhood
		for( k = 0; k < S.IN; k ++)  // drop a term
		{
			j = S.S[k];
			sum_penalty = 0;
			for(i=0;i<M;i++)   if(S.IW[i] - R[i][j] > B[i]) sum_penalty += (S.IW[i] - R[i][j] - B[i]);
			for(i=M;i<M+Q;i++) if(S.IW[i] - R[i][j] < B[i]) sum_penalty += (B[i] - S.IW[i] + R[i][j]);
			delt = -1.0*lanbda*sum_penalty;

			Hx11 = Hx1 - W1[j] ;
			Hx22 = Hx2 - W2[j] ;
			Hx33 = Hx3 - W3[j] ;

			f_c = S.f - P[j] ;

			if( H1[Hx11%L]==1 && H2[Hx22%L]==1 && H3[Hx33%L]==1 ) // if it is tabu
			{
				if( f_c + delt > tabu_best_fc )
				{
					tabu_best[ 0 ].x = j ;
					tabu_best[ 0 ].type = 1 ;
					tabu_best[ 0 ].IO = 1;
					tabu_best[ 0 ].k = k;
					tabu_best[ 0 ].f = f_c;
					tabu_best_fc = f_c + delt;
					num_tabu_best = 1 ;
				}
				else if( f_c + delt == tabu_best_fc && num_tabu_best < 50 )
				{
					tabu_best[ num_tabu_best ].x   = j;
					tabu_best[ num_tabu_best ].type = 1;
					tabu_best[ num_tabu_best ].IO = 1;
					tabu_best[ num_tabu_best ].k = k;
					tabu_best[ num_tabu_best ].f = f_c;
					num_tabu_best++ ;
				}
			}
			else
			{
				if( f_c + delt > best_fc )
				{
					best[ 0 ].x = j;
					best[ 0 ].type = 1;
					best[ 0 ].IO = 1;
					best[ 0 ].k = k;
					best[ 0 ].f = f_c ;
					best_fc  = f_c + delt;
					num_best = 1 ;
				}
				else if( f_c + delt == best_fc && num_best < 50 )
				{
					best[ num_best ].x   = j ;
					best[ num_best ].type = 1 ;
					best[ num_best ].IO = 1;
					best[ num_best ].k = k;
					best[ num_best ].f = f_c;
					num_best++ ;
				}
			}
		}

		//3) Evaluating the neighborhood N2 based candidate list
		for(x = 0; x < S.IN; x++)
		{
			for(y = 0; y < S.ON; y++)
			{
				sum_penalty = 0;
				for(i=0;i<M;i++) if(S.IW[i]+(R[i][S.NS[y]]-R[i][S.S[x]]) > B[i])   sum_penalty += (S.IW[i]+(R[i][S.NS[y]]-R[i][S.S[x]]) - B[i]) ;
				for(i=M;i<M+Q;i++) if(S.IW[i]+(R[i][S.NS[y]]-R[i][S.S[x]]) < B[i]) sum_penalty += (B[i]- S.IW[i] - (R[i][S.NS[y]]-R[i][S.S[x]]));
				delt = -1.0*lanbda*sum_penalty;

				f_c = S.f + (P[S.NS[y]]-P[S.S[x]]);
				Hx11 = Hx1 + (W1[S.NS[y]]- W1[S.S[x]]) ;
				Hx22 = Hx2 + (W2[S.NS[y]]- W2[S.S[x]]) ;
				Hx33 = Hx3 + (W3[S.NS[y]]- W3[S.S[x]]) ;

				if(  H1[Hx11%L]==1 && H2[Hx22%L]==1 && H3[Hx33%L]==1 ) // if it is tabu
				{
					if( f_c + delt > tabu_best_fc )
					{
						tabu_best[ 0 ].x1 = x ;
						tabu_best[ 0 ].y1 = y ;
						tabu_best[ 0 ].type = 2 ;
						tabu_best[ 0 ].IO = -1;
						tabu_best[ 0 ].f = f_c;
						tabu_best_fc = f_c + delt;
						num_tabu_best = 1 ;
					}
					else if( f_c + delt == tabu_best_fc && num_tabu_best < 50 )
					{
						tabu_best[ num_tabu_best ].x1   = x;
						tabu_best[ num_tabu_best ].y1   = y;
						tabu_best[ num_tabu_best ].type = 2;
						tabu_best[ num_tabu_best ].IO = -1;
						tabu_best[ num_tabu_best ].f = f_c;
						num_tabu_best++ ;
					}
				}
				else
				{
					if( f_c + delt > best_fc )
					{
						best[ 0 ].x1 = x;
						best[ 0 ].y1 = y;
						best[ 0 ].type = 2;
						best[ 0 ].IO = -1;
						best[ 0 ].f  = f_c;
						best_fc  = f_c + delt;
						num_best = 1 ;
					}
					else if( f_c + delt == best_fc && num_best < 50 )
					{
						best[ num_best ].x1   = x ;
						best[ num_best ].y1   = y ;
						best[ num_best ].type = 2 ;
						best[ num_best ].IO = -1 ;
						best[ num_best ].f = f_c ;
						num_best++ ;
					}
				}
			}
		}
		// (change 6) no candidate move at all
		if( num_best == 0 && num_tabu_best == 0 ) break;

		//4) moves
		if( num_best == 0 ) //aspiration criterion
		{
			select = rand() % num_tabu_best ;
			f = tabu_best[select].f ;
			if(tabu_best[select].type==2)
			{
				u = tabu_best[ select ].x1;
				v = tabu_best[ select ].y1;

				Hx1 += (W1[S.NS[v]]-W1[S.S[u]]);
				Hx2 += (W2[S.NS[v]]-W2[S.S[u]]);
				Hx3 += (W3[S.NS[v]]-W3[S.S[u]]);
				H1[Hx1%L] = 1;
				H2[Hx2%L] = 1;
				H3[Hx3%L] = 1;

				for(i=0;i<M+Q;i++)
				{
					S.IW[i] += (R[i][S.NS[v]] - R[i][S.S[u]]);
				}
				S.X[S.S[u]]  = 1 - S.X[S.S[u]];
				S.X[S.NS[v]] = 1 - S.X[S.NS[v]];

				swap    = S.S[u] ;
				S.S[u]  = S.NS[v];
				S.NS[v] = swap ;
				S.f = f;
			}

			else if(tabu_best[select].type == 1)
			{
				j  = tabu_best[ select ].x;
				k0 = tabu_best[ select ].k;

				if(tabu_best[ select ].IO == 1)
				{
					Hx1 -= W1[j];
					Hx2 -= W2[j];
					Hx3 -= W3[j];
					H1[Hx1%L] = 1;
					H2[Hx2%L] = 1;
					H3[Hx3%L] = 1;

					for(i=0;i<M+Q;i++)
					{
						S.IW[i] -= R[i][j];
					}

					S.X[j] = 1 - S.X[j];
					for(i=k0; i<S.IN-1; i++) S.S[i] = S.S[i+1];
					S.IN -- ;

					S.NS[S.ON] = j ;
					S.ON ++ ;
					S.f = f;

				}
				else if(tabu_best[ select ].IO == 0)
				{
					Hx1 += W1[j];
					Hx2 += W2[j];
					Hx3 += W3[j];
					H1[Hx1%L] = 1;
					H2[Hx2%L] = 1;
					H3[Hx3%L] = 1;

					for(i=0;i<M+Q;i++)
					{
						S.IW[i] += R[i][j];
					}
					S.X[j] = 1 - S.X[j];
					for(i=k0;i<S.ON-1;i++) S.NS[i] = S.NS[i+1];
					S.ON-- ;
					S.S[S.IN] = j;
					S.IN++ ;
					S.f = f;
				}
			}
		}
		else // non tabu
		{
			select = rand() % num_best ;
			f = best[select].f ;
			if(best[select].type == 2)
			{
				u = best[ select ].x1;
				v = best[ select ].y1;

				Hx1 += (W1[S.NS[v]]-W1[S.S[u]]);
				Hx2 += (W2[S.NS[v]]-W2[S.S[u]]);
				Hx3 += (W3[S.NS[v]]-W3[S.S[u]]);
				H1[Hx1%L] = 1;
				H2[Hx2%L] = 1;
				H3[Hx3%L] = 1;

				for(i=0;i<M+Q;i++)
				{
					S.IW[i] += (R[i][S.NS[v]] - R[i][S.S[u]]);
				}

				S.X[S.S[u]]  = 1 - S.X[S.S[u]];
				S.X[S.NS[v]] = 1 - S.X[S.NS[v]];

				swap    = S.S[u] ;
				S.S[u]  = S.NS[v];
				S.NS[v] = swap ;

				S.f = f;
			}
			else if( best[select].type == 1 )
			{
				j  = best[ select ].x;
				k0 = best[ select ].k;

				if(best[ select ].IO == 1)
				{
					Hx1 -= W1[j];
					Hx2 -= W2[j];
					Hx3 -= W3[j];
					H1[Hx1%L] = 1;
					H2[Hx2%L] = 1;
					H3[Hx3%L] = 1;

					for(i=0;i<M+Q;i++)
					{
						S.IW[i] -= R[i][j];
					}

					S.X[j] = 1 - S.X[j];
					for(i=k0;i<S.IN-1;i++) S.S[i] = S.S[i+1];
					S.IN -- ;

					S.NS[S.ON] = j ;
					S.ON ++ ;
					S.f = f;

				}
				else if(best[ select ].IO == 0)
				{
					Hx1 += W1[j];
					Hx2 += W2[j];
					Hx3 += W3[j];
					H1[Hx1%L] = 1;
					H2[Hx2%L] = 1;
					H3[Hx3%L] = 1;

					for(i=0;i<M+Q;i++)
					{
						S.IW[i] += R[i][j];
					}
					S.X[j] = 1 - S.X[j];
					for(i=k0;i<S.ON-1;i++) S.NS[i] = S.NS[i+1];
					S.ON -- ;
					S.S[S.IN] = j;
					S.IN ++ ;
					S.f = f;
				}
			}
		}
		//5. update
		iter ++ ;
		current_time = now() - starting_time;
		sum_penalty = 0;
		for(i=0;i<M;i++)   if(S.IW[i] > B[i])  sum_penalty += (S.IW[i] - B[i]);
		for(i=M;i<M+Q;i++) if(S.IW[i] < B[i])  sum_penalty += (B[i] - S.IW[i]);
		if(sum_penalty == 0) flag = 1;
		if( S.f >= f_best && sum_penalty == 0 )
		{
			if( S.f > f_best  )
			{
				f_best = S.f;
				for( i = 0 ; i < N ; i ++ )
					S_BEST.X[ i ] = S.X[ i ] ;
				for( i = 0 ; i < N ; i ++ )
					S_BEST.S[ i ] = S.S[ i ] ;
				for( i = 0 ; i < N ; i ++ )
					S_BEST.NS[ i ] = S.NS[ i ] ;
				for( i = 0 ; i < M+Q ; i ++ )
					S_BEST.IW[ i ] = S.IW[ i ] ;
				S_BEST.IN = S.IN;
				S_BEST.ON = S.ON;
				S_BEST.f = S.f;
				non_improve = 0 ;
			}
			else if ( S.f == f_best )  non_improve ++ ;
		}
		else non_improve ++ ;
	}

	for(i=0;i<N;i++)  S.X[i]  = S_BEST.X[i];
	for(i=0;i<N;i++)  S.S[i]  = S_BEST.S[i];
	for(i=0;i<N;i++)  S.NS[i] = S_BEST.NS[i];
	for(i=0;i<M+Q;i++)  S.IW[i] = S_BEST.IW[i];
	S.f  = S_BEST.f;
	S.IN = S_BEST.IN;
	S.ON = S_BEST.ON;
}
//-------------------------------------------------------------------------------------------------
// 4.3 Tabu Search with the constrained swap neighborhood N3 and the extended evaluation function
//-------------------------------------------------------------------------------------------------
void Swap_tabu_search(Solution &S, double X)
{
	int i, u, v, iter ;
	long int sum_penalty;
	long int delt;
	int select, swap;
	int non_improve = 0 ;  //the stop condition of TS
	long int tabu_best_fc, best_fc, f_c ;
	int num_tabu_best, num_best;  //the number of tabu neighbors and non-tabu neighbors
	Neighbor best[ 50 ];
	Neighbor tabu_best[ 50 ];
	int x, y;

	int Hx1, Hx2, Hx3;
	int Hx11, Hx22, Hx33;
	double current_time, starting_time;

	Build_Information();
	Hx1 = 0;
	Hx2 = 0;
	Hx3 = 0;
	for(i=0;i<N;i++)
	{
		Hx1 += W1[i]*(S.X[i]);
		Hx2 += W2[i]*(S.X[i]);
		Hx3 += W3[i]*(S.X[i]);
	}

	f = S.f;
	f_best = S.f;
	S_BEST.IN = S.IN;
	S_BEST.ON = S.ON;
	S_BEST.f = S.f;
	for(i=0;i<N;i++) S_BEST.S[i]  = S.S[i];
	for(i=0;i<N;i++) S_BEST.NS[i] = S.NS[i];
	for(i=0;i<N;i++) S_BEST.X[i]  = S.X[i];
	for(i=0;i<M+Q;i++) S_BEST.IW[i] = S.IW[i];

	iter = 0;
	non_improve = 0;
	starting_time = now();
	current_time = 0;
	while( current_time < X )
	{
		tabu_best_fc = -99999999 ;
		best_fc = -99999999 ;
		num_tabu_best = 0 ;
		num_best = 0 ;
		//2) Evaluating the neighborhood N3
		for(x = 0; x < S.IN; x++)
		{
			for(y = 0; y < S.ON; y++)
			{
				f_c = f + (P[S.NS[y]]-P[S.S[x]]) ;
				if(f_c <= f_best) continue;

				sum_penalty = 0;
				for(i=0;i<M;i++)   if(S.IW[i]+(R[i][S.NS[y]]-R[i][S.S[x]]) > B[i])  sum_penalty += (S.IW[i] + R[i][S.NS[y]] - R[i][S.S[x]] - B[i]);
				for(i=M;i<M+Q;i++) if(S.IW[i]+(R[i][S.NS[y]]-R[i][S.S[x]]) < B[i])  sum_penalty += (B[i] - S.IW[i] - R[i][S.NS[y]] + R[i][S.S[x]]);
				delt = -1.0*lanbda*sum_penalty;

				Hx11 = Hx1 + (W1[S.NS[y]]- W1[S.S[x]]) ;
				Hx22 = Hx2 + (W2[S.NS[y]]- W2[S.S[x]]) ;
				Hx33 = Hx3 + (W3[S.NS[y]]- W3[S.S[x]]) ;

				if(  H1[Hx11%L]==1 && H2[Hx22%L]==1 && H3[Hx33%L]==1 ) // if it is tabu
				{
					if( f_c + delt > tabu_best_fc )
					{
						tabu_best[ 0 ].x1 = x ;
						tabu_best[ 0 ].y1 = y ;
						tabu_best[ 0 ].type = 2 ;
						tabu_best[ 0 ].f = f_c ;
						tabu_best_fc = f_c + delt ;
						num_tabu_best = 1 ;
					}
					else if( f_c + delt == tabu_best_fc && num_tabu_best < 50 )
					{
						tabu_best[ num_tabu_best ].x1   = x;
						tabu_best[ num_tabu_best ].y1   = y;
						tabu_best[ num_tabu_best ].type = 2;
						tabu_best[ num_tabu_best ].f = f_c;
						num_tabu_best++ ;
					}
				}
				else
				{
					if( f_c + delt > best_fc )
					{
						best[ 0 ].x1 = x;
						best[ 0 ].y1 = y;
						best[ 0 ].type = 2;
						best[ 0 ].f = f_c;
						best_fc  = f_c + delt;
						num_best = 1 ;
					}
					else if( f_c + delt == best_fc && num_best < 50 )
					{
						best[ num_best ].x1   = x ;
						best[ num_best ].y1   = y ;
						best[ num_best ].type = 2 ;
						best[ num_best ].f = f_c;
						num_best++ ;
					}
				}
			}
		}
		// (change 6) no swap improves on f_best: original code did rand() % 0 here
		if( num_best == 0 && num_tabu_best == 0 ) break;

		//4) moves
		if(  num_best == 0 )  // aspiration criterion
		{
			select = rand() % num_tabu_best ;
			f = tabu_best[select].f ;
			u = tabu_best[ select ].x1;
			v = tabu_best[ select ].y1;

			Hx1 += (W1[S.NS[v]]-W1[S.S[u]]);
			Hx2 += (W2[S.NS[v]]-W2[S.S[u]]);
			Hx3 += (W3[S.NS[v]]-W3[S.S[u]]);
			H1[Hx1%L] = 1;
			H2[Hx2%L] = 1;
			H3[Hx3%L] = 1;

			for(i=0;i<M+Q;i++)
			{
				S.IW[i] += (R[i][S.NS[v]] - R[i][S.S[u]]);
			}
			S.X[S.S[u]] = 1 - S.X[S.S[u]];
			S.X[S.NS[v]] = 1 - S.X[S.NS[v]];

			swap    = S.S[u] ;
			S.S[u]  = S.NS[v];
			S.NS[v] = swap   ;
			S.f = f;
		}

		else // non tabu
		{
			select = rand() % num_best ;
			f = best[ select ].f ;
			u = best[ select ].x1;
			v = best[ select ].y1;

			Hx1 += (W1[S.NS[v]]-W1[S.S[u]]);
			Hx2 += (W2[S.NS[v]]-W2[S.S[u]]);
			Hx3 += (W3[S.NS[v]]-W3[S.S[u]]);
			H1[Hx1%L] = 1;
			H2[Hx2%L] = 1;
			H3[Hx3%L] = 1;

			for(i=0;i<M+Q;i++)
			{
				S.IW[i] += (R[i][S.NS[v]] - R[i][S.S[u]]);
			}

			S.X[S.S[u]]  = 1 - S.X[S.S[u]];
			S.X[S.NS[v]] = 1 - S.X[S.NS[v]];

			swap    = S.S[u] ;
			S.S[u]  = S.NS[v];
			S.NS[v] = swap   ;

			S.f = f;
		}
		//5. update
		iter ++ ;
		current_time = now() - starting_time;
		if( f >= f_best  )
		{
			sum_penalty = 0;
			for(i=0;i<M;i++)   if(S.IW[i] > B[i])  sum_penalty += (S.IW[i] - B[i]);
			for(i=M;i<M+Q;i++) if(S.IW[i] < B[i])  sum_penalty += (B[i] - S.IW[i]);
			if( f > f_best && sum_penalty == 0 )
			{
				f_best = f;
				for( i = 0 ; i < N ; i ++ )
					S_BEST.X[ i ] = S.X[ i ] ;
				for( i = 0 ; i < N ; i ++ )
					S_BEST.S[ i ] = S.S[ i ] ;
				for( i = 0 ; i < N ; i ++ )
					S_BEST.NS[ i ] = S.NS[ i ] ;
				for( i = 0 ; i < M+Q ; i ++ )
					S_BEST.IW[ i ] = S.IW[ i ] ;
				S_BEST.IN = S.IN;
				S_BEST.ON = S.ON;
				S_BEST.f = f;
				non_improve = 0 ;
			}
			else if ( f == f_best )  non_improve ++ ;
		}
		else non_improve ++ ;
	}

	for(i=0;i<N;i++)  S.X[i]  = S_BEST.X[i];
	for(i=0;i<N;i++)  S.S[i]  = S_BEST.S[i];
	for(i=0;i<N;i++)  S.NS[i] = S_BEST.NS[i];
	for(i=0;i<M+Q;i++)  S.IW[i] = S_BEST.IW[i];
	S.f  = S_BEST.f;
	S.IN = S_BEST.IN;
	S.ON = S_BEST.ON;
} // With a fixed value of K.

/*****************************************************************************/
/**************************     9.  Two-phase Search  ************************/
/*****************************************************************************/
void TwoPhaseSearch()
{
	int j;
	double start_time, first_time;

	start_time = now();
	InitiaSol(SC);
	Tabu_Search(SC); // the search of the first pahse.
	proof(SC);
	first_time = now() - start_time;
	Swap_tabu_search(SC,time_limit-first_time);  // the search of the second phase.
	proof(SC);

	for(j=0;j<N;j++)   G_BEST.S[j]  = SC.S[j];
	for(j=0;j<N;j++)   G_BEST.NS[j] = SC.NS[j];
	for(j=0;j<N;j++)   G_BEST.X[j]  = SC.X[j];
	for(j=0;j<M+Q;j++) G_BEST.IW[j] = SC.IW[j];
	G_BEST.f  = SC.f;
	G_BEST.IN = SC.IN;
	G_BEST.ON = SC.ON;
}

/*****************************************************************************/
/**************************   11. Main  Scheme     ***************************/
/*****************************************************************************/
int main(int argc, char const *argv[])
{
	if(!leerParametros(argc, argv)){
		cout << "Uso: " << argv[0] << " [parametros] < <archivo_instancias>\n";
		cout << "Parametros (todos opcionales): --q --coefs --instancia --seed --tmax --alpha --lambda --L\n";
		return 0;
	}

	srand(SEED);
	time_limit = TMAX;

	int T; cin >> T;
	if(INSTANCIA > T || INSTANCIA < 1){
		cout << "Instancia " << INSTANCIA << " no encontrada. Total de instancias del archivo es " << T << "\n";
		return 0;
	}
	if(!readInput(OPCION_Q, OPCION_COEFS, INSTANCIA)){
		return 0;
	}
	// cout << "T = " << T << "\n";
	cout << "N = " << N << ", M = " << M << ", Q = " << Q << ", positive_coeffs = " << pos_coeffs << "\n";

	copiarInstancia();
	AssignMemery();

	double inicio = now();
	TwoPhaseSearch();
	double tiempo = now() - inicio;

	// Independent check of the final solution: feasibility and profit recomputed from x
	bool factible = (proof(G_BEST) == 0);
	long int z = 0;
	for(int j=0;j<N;j++) if(G_BEST.X[j]==1) z += P[j];
	if(z != G_BEST.f){
		cerr << "AVISO: profit acumulado " << G_BEST.f << " distinto del recalculado " << z << "\n";
	}

	cout << "--------------------------------- Mejor solución -------------------------\n";
	// cout << "x: ";
	// for(int j=0;j<N;j++) cout << G_BEST.X[j];
	// cout << "\n";
	cout << "Profit: " << z << "\n";
	if(!factible){
		cout << "No se encontró solución factible\n";
	}
	cout << "Tiempo: " << tiempo << " s\n";
	return 0;
}