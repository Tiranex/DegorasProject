#include "class_staticresidualsplot.h"

#include <LibDegorasBase/Statistics/fitting.h>

StaticResidualsPlot::StaticResidualsPlot(QWidget *parent) :
    QwtPlot(parent)
{
    this->setAutoReplot(false);
    this->canvas()->setStyleSheet("border: 2px solid #262626;"
                                  "border-radius: 15px;"
                                  "background-color: rgb(70,70,70);");

    // All points curve.
    this->series_all = new QwtPlotCurve;
    this->series_all->setPen(QColor("White"));
    this->series_all->setZ(4);
    this->series_all->attach(this);
    // Data curve render and style.
    this->series_all->setRenderThreadCount(0);
    this->series_all->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->series_all->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    this->series_all->setPaintAttribute(QwtPlotCurve::ImageBuffer, true);
    this->series_all->setStyle(QwtPlotCurve::CurveStyle::Dots);
    //this->series_all->setSymbol(new QwtSymbol(QwtSymbol::Hexagon, QColor(240,240,240), Qt::NoPen, QSize(1,1)));

    // Data points curve.
    this->series_data = new QwtPlotCurve;
    this->series_data->setPen(QColor("Orange"));
    this->series_data->setZ(5);
    this->series_data->attach(this);
    // Data curve render and style.
    this->series_data->setRenderThreadCount(0);
    this->series_data->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->series_data->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    this->series_data->setPaintAttribute(QwtPlotCurve::ImageBuffer, true);
    this->series_data->setStyle(QwtPlotCurve::CurveStyle::Dots);
    //this->series_data->setSymbol(new QwtSymbol(QwtSymbol::Hexagon, QColor(240,240,240), Qt::NoPen, QSize(1,1)));

    // Adjust points curve.
    this->adjust_data = new QwtPlotCurve;
    this->adjust_data->setPen(QColor("Blue"));
    this->adjust_data->setZ(6);
    this->adjust_data->attach(this);
    // Data curve render and style.
    this->adjust_data->setRenderThreadCount(0);
    this->adjust_data->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->adjust_data->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    this->adjust_data->setPaintAttribute(QwtPlotCurve::ImageBuffer, true);
    this->adjust_data->setStyle(QwtPlotCurve::CurveStyle::Lines);
    //this->adjust_data->setSymbol(new QwtSymbol(QwtSymbol::Hexagon, QColor(240,240,240), Qt::NoPen, QSize(1,1)));
}


void StaticResidualsPlot::fastPopulate(const std::vector<double>& xdata, const std::vector<double> &ydata,
                                       DataTypeEnum type)
{
    switch(type)
    {
    // All the points.
    case StaticResidualsPlot::DataTypeEnum::GENERIC:
        // Set all the samples.
        this->series_all->setRawSamples(xdata.data(), ydata.data(),xdata.size());
        break;

    // Data points.
    case StaticResidualsPlot::DataTypeEnum::DATA:
        // Auxiliar containers.
        QVector<QPointF> adjust_points;
        // Set the data samples.
        this->series_data->setRawSamples(xdata.data(), ydata.data(),xdata.size());
        // Generate the fit curve.
        auto coefs = dpbase::stats::polynomialFit(xdata, ydata, 9, {}, dpbase::stats::types::PolyFitRobustMethod::NO_ROBUST);
        for (std::size_t i = 0; i < xdata.size(); i++)
            adjust_points.push_back({xdata[i], dpbase::stats::applyPolynomial(coefs, xdata[i])});
        this->adjust_data->setSamples(adjust_points);
        break;
    }

    // Replot the plot.
    this->replot();
}

void StaticResidualsPlot::setAdjustCurveVisible(bool visible)
{
    this->adjust_data->setVisible(visible);
    this->replot();
}

void StaticResidualsPlot::clearAll()
{
    this->series_all->setSamples({});
    this->series_data->setSamples({});
    this->adjust_data->setSamples({});
    this->replot();
}

void StaticResidualsPlot::clearData()
{
    this->series_data->setSamples({});
    this->adjust_data->setSamples({});
    this->replot();
}

void StaticResidualsPlot::clearGeneric()
{
    this->series_all->setSamples({});
    this->replot();
}
