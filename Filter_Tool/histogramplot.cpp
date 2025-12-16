#include "histogramplot.h"
#include "plot.h"
#include <algorithm>
#include <QPen>
#include <cmath>
#include <QDebug>
#include <QPalette>

HistogramPlot::HistogramPlot(QWidget *parent) : QwtPlot(parent)
{
    // --- 1. Style Setup (Same as before) ---
    canvas()->setStyleSheet("border: 2px solid Black;"
                            "border-radius: 15px;"
                            "background-color: rgb(70,70,70);");

    QwtPlotCanvas *plotCanvas = qobject_cast<QwtPlotCanvas*>(canvas());
    if (plotCanvas) {
        plotCanvas->setFrameStyle(QFrame::NoFrame);
        QPalette pal = plotCanvas->palette();
        pal.setColor(QPalette::Window, QColor(70,70,70));
        plotCanvas->setPalette(pal);
    }

    plotLayout()->setAlignCanvasToScales(true);
    plotLayout()->setCanvasMargin(0);

    // Remove Title to match the requested clean look
    setTitle("");

    // --- 2. Grid ---
    QPen pen_grid(Qt::SolidPattern, 1);
    pen_grid.setColor(QColor(255, 255, 255, 20));

    grid = new QwtPlotGrid();
    grid->setPen(pen_grid);
    grid->enableX(false);
    grid->enableY(true);  // Horizontal lines for rotated histogram
    grid->setZ(0);
    grid->attach(this);

    // --- 3. Axes Configuration (THE FIX) ---
    enableAxis(QwtPlot::xBottom, true);
    enableAxis(QwtPlot::yLeft, true);

    // A. Apply the specific ScaleDraw to handle the Unit conversion (ps -> m)
    setAxisScaleDraw(QwtPlot::yLeft, new QwtPsToMScaleDraw());

    // B. Match the Font exactly (Open Sans, Size 6)
    setAxisFont(QwtPlot::Axis::yLeft, QFont("Open Sans", 6));
    setAxisFont(QwtPlot::Axis::xBottom, QFont("Open Sans", 6));

    // C. Match the Tick Density (MaxMajor 20)
    // This ensures Qwt calculates the same steps (intervals) as the filter plot
    setAxisMaxMajor(QwtPlot::yLeft, 20);
    setAxisMaxMajor(QwtPlot::xBottom, 10); // Standardize bottom axis too

    // --- 4. Histogram Setup ---
    histogram = new QwtPlotHistogram();
    histogram->setOrientation(Qt::Horizontal); // Rotated
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->setBaseline(0.0);

    histogram->setBrush(QColor(200, 200, 200));
    histogram->setPen(QPen(QColor(180, 180, 180), 1));
    histogram->setZ(1);
    histogram->attach(this);

    QwtText xt("Nº de coincidencias");
    xt.setFont(QFont("Open Sans Semibold", 8));
    this->setAxisTitle(QwtPlot::Axis::xBottom, xt);

    setAutoReplot(false);

    this->magnifier = new QwtSLRPlotMagnifier(canvas());
    magnifier->setMouseButton(Qt::NoButton);
    magnifier->setAxisEnabled(QwtPlot::Axis::xBottom, false);
    magnifier->setAxisEnabled(QwtPlot::Axis::xTop, false);
    magnifier->setEnabled(true);
}


void HistogramPlot::setValues(const QVector<double> &values)
{
    if (values.isEmpty()) {
        histogram->setSamples(QVector<QwtIntervalSample>());
        replot();
        return;
    }

    // 2. Find Range
    auto result = std::minmax_element(values.begin(), values.end());
    double min = *result.first;
    double max = *result.second;

    if (qFuzzyCompare(min, max)) {
        max = min + 1.0;
        if (min == 0) { min = -0.5; max = 0.5; }
    }

    // 3. Calculate Bins
    QVector<double> binCounts(num_bins, 0);
    double range = max - min;
    double step = (range * 1.00001) / num_bins;

    for (double v : values) {
        int index = static_cast<int>((v - min) / step);
        if (index < 0) index = 0;
        if (index >= num_bins) index = num_bins - 1;
        binCounts[index]++;
    }

    // 4. Create Qwt Samples
    // When Orientation is Horizontal:
    // QwtIntervalSample(value, min, max) -> Value determines length (X), Min/Max determine thickness (Y)
    QVector<QwtIntervalSample> samples;
    samples.reserve(num_bins);
    for (int i = 0; i < num_bins; ++i) {
        double lower = min + (i * step);
        double upper = min + ((i + 1) * step);
        // The 'value' is the count
        samples.append(QwtIntervalSample(binCounts[i], lower, upper));
    }

    histogram->setSamples(samples);

    // 5. Explicit Scaling (ROTATED)
    double maxCount = *std::max_element(binCounts.begin(), binCounts.end());
    if (maxCount == 0) maxCount = 1.0;

    // ROTATION MOD:
    // yLeft gets the Data Range (Meters)
    // xBottom gets the Count Range
    setAxisScale(QwtPlot::xBottom, 0.0, maxCount * 1.05);
    setAxisScale(QwtPlot::yLeft, min, max);

    replot();
}

void HistogramPlot::setNumBins(int numBins) {
    if (numBins > 0) num_bins = numBins;
}
