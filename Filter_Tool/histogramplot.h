/// @file HistogramPlot.h
/// @brief Defines the **HistogramPlot** class, a specialized Qwt plot component for visualizing data distribution.

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

/**
 * @class HistogramPlot
 * @brief A specialized QwtPlot widget designed to display data distribution as a histogram.
 *
 * This class handles the necessary setup for a histogram visualization, including the
 * creation of the QwtPlotHistogram item, grid, and reference markers.
 */
class HistogramPlot : public QwtPlot
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the HistogramPlot.
     *
     * Initializes the plot, creates the internal QwtPlotHistogram item, and sets up
     * the basic grid and reference marker.
     *
     * @param parent The parent widget.
     */
    explicit HistogramPlot(QWidget *parent = nullptr);

    /**
     * @brief Calculates the data distribution and updates the histogram with new values.
     *
     * This method takes a set of raw data values, calculates their frequencies,
     * and updates the bars of the histogram plot.
     *
     * @param values The raw data values to be plotted (e.g., error measurements, residual data).
     */
    void setValues(const QVector<double> &values);

    /**
     * @brief Sets the number of bins (buckets) used for calculating the histogram distribution.
     *
     * The histogram will be recalculated and replotted using this new bin count
     * the next time setValues() is called.
     *
     * @param numBins The desired number of bins (must be greater than 0).
     */
    void setNumBins(int numBins);

private:
    /** @name Plot Components */
    ///@{
    QwtPlotHistogram *histogram; ///< @brief The main Qwt item responsible for drawing the histogram bars.
    QwtPlotGrid *grid;           ///< @brief The grid for background reference.
    QwtPlotMarker *mark_zero;    ///< @brief A plot marker used to indicate the zero reference line or mean/median.
    ///@}

    int num_bins = 100; ///< @brief The internal variable storing the current number of bins to be used in distribution calculation.
};

#endif // HISTOGRAMPLOT_H
