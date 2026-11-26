#include "global.h"

bool readInput(int opcion_Q, string opcion_coeff, int instancia){	
	if(instancia > 1){
		for(int i=1; i<instancia; i++){
			cin >> N >> M;
			for(int j=0; j < 2*M+9; j++){
				cin.ignore(INT_MAX, '\n'); // elimina el resto
			}
		}
	}
	cin >> N >> M;

	if(opcion_Q == 1){
		Q = 1;
	}
	else if(opcion_Q == 2){
		Q = M/2;
	}
	else if(opcion_Q == 3){
		Q = M;
	}
	else{
		cout << opcion_Q << " no es una opción válida para Q ('1', '2', '3')\n";
		return false;
	}

	if(opcion_coeff == "mixed"){
		pos_coeffs = false;
	}
	else if(opcion_coeff == "positive"){
		pos_coeffs = true;
	}
	else{
		cout << opcion_coeff << " no es una opción válida para opcion_coeff ('mixed', 'positive')\n";
		return false;
	}

	coeff.resize(M+Q, vector<int>(N)); profit.resize(N); bound.resize(M+Q);
	
	// Coeficientes de knapsack
	for(int i=0; i<M; i++){
		for(int j=0; j<N; j++){
			cin >> coeff[i][j];
		}
	}
	for(int i=0; i<M; i++){
		cin >> bound[i];
	}

	// Coeficientes de demanda
	for(int i=M; i<M+Q; i++){
		for(int j=0; j<N; j++){
			cin >> coeff[i][j];
		}
	}

	// Saltar las líneas que sobraron
	for(int i=0; i<M-Q + 1; i++){
		cin.ignore(INT_MAX, '\n');
	}

	for(int i=M; i<M+Q; i++){
		cin >> bound[i];
	}

	// Profits
	if(!pos_coeffs){
		for(int i=0; i<4; i++){
			cin.ignore(INT_MAX, '\n');
		}
	}
	else{
		cin.ignore(INT_MAX, '\n');
	}

	for(int i=1; i<opcion_Q; i++){
		cin.ignore(INT_MAX, '\n');
	}

	for(int i=0; i<N; i++){
		cin >> profit[i];
	}

	return true;
}