#include "global.h"

// Variables globales del problema
int N, M, Q;
bool pos_coeffs;
vector<int> profit;
vector<vector<int>> coeff;
vector<int> bound;

// Parámetros del programa
int ISLA_SIZE;

// Generador de numeros aleatorios
mt19937 rng;