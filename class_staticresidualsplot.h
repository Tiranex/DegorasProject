#pragma once

#include <qwt/qwt_plot.h>
#include <qwt/qwt_series_data.h>
#include <qwt/qwt_plot_curve.h>



class StaticResidualsPlot : public QwtPlot
{
public:

    enum class DataTypeEnum
    {
        GENERIC,
        DATA
    };

    StaticResidualsPlot(QWidget* parent = nullptr);
    ~StaticResidualsPlot() override = default;

    void fastPopulate(const std::vector<double>& xdata, const std::vector<double> &ydata, DataTypeEnum type);
    void setAdjustCurveVisible(bool visible);

    void clearAll();
    void clearData();
    void clearGeneric();

protected:

    QwtPlotCurve *series_all;
    QwtPlotCurve *series_data;
    QwtPlotCurve *adjust_data;

};
