/// @file MainWindow.h
/// @brief Defines the **MainWindow** class, the primary graphical user interface (GUI) component
/// for the application, handling all data loading, processing, and visualization controls.

#pragma once

#include <QMainWindow>
#include <QCloseEvent>
#include <QObject> // Required for QObject* in eventFilter
#include <QEvent>  // Required for QEvent* in eventFilter

// --- Forward declarations ---
namespace Ui { class MainWindow; }
class TrackingData; ///< Data structure holding raw tracking information.
class Plot;         ///< Base class for plot widgets.
class ErrorPlot;    ///< Plot widget specialized in visualizing errors/residuals.
class CPF;          ///< Class handling CPF (Consolidated Prediction Format) data.

/**
 * @enum FilterOptions
 * @brief Defines the available data filtering techniques that can be applied to the tracking data.
 */
enum class FilterOptions{
    TreshFilter,        ///< Filtering based on a calculated statistical threshold (e.g., +/- 2.5*STD).
    MovingAverage,      ///< Smoothing filter using a moving average window.
    MedianFilter,       ///< Smoothing filter using a median window.
    ExponentialSmoothing ///< Smoothing filter using Exponential Smoothing.
};

/**
 * @class MainWindow
 * @brief The main window of the application, managing the user interface, data state,
 * data loading, filtering, and interaction between plot components.
 *
 * Inherits from QMainWindow to provide standard window functionality.
 */
class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the main application window.
     *
     * @param parent The parent widget (defaults to nullptr).
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~MainWindow();

protected:
    /**
     * @brief Overrides the standard close event handler.
     *
     * Used to prompt the user to save unsaved changes before the application exits.
     *
     * @param event Pointer to the close event object.
     */
    void closeEvent(QCloseEvent* event) override;

    /**
     * @brief Filters events before they reach the target object.
     * * @brief (Adición Mario): Intercepts events, primarily used here to copy statistics data
     * to the clipboard when the user interacts with the statistics table/area.
     *
     * @param obj The object receiving the event.
     * @param event The event being filtered.
     * @return **true** if the event was handled and should stop propagation; **false** otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // --- Slots for UI actions from .ui file (auto-connected) ---
    /**
     * @brief Slot triggered when the 'Load Data' push button is clicked.
     *
     * Initiates the process of loading new tracking data.
     */
    void on_pb_load_clicked();

    /**
     * @brief Slot triggered when the 'Recalculate' button is clicked.
     *
     * Forces a recalculation of statistics and plot data, often after filtering.
     */
    void on_pb_recalculate_clicked();

    /**
     * @brief Applies a selected filter algorithm to the current tracking data set.
     * * This is the general function called by specific filter buttons/actions.
     *
     * @param f The filtering option (TreshFilter, MovingAverage, etc.) to apply.
     */
    void applyFilter(FilterOptions f);

    /**
     * @brief Slot triggered to specifically apply the RMS-based threshold filter.
     */
    void on_pb_rmsFilter_clicked();

    /**
     * @brief Slot triggered to manually calculate and display the statistical metrics.
     */
    void on_pb_calcStats_clicked();

    /**
     * @brief Slot triggered by the "Load" menu action.
     */
    void on_actionLoad_triggered();

    /**
     * @brief Slot triggered by the "Save" menu action.
     *
     * Saves the currently processed data set, potentially including applied filters.
     */
    void on_actionSave_triggered();

    /**
     * @brief Slot triggered by the "Discard" menu action.
     *
     * Resets the data to its original state or clears unsaved changes.
     */
    void on_actionDiscard_triggered();

    // adición MARIO: cargar fichero CPF
    /**
     * @brief Slot triggered to load a CPF (Consolidated Prediction Format) file.
     *
     * (Adición Mario): Handles the file dialog and loading of the CPF prediction data.
     */
    void on_pb_loadCPF_clicked();

    // --- Custom slots for plot interactions ---
    /**
     * @brief Slot executed when the user changes the selection area on the main plot.
     * * Typically updates displayed statistics or highlights selected points.
     */
    void onPlotSelectionChanged();

    /**
     * @brief Slot executed when the user starts the picking/selection interaction on the main plot.
     */
    void onPlotPickingStarted();

    /**
     * @brief Slot executed when the user finishes the picking/selection interaction on the main plot.
     */
    void onPlotPickingFinished();

    /**
     * @brief Slot executed when the user starts the picking/selection interaction on the histogram plot.
     */
    void onHistogramPickingStarted();

    /**
     * @brief Slot executed when the user finishes the picking/selection interaction on the histogram plot.
     */
    void onHistogramPickingFinished();

    // --- Sync plot navigation ---
    /**
     * @brief Synchronizes the panning action (X-axis view) from the main plot to other plots.
     */
    void syncPanFromMainPlot();

    /**
     * @brief Synchronizes the panning action (X-axis view) from the histogram plot to other plots.
     */
    void syncPanFromHistogramPlot();

    // --- Slot to update status ---
    /**
     * @brief Updates the UI state or status bar when a filter has been successfully applied or changed.
     */
    void onFilterChanged();

    /**
     * @brief Updates the UI state or status bar when the filtered/processed data has been saved.
     */
    void onFilterSaved();

    /**
     * @brief Slot triggered when the UI button to select navigation mode is clicked.
     *
     * Activates panning/zoom interaction on the plots.
     */
    void on_selectNavMode_clicked();

    /**
     * @brief Slot triggered when the UI button to remove navigation mode is clicked.
     *
     * Deactivates panning/zoom interaction on the plots.
     */
    void on_removeNavMode_clicked();

    /**
     * @brief Slot triggered to undo the last data modification or filter action.
     */
    void on_undoButton_clicked();

    /**
     * @brief Slot triggered to redo the last undone data modification or filter action.
     */
    void on_redoButton_clicked();

    /**
     * @brief Slot triggered to deselect all currently selected points on the plots.
     */
    void on_deselectButton_clicked();


private:
    /**
     * @brief Helper function to set up custom signal/slot connections between application components (e.g., plots and data handlers).
     */
    void setupConnections();

    /**
     * @brief Handles the loading and parsing of raw tracking data from a file.
     *
     * @param filePath The path to the tracking data file.
     */
    void loadTrackingData(const QString& filePath);

    /**
     * @brief Updates the visibility and enablement of UI elements based on the current data state (e.g., enable filter buttons only if data is loaded).
     *
     * @param hasData **true** if data is currently loaded; **false** otherwise.
     */
    void updateUIState(bool hasData);

    /**
     * @brief Triggers the repopulation and redrawing of all visual plots.
     */
    void updatePlots();

    /**
     * @brief Clears the displayed statistical results in the dedicated UI area.
     */
    void clearStatistics();

    /**
     * @brief Implements the threshold filtering logic.
     *
     * @return The number of points discarded/filtered out.
     */
    int threshFilter();

    /**
     * @brief Sets up default key sequences for specific keyboard shortcuts for QActions and QPushButtons in UI.
     */
    void setupShortcuts();

    /**
     * @brief Gets current stored actions with keyboard shortcuts and puts them in UI toolbar list.
     */
    void buildShorcutsUI();

    // --- Private Members ---
    Ui::MainWindow *ui;                 ///< @brief Pointer to the Qt Designer generated UI object.
    TrackingData* m_trackingData;       ///< @brief Holds the current set of tracking data (raw and processed).
    QString m_currentFilePath;          ///< @brief Stores the path to the currently loaded tracking data file.
    // adición MARIO: variable para guardar la ruta
    QString m_cpfPath;                  ///< @brief Stores the path to the currently loaded CPF file.
    bool m_isChanged;                   ///< @brief Flag indicating if the data has been modified since the last save operation.
};
