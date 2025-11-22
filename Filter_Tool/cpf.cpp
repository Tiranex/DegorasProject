#include "cpf.h"
#include <QFile>
#include <QTextStream>
#include <cmath>

CPF::CPF() {}
CPF::~CPF() {}

bool CPF::load(const QString& filePath) {
    m_filePath = filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    // Aquí iría el parser real del formato CPF (ILRS)
    // Por ahora, simulamos que ha cargado correctamente
    return true;
}

long long CPF::calculateTwoWayTOF(int mjd, unsigned long long time_ns) {
    // --- SIMULACIÓN FÍSICA ---
    // En un caso real:
    // 1. Interpolas la posición (X,Y,Z) del satélite para ese instante exacto.
    // 2. Tienes la posición de la estación (Estación -> Satélite).
    // 3. Calculas distancia: D = sqrt((Xs-Xe)^2 + ...).
    // 4. Tiempo vuelo = (2 * D / c).

    // PARA QUE TU CÓDIGO FUNCIONE AHORA:
    // Vamos a devolver un valor ficticio o un pequeño offset para ver cambios en la gráfica.
    // Supongamos que el satélite está a unos 20ms (20,000,000,000 ps)

    double time_seconds = time_ns * 1e-9;

    // Simulamos una onda senoidal para que veas la curva cambiar en la gráfica
    long long simulated_tof = 20000000000LL + (long long)(sin(time_seconds) * 1000);

    return simulated_tof;
}
