#include "histogramplot.h"
#include "plot.h"
#include <algorithm>
#include <QPen>
#include <cmath>
#include <QDebug>
#include <QPalette>

HistogramPlot::HistogramPlot(QWidget *parent) : QwtPlot(parent)
{
    // --- 1. Dark Background Setup ---
    QColor darkBg(60, 60, 60);
    setCanvasBackground(darkBg);

    QwtPlotCanvas *plotCanvas = qobject_cast<QwtPlotCanvas*>(canvas());
    if (plotCanvas) {
        // Remove 3D borders to make it look flat
        plotCanvas->setFrameStyle(QFrame::NoFrame);
        plotCanvas->setBorderRadius(0);

        // Ensure palette matches background
        QPalette pal = plotCanvas->palette();
        pal.setColor(QPalette::Window, darkBg);
        plotCanvas->setPalette(pal);
    }

    // maximize space
    plotLayout()->setAlignCanvasToScales(true);
    plotLayout()->setCanvasMargin(0);

    // --- 2. Title ---
    QwtText titleText("Histogram");
    titleText.setFont(QFont("Segoe UI", 9, QFont::Bold));
    titleText.setColor(Qt::lightGray);
    setTitle(titleText);

    // --- 3. Grid ---
    // Make grid lines very subtle (transparent white)
    QPen pen_grid(Qt::SolidPattern, 1);
    pen_grid.setColor(QColor(255, 255, 255, 20));

    grid = new QwtPlotGrid();
    grid->setPen(pen_grid);
    grid->enableX(true);
    grid->enableY(false); // Only vertical lines usually look best
    grid->setZ(0);        // Draw grid behind bars
    grid->attach(this);

    // --- 4. Axes ---
    // We enable them for scaling to work, but we can hide the text if you want strict minimalism
    enableAxis(QwtPlot::xBottom, true);
    enableAxis(QwtPlot::yLeft, true);

    setAxisScaleDraw(QwtPlot::Axis::xBottom, new QwtPsToMScaleDraw()); // Apply the Meters implementation

    // Optional: Hide axis numbers for "sparkline" look
    // axisWidget(QwtPlot::xBottom)->setVisible(false);
    // axisWidget(QwtPlot::yLeft)->setVisible(false);

    // --- 5. Histogram Setup ---
    histogram = new QwtPlotHistogram();
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->setBaseline(0.0); // Crucial for correct Y-rendering

    // VISUAL FIX:
    // Light grey fill
    histogram->setBrush(QColor(200, 200, 200));
    // Use a thin border of the SAME color (or slightly darker).
    // Do NOT use Qt::NoPen, as thin bars might disappear.
    histogram->setPen(QPen(QColor(180, 180, 180), 1));

    histogram->setZ(1); // Draw on top of grid
    histogram->attach(this);

    setAutoReplot(false);
}

void HistogramPlot::setNumBins(int numBins) {
    if (numBins > 0) num_bins = numBins;
}

void HistogramPlot::setValues(const QVector<double> &values)
{
    // 1. Handle empty data
    if (values.isEmpty()) {
        histogram->setSamples(QVector<QwtIntervalSample>());
        replot();
        return;
    }

    // 2. Find Range
    auto result = std::minmax_element(values.begin(), values.end());
    double min = *result.first;
    double max = *result.second;

    // Fix flat data (min == max)
    if (qFuzzyCompare(min, max)) {
        max = min + 1.0;
        if (min == 0) { min = -0.5; max = 0.5; }
    }

    // 3. Calculate Bins
    QVector<double> binCounts(num_bins, 0);
    double range = max - min;

    // slightly larger step to include max value safely
    double step = (range * 1.00001) / num_bins;

    for (double v : values) {
        int index = static_cast<int>((v - min) / step);
        // Safety clamp
        if (index < 0) index = 0;
        if (index >= num_bins) index = num_bins - 1;

        binCounts[index]++;
    }

    // 4. Create Qwt Samples
    QVector<QwtIntervalSample> samples;
    samples.reserve(num_bins);
    for (int i = 0; i < num_bins; ++i) {
        double lower = min + (i * step);
        double upper = min + ((i + 1) * step);
        samples.append(QwtIntervalSample(binCounts[i], lower, upper));
    }

    histogram->setSamples(samples);

    // 5. Explicit Scaling (Fixes "Small/Invisible" issues)
    double maxCount = *std::max_element(binCounts.begin(), binCounts.end());
    if (maxCount == 0) maxCount = 1.0;

    // Force Y axis to start at 0 and end slightly above max count
    setAxisScale(QwtPlot::yLeft, 0.0, maxCount * 1.05);

    // Force X axis to fit data exactly
    setAxisScale(QwtPlot::xBottom, min, max);

    // 6. Force Update
    replot();
}
