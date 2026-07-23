#ifndef RESPALDOS_H
#define RESPALDOS_H

typedef struct {
    char nombres[64][128];
    int total;
} ListaVersiones;

typedef struct {
    char items[64][512];
    int total;
} ColaSnapshot;

