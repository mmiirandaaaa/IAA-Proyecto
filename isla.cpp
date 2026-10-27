#include "global.h"
#include "individuo.h"
#include "isla.h"

// Constructor aleatorio
isla::isla(){
	poblacion.resize(ISLA_SIZE);
}

void isla::print(){
	for(int i=0; i<ISLA_SIZE; i++){
		cout << "[" << i << "]: ";
		poblacion[i].print();
	}
	cout << "\n";
}
