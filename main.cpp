#include "global.h"
#include "reader.h"
#include "individuo.h"
#include "isla.h"

// largo de vector profit = N
// coeff es de M+Q vectores de N
// largo de vector bound = M+Q

void printInstance(int instancia){
	cout << "---------------------- Instancia " << instancia << " ----------------------\n";
	cout << "N = " << N << ", M = " << M << ", Q = " << Q << ", positive_coeffs = " << pos_coeffs << "\n";
	for(int i=0; i<M; i++){
		for(int j=0; j<N; j++){
			cout << coeff[i][j] << "\t";
		}
		cout << " <= " << bound[i] << "\n";
	}

	for(int i=M; i<M+Q; i++){
		for(int j=0; j<N; j++){
			cout << coeff[i][j] << "\t";
		}
		cout << " >= " << bound[i] << "\n";
	}

	cout << "Profits:\n";
	for(int i=0; i<N; i++){
		cout << profit[i] << "\t";
	}

	cout << "\n";
}

int main(int argc, char const *argv[]){

	if(argc < 6){
		cout << "mal uso\n";
		cout << "Uso: " << argv[0] << " <opcion_Q> <opcion_coefs> <instancia> <seed> <TamañoIsla> < <archivo_instancias>\n";
		return 0;
	}

	ISLA_SIZE = stoi(argv[5]);

	rng.seed(stoi(argv[4]));

	int T; cin >> T;
	int cual_instancia = stoi(argv[3]);
	if(cual_instancia > T || cual_instancia < 1){
		cout << "Instancia " << cual_instancia << " no encontrada. Total de instancias del archivo es " << T << "\n";
		return 0;
	}

	cout << "T = " << T << "\n";
	bool input_check = readInput(stoi(argv[1]), argv[2], cual_instancia);
	if(!input_check){
		return 0;
	}
	printInstance(cual_instancia);

	cout << "--------------------------------- Población Inicial -------------------------\n";
	isla I_inicial;
	I_inicial.print();
	return 0;
}