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

// individuo(vector<int> &v);
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

// void updateFitness();

void individuo::updateFeasibility(){
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

// int getFitness();

// double individuo::getKFeasibility(){
// 	return K_f;
// }
// double individuo::getDFeasibility(){
// 	return D_f;
// }