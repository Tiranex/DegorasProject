/// @file Plot.h
/// @brief Defines the **Plot** class, the core base class for all Qwt plotting widgets
/// in the application, and auxiliary Qwt components (ScaleDraws, Data Containers)
/// used for Satellite Laser Ranging (SLR) data visualization.

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

/**
 * @struct PlotState
 * @brief Represents a snapshot of the plot's data at a specific moment for **Undo/Redo** operations.
 */
struct PlotState {
    QVector<QPointF> candidatePoints; ///< @brief Data currently available for plotting/filtering.
    QVector<QPointF> selectedPoints;  ///< @brief Data actively selected or deemed as "inliers."
};

/**
 * @class QwtPsToMScaleDraw
 * @brief Custom Qwt scale draw class to convert Y-axis units from **Picoseconds (ps) to Meters (m)**.
 *
 * This scale draw applies a conversion factor based on the speed of light
 * ($0.000299792458 \text{ m/ps}$) to display metric units on a secondary axis label.
 */
class QwtPsToMScaleDraw: public QwtScaleDraw
{
public:
    QwtPsToMScaleDraw(){}

    /**
     * @brief Converts the picosecond value to meters for the axis label.
     * @param v The value in picoseconds (ps).
     * @return The QwtText label showing the value in meters (m).
     */
    QwtText label(double v) const;
};

/**
 * @class QwtNanoseconds2TimeScaleDraw
 * @brief Custom Qwt scale draw class to convert X-axis values (**Nanoseconds**) to a readable time format (hh:mm:ss).
 */
class QwtNanoseconds2TimeScaleDraw: public QwtScaleDraw
{
public:
    QwtNanoseconds2TimeScaleDraw(){}

    /**
     * @brief Converts the nanosecond value to a time string.
     * @param v The value in nanoseconds (ns).
     * @return The QwtText label showing the time in "hh:mm:ss" UTC format.
     */
    QwtText label(double v) const;
};


/**
 * @class QwtSLRArraySeriesData
 * @brief Custom data container for Qwt curves specialized for SLR points (QPointF).
 *
 * Provides specialized methods for efficient data manipulation (append, remove, clear)
 * and easy extraction of X and Y data vectors, which is useful for statistical calculations.
 */
class QwtSLRArraySeriesData: public QwtArraySeriesData<QPointF>
{
public:
    QwtSLRArraySeriesData(){}

    QwtSLRArraySeriesData(const QVector<QPointF>& v)
    {
        QwtArraySeriesData::setSamples(v);
    }

    /**
     * @brief Calculates and returns the bounding rectangle of the data set.
     * @return The bounding rectangle.
     */
    inline QRectF boundingRect() const
    {
        if ( this->cachedBoundingRect.width() < 0.0 )
            this->cachedBoundingRect = qwtBoundingRect( *this );

        return this->cachedBoundingRect;
    }
    /// @brief Appends a single point to the data set.
    inline void append(const QPointF &point){m_samples += point;}
    /// @brief Appends a vector of points to the data set.
    inline void append(const QVector<QPointF>& v){m_samples.append(v);}
    /// @brief Removes a point at a specific index.
    inline void remove(int i){m_samples.removeAt(i);}

    /**
     * @brief Removes a set of specific points from the data set.
     * @param v The vector of points to remove.
     */
    inline void remove(const QVector<QPointF>& v)
    {
        foreach(QPointF p, v)
        m_samples.removeOne(p);
    }

    /**
     * @brief Extracts all Y-values (deviations/errors) from the stored points.
     * @return A vector of Y-coordinates (double).
     */
    inline const QVector<double> getYData() const
    {
        QVector<double> y_data;
        foreach(QPointF p, this->samples())
            y_data.append(p.y());
        return y_data;
    }

    /**
     * @brief Extracts all X-values (time) from the stored points.
     * @return A vector of X-coordinates (double).
     */
    inline const QVector<double> getXData() const
    {
        QVector<double> x_data;
        foreach(QPointF p, this->samples())
            x_data.append(p.y());
        return x_data;
    }

    /**
     * @brief Clears all points from the data container and resets the bounding rectangle cache.
     */
    void clear()
    {
        m_samples.clear();
        m_samples.squeeze();
        this->cachedBoundingRect = QRectF( 0.0, 0.0, -1.0, -1.0 );
    }


};

/**
 * @enum PlotMode
 * @brief Defines the active interaction mode for the plot picker.
 */
enum class PlotMode {
    Navigation, ///< @brief Default mode: enables **Zoom** and **Pan** interaction.
    Selection,  ///< @brief "Keep Inside" mode: points inside the selection area are **kept**, others are removed.
    Deletion    ///< @brief "Remove Inside" mode: points inside the selection area are **removed**.
};

/**
 * @class Plot
 * @brief The base QwtPlot class for displaying Satellite Laser Ranging (SLR) data.
 *
 * Handles standard plotting features, navigation (zoom/pan), point selection/filtering,
 * data persistence (Undo/Redo stack), and visualization of various curves (raw data, selected data, fit).
 */
class Plot : public QwtPlot
{
    // --- Internal/Helper Classes ---
    class QScaleDraw : public QwtScaleDraw
    {
    public:
        /**
         * @brief Custom scale draw for controlling axis label notation.
         * @param s_notation If true, uses standard Qwt scale notation (scientific); otherwise, uses fixed notation.
         */
        explicit QScaleDraw(bool s_notation = false)
        {
            this->notation = s_notation;
        }

        /**
         * @brief Overridden label function to apply custom formatting.
         */
        QwtText label(double value) const override
        {
            if (notation)
                return QwtScaleDraw::label(value);
            else
                return QwtText(QString::number(value, 'f', 0));
        }


    private:

        bool notation; ///< @brief Flag to control scientific vs. fixed notation.
    };

    class QwtSLRPlotMagnifier:public QwtPlotMagnifier
    {
    public:
        QwtSLRPlotMagnifier(QWidget* p): QwtPlotMagnifier(p){}

        /**
         * @brief Overridden rescale function to control zoom behavior.
         * @param factor The scaling factor.
         */
        void rescale(double factor);

        ~QwtSLRPlotMagnifier(){}
    };

    Q_OBJECT

public:
    /**
     * @brief Constructor for the base Plot class.
     *
     * Initializes axes, grid, panner, magnifier, and the internal undo/redo stacks.
     *
     * @param parent Parent widget.
     * @param title Plot title.
     */
    Plot( QWidget *parent = nullptr, QString title = "");

    /**
     * @brief Sets the symbol used to draw points on the curves.
     * @param symbol Pointer to the QwtSymbol object.
     */
    void setSymbol( QwtSymbol * );

    /**
     * @brief Sets the initial raw data samples for the plot.
     * @param samples Vector of data points.
     */
    void setSamples( const QVector<QPointF> &samples );

    /**
     * @brief Processes points based on the current PlotMode (Selection/Deletion) and the defined polygon area.
     * @param pol The polygon defining the selection area.
     */
    void selectPoints(const QPolygonF& pol);

    /**
     * @brief Sets the bin size used for data processing (e.g., binning or averaging).
     * @param bin_size The new bin size.
     */
    void setBinSize(int bin_size);

    /**
     * @brief Undoes the last data modification action by reverting to the previous state on the undo stack.
     * @return **true** if an undo operation was performed; **false** otherwise.
     */
    bool undo();

    /**
     * @brief Redoes the last undone action by applying the state from the redo stack.
     * @return **true** if a redo operation was performed; **false** otherwise.
     */
    bool redo();

    /**
     * @brief Saves the current data state (selected and candidate points) onto the undo stack.
     */
    void pushCurrentStateToUndo();

    // Conecta estos slots a tus botones externos
    /**
     * @brief Sets the plot interaction mode to **Navigation** (Zoom/Pan).
     */
    void setNavigationMode();

    /**
     * @brief Sets the plot interaction mode to **Selection** (Keep Inside).
     */
    void setSelectionMode();

    /**
     * @brief Sets the plot interaction mode to **Deletion** (Remove Inside).
     */
    void setDeletionMode();

    // Botón "Deseleccionar / Resetear"
    /**
     * @brief Resets the current selection, effectively restoring all original points
     * or clearing temporary selections without using the undo stack.
     */
    void resetSelection();

    /**
     * @brief Triggers the calculation and display of the polynomial fit for the selected data.
     */
    void updateFit();

    /**
     * @brief Retrieves the currently selected/active data samples for processing.
     * @return Vector of selected points.
     */
    virtual const QVector<QPointF> getSelectedSamples() const
    {
        QwtSLRArraySeriesData *curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());
        return curve_data->samples();
    }

    /**
     * @brief Retrieves the points generated by the last calculated polynomial fit.
     * @return Vector of fit points.
     */
    const QVector<QPointF> getFitSamples() const
    {
        QwtSLRArraySeriesData *adjust_data = static_cast<QwtSLRArraySeriesData *>(this->adjust_curve->data());
        return adjust_data->samples();
    }

    /**
     * @brief Retrieves the residual errors (difference between data and fit) from the last fit calculation.
     * @return Vector of fit error points.
     */
    const QVector<QPointF> getFitErrors() const
    {
        return this->points_fiterrors;
    }

    /**
     * @brief Retrieves the QwtPlotPanner object for external synchronization of panning actions.
     * @return Pointer to the panner object.
     */
    QwtPlotPanner* getPanner() const
    {
        return this->panner;
    }

    // --- Public Curves/Items (exposed for derived classes or UI) ---
    QwtPlotCurve *selected_curve; ///< @brief Curve displaying currently selected/filtered points (inliers).
    QwtPlotCurve *error_curve;    ///< @brief Curve possibly dedicated to displaying errors or outliers (less used in base Plot).

public slots:
    /**
     * @brief Enables or disables the plot picker/selection functionality.
     * @param enabled If true, enables the picker tool.
     */
    void setPickingEnabled(bool enabled);

signals:
    /**
     * @brief Signal emitted after a new polynomial fit has been calculated.
     * @param fit The vector of points representing the new fit curve.
     */
    void fitCalculated(const QVector<QPointF>& fit);

    /**
     * @brief Signal emitted after the set of selected/filtered points has changed.
     */
    void selectionChanged();

    /**
     * @brief Signal emitted when the user initiates a picking/selection operation.
     */
    void startedPicking();

    /**
     * @brief Signal emitted when the user completes a picking/selection operation.
     */
    void finishedPicking();


protected:
    /** @name Internal Plot State and Data Containers */
    ///@{
    bool picking; ///< @brief Flag indicating if picking mode is currently active.
    QVector<QPointF> original_curve; ///< @brief Backup of the initial raw data samples.
    QVector<QPointF> points_fiterrors; ///< @brief Stores the residual values (errors) calculated from the last fit.
    QwtPlotCurve *plot_curve; ///< @brief Primary curve, often holds the current unfiltered/candidate data.
    QwtPlotCurve *adjust_curve; ///< @brief Curve displaying the calculated polynomial fit line.
    ///@}

    /** @name Plot Markers */
    ///@{
    QwtPlotMarker *mark_0;      ///< @brief Marker for the zero reference line.
    QwtPlotMarker *mark_inicio; ///< @brief Marker indicating the start time of the data.
    QwtPlotMarker *mark_final;  ///< @brief Marker indicating the end time of the data.
    ///@}

    /** @name Interaction Tools */
    ///@{
    QwtPlotPanner* panner; ///< @brief Tool for panning (moving the view).
    QwtSLRPlotMagnifier *magnifier; ///< @brief Tool for zooming.
    QwtSLRPlotPicker *picker; ///< @brief Custom tool for selection/picking operations.
    ///@}

    int bin_size; ///< @brief Current bin size parameter used for certain data operations.

    /** @name Undo/Redo Stacks */
    ///@{
    QStack<PlotState> m_undoStack; ///< @brief Stack to store previous data states for undo functionality.
    QStack<PlotState> m_redoStack; ///< @brief Stack to store undone states for redo functionality.
    ///@}

    PlotMode currentMode; ///< @brief The currently active interaction mode (Navigation, Selection, Deletion).

    /**
     * @brief Helper function to apply a saved PlotState (from the undo/redo stacks) to the current plot data.
     * @param state The state to apply.
     */
    void applyState(const PlotState& state);
};
