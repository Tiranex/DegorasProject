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

class ErrorPlot : public Plot
{
    Q_OBJECT

public:
    ErrorPlot( QWidget *parent = nullptr, QString title = "");
    void setSamples( const QVector<QPointF> &samples );

    QVector<QPointF> getThreshSamples() const;
    QwtPlotMarker *mark_thresh1;
    QwtPlotMarker *mark_thresh2;
};
