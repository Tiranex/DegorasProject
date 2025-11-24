#ifndef CPF_PREDICTOR_H
#define CPF_PREDICTOR_H


#pragma once

#include <QString>

#include <LibDegorasSLR/ILRS/formats/cpf/cpf.h>
#include <LibDegorasSLR/ILRS/formats/cpf/records/cpf_data.h>

class CPFPredictor {
public:

    CPFPredictor();
    ~CPFPredictor();

    bool load(const QString& filePath);

    long long calculateTwoWayTOF(int mjd, unsigned long long time_ns);

    // Configurar coordenadas de la estación (San Fernando por defecto)
    void setStationCoordinates(double x, double y, double z);

private:

    dpslr::ilrs::cpf::CPF m_cpfLib;
    double st_x, st_y, st_z;
    double interpolateLagrange(double target_s, int start_idx, int comp);
};

#endif // CPF_H
