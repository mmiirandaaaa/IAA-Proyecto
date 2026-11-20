#include "global.h"
#include "individuo.h"

// Inicialización aleatoria
individuo::individuo(){
	x.resize(N);
	uniform_int_distribution<> U(0,1);
	for(int i=0; i<N; i++){
		x[i] = U(rng);
	}
	updateFeasibility();
}

// Inicialización a partir de un vector
individuo::individuo(vector<int> &v){
	x = v;
	updateFeasibility();
}

void individuo::print(){
	for(int i=0; i<N; i++){
		cout << x[i] << " ";
	}

	if(K_f == 0 && D_f == 0){
		cout << " -> KFDF (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
	else if(K_f == 0 && D_f > 0){
		cout << " -> KFDI (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
	else if(K_f > 0 && D_f == 0){
		cout << " -> KIDF (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
	else if(K_f > 0 && D_f > 0){
		cout << " -> KIDI (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
}

void individuo::printFeasibility(){
	if(K_f == 0 && D_f == 0){
		cout << "KFDF (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
	else if(K_f == 0 && D_f > 0){
		cout << "KFDI (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
	else if(K_f > 0 && D_f == 0){
		cout << "KIDF (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
	else if(K_f > 0 && D_f > 0){
		cout << "KIDI (K_f: " << K_f << ", D_f: " << D_f << ")\n";
	}
}


// Calcula el profit y la violación normalizada de cada familia de restricciones
void individuo::updateFeasibility(){
	z = 0;
	for(int i=0; i<N; i++){
		z += profit[i]*x[i];
	}

	K_f = 0; D_f = 0;
	for(int j=0; j<M; j++){
		double c = 0.0;
		for(int i=0; i<N; i++){
			c += coeff[j][i]*x[i];
		}
		K_f += (1.0/bound[j])*max(0.0, c - bound[j]);
	}

	for(int j=M; j<M+Q; j++){
		double c = 0.0;
		for(int i=0; i<N; i++){
			c += coeff[j][i]*x[i];
		}
		D_f += (1.0/bound[j])*max(0.0, bound[j] - c);
	}
}

// Cruce uniforme: cada gen del hijo viene de uno de los dos padres con probabilidad 1/2
vector<int> individuo::cruce(individuo &otro){
	uniform_int_distribution<> U(0,1);
	vector<int> v(N);
	for(int i=0; i<N; i++){
		v[i] = U(rng) ? otro.x[i] : x[i];
	}
	return v;
}

// Mutación: bit flip en cada gen con probabilidad PM
void individuo::mutar(vector<int> &v){
	uniform_real_distribution<> U(0.0, 1.0);
	for(int i=0; i<N; i++){
		if(U(rng) < PM){
			v[i] = 1 - v[i];
		}
	}
}

// int getFitness();

// double individuo::getKFeasibility(){
// 	return K_f;
// }
// double individuo::getDFeasibility(){
// 	return D_f;
// }