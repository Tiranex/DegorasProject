#ifndef DATAFILTER_H
#define DATAFILTER_H

#include <QVector>
#include <QPointF>
#include <vector>
#include <algorithm>
#include <cmath>
#include "dpcore_global.h"

class DP_CORE_EXPORT DataFilter
{
public:
    /**
     * 1. Media Móvil Simple (Simple Moving Average - SMA)
     * Suaviza la señal promediando vecinos. Reduce ruido aleatorio blanco.
     * @param input: Datos originales.
     * @param windowSize: Tamaño de la ventana (número de puntos impares, ej: 3, 5, 7).
     */
    static QVector<QPointF> applyMovingAverage(const QVector<QPointF> &input, int windowSize);

    /**
     * 2. Filtro de Mediana (Median Filter)
     * Excelente para eliminar "Salt and Pepper noise" (picos o valores atípicos)
     * preservando los bordes de la señal mejor que la media.
     * @param input: Datos originales.
     * @param windowSize: Tamaño de la ventana (debe ser impar).
     */
    static QVector<QPointF> applyMedianFilter(const QVector<QPointF> &input, int windowSize);

    /**
     * 3. Suavizado Exponencial (Exponential Moving Average - EMA)
     * Filtro recursivo de paso bajo. Da más peso a los datos recientes.
     * Muy rápido y bueno para series temporales.
     * @param input: Datos originales.
     * @param alpha: Factor de suavizado [0.0 - 1.0].
     *        - alpha pequeño (ej. 0.1): Mucho suavizado, respuesta lenta.
     *        - alpha grande (ej. 0.9): Poco suavizado, sigue fielmente la señal.
     */
    static QVector<QPointF> applyExponentialSmoothing(const QVector<QPointF> &input, double alpha);
};

#endif // DATAFILTER_H
