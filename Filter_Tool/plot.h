#pragma once

#include <QGenericMatrix>

#include <QPen>
#include <qwt/qwt_scale_draw.h>
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
#include <QStack>

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

#include "qwt_slrplot_picker.h"

class QwtPlotCurve;
class QwtSymbol;

struct PlotState {
    QVector<QPointF> candidatePoints; // Data from plot_curve
    QVector<QPointF> selectedPoints;  // Data from selected_curve
};

class QwtPsToMScaleDraw: public QwtScaleDraw
{
public:
    QwtPsToMScaleDraw(){}

    QwtText label(double v) const
    {
        double light_speed = 0.000299792458; // m/ps
        double value = v*light_speed;
        return QString::number(value, 'd', 3);
    }
};

class QwtNanoseconds2TimeScaleDraw: public QwtScaleDraw
{
public:
    QwtNanoseconds2TimeScaleDraw(){}

    QwtText label(double v) const
    {
        double value_seconds = v*0.000000001;
        return QDateTime::fromSecsSinceEpoch(uint(value_seconds)).toUTC().toString("hh:mm:ss");
    }
};


class QwtSLRArraySeriesData: public QwtArraySeriesData<QPointF>
{
public:
    QwtSLRArraySeriesData(){}

    QwtSLRArraySeriesData(const QVector<QPointF>& v)
    {
        QwtArraySeriesData::setSamples(v);
    }

    inline QRectF boundingRect() const
    {
        if ( this->cachedBoundingRect.width() < 0.0 )
            this->cachedBoundingRect = qwtBoundingRect( *this );

        return this->cachedBoundingRect;
    }
    inline void append(const QPointF &point){m_samples += point;}
    inline void append(const QVector<QPointF>& v){m_samples.append(v);}
    inline void remove(int i){m_samples.removeAt(i);}

    inline void remove(const QVector<QPointF>& v)
    {
        foreach(QPointF p, v)
            m_samples.removeOne(p);
    }

    inline const QVector<double> getYData() const
    {
        QVector<double> y_data;
        foreach(QPointF p, this->samples())
            y_data.append(p.y());
        return y_data;
    }

    inline const QVector<double> getXData() const
    {
        QVector<double> x_data;
        foreach(QPointF p, this->samples())
            x_data.append(p.y());
        return x_data;
    }

    void clear()
    {
        m_samples.clear();
        m_samples.squeeze();
        this->cachedBoundingRect = QRectF( 0.0, 0.0, -1.0, -1.0 );
    }


};

enum class PlotMode {
    Navigation, // Zoom y Pan (comportamiento por defecto)
    Selection,  // "Keep Inside": Mantiene lo de dentro, borra lo de fuera
    Deletion    // "Remove Inside": Borra lo de dentro
};

class Plot : public QwtPlot
{
    Q_OBJECT

    class QScaleDraw : public QwtScaleDraw
    {
    public:

        explicit QScaleDraw(bool s_notation = false)
        {
            this->notation = s_notation;
        }

        QwtText label(double value) const override
        {
            if (notation)
                return QwtScaleDraw::label(value);
            else
                return QwtText(QString::number(value, 'f', 0));
        }


    private:

        bool notation;
    };

    class QwtSLRPlotMagnifier:public QwtPlotMagnifier
    {
    public:
        QwtSLRPlotMagnifier(QWidget* p): QwtPlotMagnifier(p){}
        void rescale(double factor);
        ~QwtSLRPlotMagnifier(){}
    };

public:
    Plot( QWidget *parent = nullptr, QString title = "");
    void setSymbol( QwtSymbol * );
    void setSamples( const QVector<QPointF> &samples );
    void selectPoints(const QPolygonF& pol);
    void setBinSize(int bin_size);
    bool undo();
    bool redo();

    void pushCurrentStateToUndo();
    // Conecta estos slots a tus botones externos
    void setNavigationMode();
    void setSelectionMode();
    void setDeletionMode();

    // Botón "Deseleccionar / Resetear"
    void resetSelection();
    void updateFit();

    virtual const QVector<QPointF> getSelectedSamples() const
    {
        QwtSLRArraySeriesData *curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());
        return curve_data->samples();
    }

    const QVector<QPointF> getFitSamples() const
    {
        QwtSLRArraySeriesData *adjust_data = static_cast<QwtSLRArraySeriesData *>(this->adjust_curve->data());
        return adjust_data->samples();
    }

    const QVector<QPointF> getFitErrors() const
    {
        return this->points_fiterrors;
    }

    QwtPlotPanner* getPanner() const
    {
        return this->panner;
    }

    QwtPlotCurve *selected_curve;
    QwtPlotCurve *error_curve;

public slots:
    void setPickingEnabled(bool enabled);

signals:
    void fitCalculated(const QVector<QPointF>& fit);
    void selectionChanged();
    void startedPicking();
    void finishedPicking();


protected:
    bool picking;
    QVector<QPointF> original_curve;
    QVector<QPointF> points_fiterrors;
    QwtPlotCurve *plot_curve;
    QwtPlotCurve *adjust_curve;
    QwtPlotMarker *mark_0;
    QwtPlotMarker *mark_inicio;
    QwtPlotMarker *mark_final;
    QwtPlotPanner* panner;
    QwtSLRPlotMagnifier *magnifier;
    QwtSLRPlotPicker *picker;
    int bin_size;
    QStack<PlotState> m_undoStack;
    QStack<PlotState> m_redoStack;
    PlotMode currentMode;

    // Helper to capture current state
    void applyState(const PlotState& state);
};

