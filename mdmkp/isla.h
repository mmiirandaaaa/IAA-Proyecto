#ifndef ISLA_H
#define ISLA_H

#include "global.h"
#include "individuo.h"

class isla{
public:
	// Atributos
	vector<individuo> poblacion;
	char tipo;				// 'F' (factibilidad) o 'Q' (calidad)
	double alpha, beta;		// pesos de phi (solo isla F)
	double lambda;			// penalización de f_Q (solo isla Q)
	double mejor_valor;		// mejor valor visto para el reinicio
	int estancado;			// generaciones seguidas sin mejorar mejor_valor

	// Métodos
	isla(char t);
	void print();
	double fitness(individuo &a);
	double criterio(individuo &a);
	double mejorValor();
	vector<int> ordenar(bool migracion);
	int torneo();
	individuo hijo();
	void generacion();
	void actualizarPesos();
	vector<individuo> emigrantes();
	void inmigrar(vector<individuo> &migrantes);
	bool revisarEstancamiento();
	void reiniciar(int cual);


};


#endif