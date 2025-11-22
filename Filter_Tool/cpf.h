#ifndef CPF_H
#define CPF_H


#pragma once
#include <QString>
#include <QVector>


struct CPFPoint {
    int mjd;
    double seconds;
    double x, y, z;
};

class CPF {
public:
    CPF();
    ~CPF();


    bool load(const QString& filePath);

    // Calcula el tiempo de vuelo (ida y vuelta) en PICOSEGUNDOS
    // Recibe: MJD (día), tiempo del día (nanosegundos)
    long long calculateTwoWayTOF(int mjd, unsigned long long time_ns);

private:
    QString m_filePath;
    QVector<CPFPoint> m_ephemeris; // Lista de puntos de la órbita
};

#endif // CPF_H
