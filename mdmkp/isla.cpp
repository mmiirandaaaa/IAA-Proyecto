#include "global.h"
#include "individuo.h"
#include "isla.h"

// Constructor aleatorio
isla::isla(char t){
	tipo = t;
	alpha = 0.5; beta = 0.5;
	lambda = 0.0;
	for(int j=0; j<N; j++){
		lambda += max(profit[j], 0);
	}
	mejor_valor = -1e18;
	estancado = 0;
	poblacion.resize(ISLA_SIZE);
}

void isla::print(){
	for(int i=0; i<ISLA_SIZE; i++){
		cout << "[" << i << "]: ";
		poblacion[i].print();
	}
	cout << "\n";
}

// Fitness de selección y reemplazo (mayor es mejor)
// Isla F: -phi (Ec. 10), isla Q: f_Q (Ec. 12)
double isla::fitness(individuo &a){
	if(tipo == 'F'){
		return -(alpha*a.K_f + beta*a.D_f);
	}
	return a.z - lambda*(a.K_f + a.D_f);
}

// Valor para migrar y reiniciar (mayor es mejor)
// Isla F: -(K_f + D_f), isla Q: f_Q
double isla::criterio(individuo &a){
	if(tipo == 'F'){
		return -(a.K_f + a.D_f);
	}
	return fitness(a);
}

// Mejor valor de la isla: en la F el menor K_f + D_f, en la Q el mayor f_Q
double isla::mejorValor(){
	double v = criterio(poblacion[0]);
	for(int i=1; i<ISLA_SIZE; i++){
		v = max(v, criterio(poblacion[i]));
	}
	return (tipo == 'F') ? -v : v;
}

// Índices de la población de mejor a peor (según fitness, o según criterio si migracion es true)
// En empates gana el de mayor profit
vector<int> isla::ordenar(bool migracion){
	vector<double> v(ISLA_SIZE);
	vector<int> idx(ISLA_SIZE);
	for(int i=0; i<ISLA_SIZE; i++){
		v[i] = migracion ? criterio(poblacion[i]) : fitness(poblacion[i]);
		idx[i] = i;
	}
	stable_sort(idx.begin(), idx.end(), [&](int a, int b){
		if(v[a] != v[b]){
			return v[a] > v[b];
		}
		return poblacion[a].z > poblacion[b].z;
	});
	return idx;
}

// Torneo binario, los empates se resuelven al azar
int isla::torneo(){
	uniform_int_distribution<> U(0, ISLA_SIZE-1);
	uniform_int_distribution<> C(0, 1);
	int a = U(rng), b = U(rng);
	double fa = fitness(poblacion[a]), fb = fitness(poblacion[b]);
	if(fa > fb){
		return a;
	}
	if(fb > fa){
		return b;
	}
	return C(rng) ? a : b;
}

// Genera un hijo: selección, cruce uniforme (con probabilidad PC) y mutación
individuo isla::hijo(){
	individuo &p1 = poblacion[torneo()];
	individuo &p2 = poblacion[torneo()];
	uniform_real_distribution<> U(0.0, 1.0);
	vector<int> v;
	if(U(rng) < PC){
		v = p1.cruce(p2);
	}
	else{
		v = p1.x;
	}
	p1.mutar(v);
	return individuo(v);
}

// Una generación: los ELITE mejores pasan directo y el resto son hijos
void isla::generacion(){
	vector<int> idx = ordenar(false);
	vector<individuo> nueva;
	for(int i=0; i<ELITE; i++){
		nueva.push_back(poblacion[idx[i]]);
	}
	while((int)nueva.size() < ISLA_SIZE){
		nueva.push_back(hijo());
	}
	poblacion = nueva;
}

// Adapta alpha y beta según la proporción de individuos factibles (Ec. 11), solo isla F
void isla::actualizarPesos(){
	int nK = 0, nD = 0;
	for(int i=0; i<ISLA_SIZE; i++){
		if(poblacion[i].K_f == 0){
			nK++;
		}
		if(poblacion[i].D_f == 0){
			nD++;
		}
	}
	double pK = (double)nK/ISLA_SIZE;
	double pD = (double)nD/ISLA_SIZE;
	double sK = (pK < T_OBJ) ? 1.0 : -1.0;
	double sD = (pD < T_OBJ) ? 1.0 : -1.0;

	alpha = alpha*pow(1.0 + GAMMA, sK);
	beta = beta*pow(1.0 + GAMMA, sD);

	// Se normaliza para que sumen 1 y se recorta a [W_MIN, 1-W_MIN]
	alpha = alpha/(alpha + beta);
	alpha = min(max(alpha, W_MIN), 1.0 - W_MIN);
	beta = 1.0 - alpha;
}

// Los MU mejores según criterio
vector<individuo> isla::emigrantes(){
	vector<int> idx = ordenar(true);
	vector<individuo> m;
	for(int i=0; i<MU; i++){
		m.push_back(poblacion[idx[i]]);
	}
	return m;
}

// Los migrantes reemplazan a los MU peores según fitness
void isla::inmigrar(vector<individuo> &migrantes){
	vector<int> idx = ordenar(false);
	for(int i=0; i<MU; i++){
		poblacion[idx[ISLA_SIZE-1-i]] = migrantes[i];
	}
}

// Cuenta las generaciones sin mejorar y reinicia la isla si llega a GR
// La isla F solo se reinicia mientras no tenga individuos factibles
bool isla::revisarEstancamiento(){
	vector<int> idx = ordenar(true);
	double valor = criterio(poblacion[idx[0]]);

	if(tipo == 'F' && valor == 0){
		estancado = 0;
		mejor_valor = valor;
		return false;
	}

	if(valor > mejor_valor){
		mejor_valor = valor;
		estancado = 0;
	}
	else{
		estancado++;
	}

	if(estancado >= GR){
		reiniciar(idx[0]);
		return true;
	}
	return false;
}

// Reinicializa toda la población menos el individuo en la posición cual
void isla::reiniciar(int cual){
	individuo mejor = poblacion[cual];
	for(int i=0; i<ISLA_SIZE; i++){
		poblacion[i] = individuo();
	}
	poblacion[0] = mejor;
	estancado = 0;
	mejor_valor = criterio(mejor);
}