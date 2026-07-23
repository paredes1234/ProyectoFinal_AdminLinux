#ifndef PROCESOS_H
#define PROCESOS_H

typedef struct {
    int pid;
    int ppid;
    char nombre[256];
    char estado[16];
    long mem_kb;
    double cpu_pct;
} InfoProceso;

typedef struct {
    InfoProceso *items;
    int total;
} ListaProcesos;
