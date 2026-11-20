#ifndef INDIVIDUO_H
#define INDIVIDUO_H

#include "global.h"

class individuo{
public:
	// Atributos
	vector<int> x;
	int z;
	double K_f, D_f;

	// Métodos
	individuo();
	individuo(vector<int> &v);
	void print();
	void printFeasibility();
	void updateFeasibility();
	vector<int> cruce(individuo &otro);
	void mutar(vector<int> &v);

	// int getFitness();
	double getKFeasibility();
	double getDFeasibility();
};




#endif