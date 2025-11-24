#ifndef HISTOGRAMPLOT_H
#define HISTOGRAMPLOT_H

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_histogram.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_text.h>
#include <QVector>
#include <QColor>
#include <QPen>

class HistogramPlot : public QwtPlot
{
    Q_OBJECT

public:
    explicit HistogramPlot(QWidget *parent = nullptr);
    void setValues(const QVector<double> &values);
    void setNumBins(int numBins);

private:
    QwtPlotHistogram *histogram;
    QwtPlotGrid *grid;
    QwtPlotMarker *mark_zero; // Example marker similar to your Plot class
    int num_bins = 100;
};

#endif // HISTOGRAMPLOT_H
