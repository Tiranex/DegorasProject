#include "errorplot.h"

#include "plot.h"

#include <LibDegorasBase/Statistics/measures.h>

ErrorPlot::ErrorPlot(QWidget *parent, QString title):
    Plot(parent, title)
{
    // Labels.
    QwtText yt("Deviation from polynomial fit (ps)");
    QwtText y2t("Deviation from polynomial fit (m)");
    //this->enableAxis(QwtPlot::Axis::xTop, true);
    this->enableAxis(QwtPlot::Axis::xBottom, false);
    //this->setAxisScaleDraw(QwtPlot::Axis::xTop, new QwtNanoseconds2TimeScaleDraw);
    //this->setAxisTitle(QwtPlot::Axis::xTop, "");


    QPen pen_mark_yellow(Qt::SolidPattern, 1, Qt::DashLine);
    pen_mark_yellow.setColor(QColor("yellow"));

    this->mark_thresh1 = new QwtPlotMarker;
    this->mark_thresh1->setLineStyle(QwtPlotMarker::LineStyle::HLine);
    this->mark_thresh1->setLinePen(pen_mark_yellow);
    this->mark_thresh1->setZ(1);
    this->mark_thresh1->attach(this);
    this->mark_thresh2 = new QwtPlotMarker;
    this->mark_thresh2->setLineStyle(QwtPlotMarker::LineStyle::HLine);
    this->mark_thresh2->setLinePen(pen_mark_yellow);
    this->mark_thresh2->setZ(1);
    this->mark_thresh2->attach(this);



    // Others.
    // this->adjust_curve->detach();
}

void ErrorPlot::setSamples(const QVector<QPointF>& samples)
{
    // Borramos los puntos anteriores.
    QwtSLRArraySeriesData *selected_data = static_cast<QwtSLRArraySeriesData *>(this->selected_curve->data());
    QwtSLRArraySeriesData *curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());

    // Borramos los puntos anteriores.
    selected_data->clear();
    curve_data->clear();

    this->plot_curve->setSamples(samples);

    selected_data = static_cast<QwtSLRArraySeriesData *>(this->selected_curve->data());
    curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());


    // Marcas.
    this->mark_0->setValue(0, 0);
    this->mark_inicio->setValue(plot_curve->minXValue(), 0);
    this->mark_final->setValue(plot_curve->maxXValue(), 0);
    // Margen para trabajar mejor
    double margen_x = (plot_curve->maxXValue()-plot_curve->minXValue())/80.0;
    double margen_y = (plot_curve->maxYValue()-plot_curve->minYValue())/50.0;

    // Ajustamos Axis y repintamos.
    std::vector<double> y_orig;
    for(const auto& p : samples){
        y_orig.push_back(p.y());
    }

    double std = dpbase::stats::measures::stddev(std::vector(y_orig.begin(), y_orig.end()));
    double thresh = 2.5*std;
    qInfo() << thresh;
    this->mark_thresh1->setValue(0, thresh);
    this->mark_thresh2->setValue(0, thresh*(-1));

    this->setAxisScale(QwtPlot::Axis::xBottom, plot_curve->minXValue()-margen_x, plot_curve->maxXValue()+margen_x);
    this->setAxisScale(QwtPlot::Axis::xTop, plot_curve->minXValue()-margen_x, plot_curve->maxXValue()+margen_x);




    //curve_data->clear();

    QVector<double> y_vector;
    QVector<QPointF> selected;
    QVector<QPointF> noselected;

    for (const auto& p : std::as_const(samples))
    {
        y_vector.append(p.y());
    }

    qInfo() << thresh;

    this->setAxisScale(QwtPlot::Axis::yLeft, -thresh - margen_y, thresh + margen_y);
    this->setAxisScale(QwtPlot::Axis::yRight, -thresh - margen_y, thresh + margen_y);


    for (const auto& p : std::as_const(samples))
    {
//        if(qAbs(p.y())>=thresh)
//            curve_data->append(p);
//        else
            selected_data->append(p);
    }
    double light_speed = 0.000299792458; // m/ps

    const auto& ydata = selected_data->getYData();
    const auto& data = std::vector(ydata.begin(), ydata.end());
    auto rms = dpbase::stats::measures::rms(data);
    qInfo()<<"Points -> "<< data.size();
    qInfo()<<"STD -> "<< dpbase::stats::measures::stddev(data);
    qInfo()<<"RMS meters -> "<< rms * light_speed;
    qInfo()<<"RMS ps -> "<< rms;

    //this->adjust_curve->detach();


    this->replot();
}

QVector<QPointF> ErrorPlot::getThreshSamples() const
{
    QVector<QPointF> thresh_points;
    QwtSLRArraySeriesData *curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());
    QVector<QPointF> samples = curve_data->samples();
    std::copy_if(std::make_move_iterator(samples.begin()), std::make_move_iterator(samples.end()),
                 std::back_inserter(thresh_points),
                 [this](const auto& p)
    {return p.y() > this->mark_thresh2->value().y() && p.y() < this->mark_thresh1->value().y();});

    return thresh_points;

}
