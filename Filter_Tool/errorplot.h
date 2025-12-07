#pragma once

#include <QPen>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_magnifier.h>
#include <qwt/qwt_plot_panner.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_plot_curve.h>


#include <QDebug>
#include <QMouseEvent>
#include <QObject>
#include <QEvent>
#include <QPolygon>
#include <QObject>
#include <QColor>
#include <QList>
#include <QPointF>
#include <QDateTime>

#include <qwt/qwt_symbol.h>
#include <qwt/qwt_scale_draw.h>
#include <qwt/qwt_picker.h>
#include <qwt/qwt_picker_machine.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_painter.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_text_label.h>
#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_curve_fitter.h>

#include "plot.h"

/// @file errorplot.h
/// @brief Contiene la definición de la clase ErrorPlot, una gráfica especializada en residuos.

/**
 * @class ErrorPlot
 * @brief Implementa una gráfica Qwt especializada para visualizar desviaciones o errores.
 *
 * Hereda de la clase Plot y añade funcionalidades específicas para el análisis estadístico,
 * incluyendo el cálculo de desviación estándar (STD), el establecimiento dinámico de umbrales
 * (como 2.5*STD) mediante QwtPlotMarkers, y el ajuste simétrico de los ejes Y.
 */

class ErrorPlot : public Plot
{
    Q_OBJECT

public:

    /**
    * @brief Constructor de la gráfica de errores.
    *
    * Inicializa los ejes, configura el dibujado de escala de tiempo en el eje superior,
    * y crea los dos marcadores horizontales (mark_thresh1 y mark_thresh2)
    * que indicarán los umbrales de desviación.
    *
    * @param parent Widget padre de la gráfica.
    * @param title Título de la gráfica.
    */

    ErrorPlot( QWidget *parent = nullptr, QString title = "");

    /**
    * @brief Procesa, establece y ajusta dinámicamente la gráfica de errores.
    *
    * Esta función es el motor del análisis. Realiza la limpieza de datos,
    * el cálculo de la desviación estándar (STD), establece el umbral de filtrado (2.5*STD),
    * ajusta las escalas de los ejes y calcula métricas clave como el RMS.
    *
    * @param samples Vector de puntos (X: tiempo, Y: desviación o error en picosegundos).
    *
    * @post Los ejes X e Y están ajustados al rango de los datos y al umbral calculado,
    * los marcadores de umbral se han reubicado, y la gráfica se ha repintado.
    */

    void setSamples( const QVector<QPointF> &samples );

    /**
    * @brief Obtiene las muestras que se encuentran dentro de los umbrales de desviación.
    *
    * Esta función filtra el conjunto completo de muestras y devuelve únicamente
    * los puntos cuya desviación (coordenada Y) está comprendida entre los límites
    * definidos por los marcadores mark_thresh1 (umbral superior) y mark_thresh2
    * (umbral inferior), que a su vez representan típicamente +/- 2.5 veces la STD.
    *
    * @return QVector<QPointF> Un vector que contiene solo los puntos clasificados
    * como "inliers" (puntos dentro del umbral de calidad).
    */

    QVector<QPointF> getThreshSamples() const;

    /** @name Marcadores de Umbral de Desviación */
    ///@{
    QwtPlotMarker *mark_thresh1; ///< Marcador horizontal del umbral superior (e.g., +2.5*STD).
    QwtPlotMarker *mark_thresh2; ///< Marcador horizontal del umbral inferior (e.g., -2.5*STD).
    ///@}
};
