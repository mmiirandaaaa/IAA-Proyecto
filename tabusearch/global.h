#ifndef GLOBAL_H
#define GLOBAL_H

#define INT_MAX 2147483647
#define INT_MIN -2147483648


// Librerías
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>

using namespace std;

// Variables globales del problema
extern int N, M, Q;
extern bool pos_coeffs;
extern vector<int> profit;
extern vector<vector<int>> coeff;
extern vector<int> bound;

// Parámetros del programa
extern int OPCION_Q, INSTANCIA, SEED, ISLA_SIZE;
extern string OPCION_COEFS, CSV;
extern double PC, PM;
extern int ELITE, K_MIG, MU, GR, GMAX;
extern double T_OBJ, GAMMA, W_MIN, TMAX;

// Generador de números aleatorios
extern mt19937 rng;

#endif