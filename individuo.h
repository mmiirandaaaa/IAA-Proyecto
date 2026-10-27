#ifndef INDIVIDUO_H
#define INDIVIDUO_H

#include "global.h"

class individuo{
public:
	// Atributos
	vector<int> x;
	// int fitness;
	double K_f, D_f;

	// Métodos
	individuo();
	// individuo(vector<int> &v);
	void print();
	// void updateFitness();
	void updateFeasibility();

	// int getFitness();
	double getKFeasibility();
	double getDFeasibility();
};




#endif