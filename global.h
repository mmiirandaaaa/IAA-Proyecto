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

using namespace std;

// Variables globales del problema
extern int N, M, Q;
extern bool pos_coeffs;
extern vector<int> profit;
extern vector<vector<int>> coeff;
extern vector<int> bound;

// Parámetros del programa
extern int ISLA_SIZE;

// Generador de números aleatorios
extern mt19937 rng;

#endif