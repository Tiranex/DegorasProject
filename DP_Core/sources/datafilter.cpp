#include "datafilter.h"

QVector<QPointF> DataFilter::applyMovingAverage(const QVector<QPointF> &input, int windowSize)
{
    if (input.isEmpty() || windowSize <= 1) return input;

    QVector<QPointF> output;
    output.reserve(input.size());

    int halfWindow = windowSize / 2;
    int n = input.size();

    for (int i = 0; i < n; ++i) {
        double sumY = 0.0;
        int count = 0;

        // Iteramos desde el inicio de la ventana hasta el final,
        // manejando los bordes (índices negativos o fuera de rango)
        for (int j = i - halfWindow; j <= i + halfWindow; ++j) {
            if (j >= 0 && j < n) {
                sumY += input[j].y();
                count++;
            }
        }

        // Mantenemos la X original, promediamos la Y
        if (count > 0) {
            output.append(QPointF(input[i].x(), sumY / count));
        } else {
            output.append(input[i]);
        }
    }

    return output;
}

QVector<QPointF> DataFilter::applyMedianFilter(const QVector<QPointF> &input, int windowSize)
{
    if (input.isEmpty() || windowSize <= 1) return input;

    QVector<QPointF> output;
    output.reserve(input.size());

    int halfWindow = windowSize / 2;
    int n = input.size();
    std::vector<double> windowValues;
    windowValues.reserve(windowSize);

    for (int i = 0; i < n; ++i) {
        windowValues.clear();

        // Recolectar vecinos
        for (int j = i - halfWindow; j <= i + halfWindow; ++j) {
            if (j >= 0 && j < n) {
                windowValues.push_back(input[j].y());
            }
        }

        // Ordenar para encontrar la mediana
        // std::nth_element es más eficiente que std::sort para encontrar la mediana
        size_t medianIndex = windowValues.size() / 2;
        std::nth_element(windowValues.begin(), windowValues.begin() + medianIndex, windowValues.end());

        double medianY = windowValues[medianIndex];
        output.append(QPointF(input[i].x(), medianY));
    }

    return output;
}

QVector<QPointF> DataFilter::applyExponentialSmoothing(const QVector<QPointF> &input, double alpha)
{
    if (input.isEmpty()) return input;

    // Clamp alpha entre 0 y 1
    alpha = std::max(0.0, std::min(1.0, alpha));

    QVector<QPointF> output;
    output.reserve(input.size());

    // El primer punto se mantiene igual para inicializar el filtro
    output.append(input.first());

    double prevY = input.first().y();

    for (int i = 1; i < input.size(); ++i) {
        double currentY = input[i].y();

        // Fórmula: Y_filtro = alpha * Y_actual + (1 - alpha) * Y_anterior_filtrada
        double newY = (alpha * currentY) + ((1.0 - alpha) * prevY);

        output.append(QPointF(input[i].x(), newY));
        prevY = newY;
    }

    return output;
}
