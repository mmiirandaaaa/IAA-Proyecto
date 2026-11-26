#include "global.h"

// Variables globales del problema
int N, M, Q;
bool pos_coeffs;
vector<int> profit;
vector<vector<int>> coeff;
vector<int> bound;

// Parámetros del programa
int OPCION_Q = 1;				// cantidad de restricciones de demanda: 1, 2 o 3
string OPCION_COEFS = "mixed";	// signo de los profits: mixed o positive
int INSTANCIA = 1;				// número de la instancia dentro del archivo
int SEED = 1;					// semilla del generador de números aleatorios
int ISLA_SIZE = 100;			// tamaño de la población de cada isla
string CSV = "";				// archivo CSV con la evolución por generación (vacío = no se escribe)
double PC = 0.9;		// probabilidad de cruce
double PM = 0.01;		// probabilidad de mutación (por gen)
int ELITE = 2;			// individuos elite
int K_MIG = 10;			// cada cuántas generaciones se migra
int MU = 2;				// número de migrantes
double T_OBJ = 0.5;		// proporción objetivo de factibilidad (isla F)
double GAMMA = 0.05;	// tasa de adaptación de los pesos (isla F)
double W_MIN = 0.05;	// peso mínimo de alpha y beta (isla F)
int GR = 50;			// generaciones sin mejorar para reiniciar
int GMAX = 1000;		// máximo de generaciones
double TMAX = 10.0;		// máximo de segundos

// Generador de numeros aleatorios
mt19937 rng;