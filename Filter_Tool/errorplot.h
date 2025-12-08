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
/// @brief Contains the definition of the ErrorPlot class, a specialized residual plot.

/**
 * @class ErrorPlot
 * @brief Implements a specialized Qwt plot for visualizing deviations or errors.
 *
 * It inherits from the Plot class and adds specific functionalities for statistical analysis,
 * including the calculation of the Standard Deviation (STD), the dynamic establishment of thresholds
 * (such as 2.5*STD) using QwtPlotMarkers, and the symmetric adjustment of the Y-axes.
 */

class ErrorPlot : public Plot
{
    Q_OBJECT

public:

    /**
 * @brief Constructor for the error plot.
 *
 * Initializes the axes, configures the time scale drawing on the top axis,
 * and creates the two horizontal markers (mark_thresh1 and mark_thresh2)
 * that will indicate the deviation thresholds.
 *
 * @param parent Parent widget of the plot.
 * @param title Title of the plot.
 */

    ErrorPlot( QWidget *parent = nullptr, QString title = "");

    /**
 * @brief Processes, sets, and dynamically adjusts the error plot.
 *
 * This function is the engine of the analysis. It performs data cleanup,
 * the calculation of the **Standard Deviation (STD)**, sets the filtering threshold (**2.5*STD**),
 * adjusts the axis scales, and calculates key metrics like the **RMS** (Root Mean Square).
 *
 * @param samples Vector of points (X: time, Y: deviation or error in picoseconds).
 *
 * @post The X and Y axes are adjusted to the data range and the calculated threshold,
 * the threshold markers have been relocated, and the plot has been redrawn.
 */

    void setSamples( const QVector<QPointF> &samples );

    /**
 * @brief Retrieves the samples that fall within the deviation thresholds.
 *
 * This function filters the complete set of samples and returns only
 * the points whose deviation (Y coordinate) is comprised between the limits
 * defined by the mark_thresh1 (upper threshold) and mark_thresh2
 * (lower threshold) markers, which typically represent +/- 2.5 times the STD.
 *
 * @return QVector<QPointF> A vector containing only the points classified
 * as "inliers" (points within the quality threshold).
 */

    QVector<QPointF> getThreshSamples() const;

    /** @name Deviation Threshold Markers */
    ///@{
    QwtPlotMarker *mark_thresh1; ///< Horizontal marker for the upper threshold (e.g., +2.5*STD).
    QwtPlotMarker *mark_thresh2; ///< Horizontal marker for the lower threshold (e.g., -2.5*STD).
    ///@}
};
