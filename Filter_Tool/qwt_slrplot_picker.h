/// @file qwt_slrplot_picker.h
/// @brief Defines the **QwtSLRPlotPicker** class, a specialized Qwt picker for
/// polygon-based selection and interaction on Satellite Laser Ranging (SLR) plots.

#pragma once

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QVector>
#include <QDateTime>

#include <qwt/qwt_global.h>
#include <qwt/qwt_picker.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_picker_machine.h>
#include <qwt/qwt_painter.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_text.h>

#include <QDebug>

class QwtPlot;

/**
 * @class QwtSLRPlotPicker
 * @brief A custom QwtPicker designed for advanced interaction modes (selection/deletion)
 * on SLR data plots.
 *
 * It extends QwtPicker to allow polygon-based selections, custom event handling,
 * and dynamic adjustment of internal parameters like point spacing.
 */
class QwtSLRPlotPicker: public QwtPicker
{
    Q_OBJECT

public:
    // Constructores y destructores.
    /**
     * @brief Constructor for the specialized SLR plot picker.
     *
     * @param canvas The canvas widget on which the picker will operate.
     * @param spacing The initial spacing value for certain picker operations (e.g., marker display).
     */
    explicit QwtSLRPlotPicker(QWidget *canvas = nullptr, int spacing=10);

    /**
     * @brief Default destructor.
     */
    ~QwtSLRPlotPicker() = default;

    // Modificadores.
    /**
     * @brief Sets the internal spacing parameter.
     * @param s The new spacing value.
     */
    inline virtual void setSpacing(int s){this->spacing = s;}

    /**
     * @brief Sets the X and Y axes used for coordinate transformation.
     * @param x The ID of the X-axis (e.g., QwtPlot::xBottom).
     * @param y The ID of the Y-axis (e.g., QwtPlot::yLeft).
     */
    virtual void setAxis(int x, int y);

    // Observadores.
    /**
     * @brief Returns the plot associated with this picker.
     * @return Pointer to the QwtPlot.
     */
    QwtPlot *plot();

    /**
     * @brief Returns the plot associated with this picker (const version).
     * @return Pointer to the const QwtPlot.
     */
    const QwtPlot *plot() const;

    /**
     * @brief Clears the current selection polygon and redraws the canvas.
     */
    void clearSelection();

    /// @brief Returns the canvas widget on which the picker operates.
    inline QWidget *canvas(){return parentWidget();}

    /// @brief Returns the canvas widget on which the picker operates (const version).
    inline const QWidget *canvas() const{return parentWidget();}

    /**
     * @brief Enables or disables the internal selector mechanism.
     * @param flag **true** to enable; **false** to disable.
     */
    inline void enableSelector(bool flag){this->enable_selector=flag;}

signals:
    /**
     * @brief Signal emitted when the spacing parameter has changed.
     * @param spacing The new spacing value.
     */
    void spacingChangued(double spacing);

    /**
     * @brief Signal emitted when a polygon selection operation is completed.
     * @param pa The polygon (in plot coordinates) representing the selection area.
     */
    void selected(const QPolygonF &pa);

protected:
    // Tracker.
    /**
     * @brief Generates the text displayed by the tracker at the given screen position.
     * @param pos The screen position of the mouse cursor.
     * @return The formatted QwtText for the tracker.
     */
    QwtText trackerText(const QPoint& pos) const;

    // Funciones del picker.
    /**
     * @brief Initiates the picking process (e.g., on first mouse click).
     */
    void beginSLRQwtPlotPicker();

    /**
     * @brief Adds a new point to the selection polygon.
     * @param pos The screen position of the new point.
     */
    void appendSLRQwtPlotPicker(const QPoint &pos);

    /**
     * @brief Updates the position of the last point during polygon creation (mouse movement).
     * @param pos The current screen position.
     */
    void moveSLRQwtPlotPicker(const QPoint &pos);

    /**
     * @brief Removes the last appended point from the selection polygon.
     */
    void removeSLRQwtPlotPicker();

    /**
     * @brief Finalizes and completes the picking operation (e.g., on second mouse click or specific key press).
     * @return **true** if the picking sequence was completed; **false** otherwise.
     */
    bool endSLRQwtPlotPicker();

    /**
     * @brief Modifies the internal spacing parameter by a given increment.
     * @param increment The amount to change the spacing by.
     */
    void spacingSLRQwtPlotPicker(int increment);

    // Función de transición.
    /**
     * @brief Processes incoming events and determines the appropriate transition command.
     * @param event The event received (e.g., mouse click, key press).
     */
    void transition(const QEvent *event);

    // Función de dibujado de rubberband.
    /**
     * @brief Draws the visual representation of the selection (the rubber band) on the canvas.
     * @param painter The QPainter object used for drawing.
     */
    void drawRubberBand(QPainter *painter) const;

    // Helpers.
    /**
     * @brief Transforms a screen coordinate point to plot coordinates (X/Y axis values).
     * @param point The screen coordinate point.
     * @return The point transformed to plot coordinates (QPointF).
     */
    QPointF invTransform(const QPoint &) const;

    // Flags.
    bool enable_selector; ///< @brief Flag controlling the enablement of the selector functionality.

private:
    // Custom picker machine for this picker.
    /**
     * @class QwtSLRPickerMachine
     * @brief Custom state machine derived from QwtPickerMachine for handling SLR-specific picker events.
     *
     * Defines custom commands for polygon selection and spacing adjustments.
     */
    class QwtSLRPickerMachine: public  QwtPickerMachine
    {
    public:
        /**
         * @enum QwtSLRPickerMachineCommand
         * @brief Defines commands specific to the SLR plot picker.
         */
        enum QwtSLRPickerMachineCommand
        {
            Begin = 1,          ///< Command to start the selection.
            Append = 2,         ///< Command to add a point to the polygon.
            Remove = 3,         ///< Command to remove the last point.
            Move = 4,           ///< Command to move the last point (during drag).
            End = 5,            ///< Command to finalize the selection.
            SpacingPlus = 101,  ///< Command to increase the spacing parameter.
            SpacingMinus = 102  ///< Command to decrease the spacing parameter.
        };

        QwtSLRPickerMachine():QwtPickerMachine(PolygonSelection){}

        /**
         * @brief Determines the sequence of commands to execute based on the received event.
         * @param eventPattern The event pattern associated with the event.
         * @param e The event received.
         * @return A list of commands to be executed by the picker.
         */
        QList<QwtPickerMachine::Command>transition(const QwtEventPattern &eventPattern, const QEvent *e);
    };

    // Variables privadas.
    int x_axis; ///< @brief ID of the X-axis used for transformations.
    int y_axis; ///< @brief ID of the Y-axis used for transformations.
    int spacing; ///< @brief Internal spacing parameter.
    QPolygon secondary_picked_points; ///< @brief Stores the screen coordinates of the selection polygon points.
    QwtSLRPickerMachine* picker_machine; ///< @brief The custom state machine driving the picker logic.
};
