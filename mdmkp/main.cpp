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

// Lee los parámetros
bool leerParametros(int argc, char const *argv[]){
	for(int i=1; i<argc; i+=2){
		string flag = argv[i];
		if(i+1 >= argc){
			cout << "Falta el valor de " << flag << "\n";
			return false;
		}
		if(flag == "--pc"){
			PC = stod(argv[i+1]);
		}
		else if(flag == "--pm"){
			PM = stod(argv[i+1]);
		}
		else if(flag == "--elite"){
			ELITE = stoi(argv[i+1]);
		}
		else if(flag == "--k"){
			K_MIG = stoi(argv[i+1]);
		}
		else if(flag == "--mu"){
			MU = stoi(argv[i+1]);
		}
		else if(flag == "--t"){
			T_OBJ = stod(argv[i+1]);
		}
		else if(flag == "--gamma"){
			GAMMA = stod(argv[i+1]);
		}
		else if(flag == "--gr"){
			GR = stoi(argv[i+1]);
		}
		else if(flag == "--gmax"){
			GMAX = stoi(argv[i+1]);
		}
		else if(flag == "--tmax"){
			TMAX = stod(argv[i+1]);
		}
		else if(flag == "--q"){
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
		else if(flag == "--s"){
			ISLA_SIZE = stoi(argv[i+1]);
		}
		else if(flag == "--csv"){
			CSV = argv[i+1];
		}
		else{
			cout << flag << " no es un parámetro válido\n";
			return false;
		}
	}

	if(ISLA_SIZE < 2 || ELITE < 1 || ELITE > ISLA_SIZE || MU < 0 || MU > ISLA_SIZE){
		cout << "Se pide: TamañoIsla >= 2, 1 <= elite <= TamañoIsla, 0 <= mu <= TamañoIsla\n";
		return false;
	}
	if(PC < 0 || PC > 1 || PM < 0 || PM > 1 || T_OBJ <= 0 || T_OBJ >= 1 || GAMMA <= 0){
		cout << "Se pide: pc y pm en [0,1], t en (0,1), gamma > 0\n";
		return false;
	}
	if(K_MIG < 1 || GR < 1 || GMAX < 0 || TMAX < 0){
		cout << "Se pide: k >= 1, gr >= 1, gmax >= 0, tmax >= 0\n";
		return false;
	}
	return true;
}

// Segundos desde inicio
double tiempo(chrono::steady_clock::time_point inicio){
	return chrono::duration<double>(chrono::steady_clock::now() - inicio).count();
}

// Actualiza x* con la población de la isla
// Una solución factible siempre le gana a una infactible, si ninguna es factible gana la de menor K_f + D_f
void actualizarMejor(isla &I, individuo &mejor){
	for(int i=0; i<ISLA_SIZE; i++){
		individuo &a = I.poblacion[i];
		bool a_fact = (a.K_f == 0 && a.D_f == 0);
		bool m_fact = (mejor.K_f == 0 && mejor.D_f == 0);
		bool mejora;
		if(a_fact && m_fact){
			mejora = a.z > mejor.z;
		}
		else if(a_fact){
			mejora = true;
		}
		else if(m_fact){
			mejora = false;
		}
		else{
			mejora = (a.K_f + a.D_f) < (mejor.K_f + mejor.D_f);
		}
		if(mejora){
			mejor = a;
		}
	}
}

// Escribe una fila del CSV: generación, profit de x*, si x* es factible y el mejor valor de cada isla
void escribirFila(ofstream &csv, int g, individuo &mejor, isla &F, isla &Q){
	csv << g << "," << mejor.z << "," << (mejor.K_f == 0 && mejor.D_f == 0) << "," << F.mejorValor() << "," << Q.mejorValor() << "\n";
}

int main(int argc, char const *argv[]){

	if(!leerParametros(argc, argv)){
		cout << "Uso: " << argv[0] << " [parametros] < <archivo_instancias>\n";
		cout << "Parametros (todos opcionales): --q --coefs --instancia --seed --s --pc --pm --elite --k --mu --t --gamma --gr --gmax --tmax --csv\n";
		return 0;
	}

	rng.seed(SEED);

	int T; cin >> T;
	int cual_instancia = INSTANCIA;
	if(cual_instancia > T || cual_instancia < 1){
		cout << "Instancia " << cual_instancia << " no encontrada. Total de instancias del archivo es " << T << "\n";
		return 0;
	}

	bool input_check = readInput(OPCION_Q, OPCION_COEFS, cual_instancia);
	if(!input_check){
		return 0;
	}

	// cout << "T = " << T << "\n";
	cout << "N = " << N << ", M = " << M << ", Q = " << Q << ", positive_coeffs = " << pos_coeffs << "\n";

	chrono::steady_clock::time_point inicio = chrono::steady_clock::now();

	isla F('F'), Q('Q');
	individuo mejor = F.poblacion[0];
	actualizarMejor(F, mejor);
	actualizarMejor(Q, mejor);

	ofstream csv;
	if(CSV != ""){
		csv.open(CSV);
		if(!csv){
			cout << "No se pudo abrir " << CSV << "\n";
			return 0;
		}
		csv.precision(10);
		csv << "generation,profit,feasible,best_F,best_Q\n";
		escribirFila(csv, 0, mejor, F, Q);
	}

	int g = 0;
	while(g < GMAX && tiempo(inicio) < TMAX){
		g++;

		F.generacion();
		actualizarMejor(F, mejor);
		F.actualizarPesos();

		Q.generacion();
		actualizarMejor(Q, mejor);

		if(g % K_MIG == 0){
			vector<individuo> deF = F.emigrantes();
			vector<individuo> deQ = Q.emigrantes();
			F.inmigrar(deQ);
			Q.inmigrar(deF);
		}

		if(F.revisarEstancamiento()){
			actualizarMejor(F, mejor);
		}
		if(Q.revisarEstancamiento()){
			actualizarMejor(Q, mejor);
		}

		if(csv.is_open()){
			escribirFila(csv, g, mejor, F, Q);
		}
	}

	cout << "--------------------------------- Mejor solución -------------------------\n";
	// mejor.print();
	cout << "Profit: " << mejor.z << "\n";
	mejor.printFeasibility();
	// if(mejor.K_f != 0 || mejor.D_f != 0){
	// 	cout << "No se encontró solución factible\n";
	// }
	cout << "Generaciones: " << g << "\n";
	cout << "Tiempo: " << tiempo(inicio) << " s\n";
	return 0;
}