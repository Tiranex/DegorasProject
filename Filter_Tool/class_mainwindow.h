#pragma once

#include <QMainWindow>
#include <QCloseEvent>

// Forward declarations
namespace Ui { class MainWindow; } // MainWindow from UI, holds the pointers to every element in window and no extra logic
class TrackingData;
class Plot;
class ErrorPlot;
class CPF;

enum class FilterOptions{
    TreshFilter,
    MovingAverage,
    MedianFilter,
    ExponentialSmoothing
};

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Slots for UI actions from .ui file (auto-connected)
    void on_pb_load_clicked();
    void on_pb_recalculate_clicked();
    void applyFilter(FilterOptions f);
    void on_pb_rmsFilter_clicked();
    void on_pb_calcStats_clicked();
    void on_actionLoad_triggered();
    void on_actionSave_triggered();
    void on_actionDiscard_triggered();
    
    // Custom slots for plot interactions
    void onPlotSelectionChanged();
    void onPlotPickingStarted();
    void onPlotPickingFinished();
    void onHistogramPickingStarted();
    void onHistogramPickingFinished();
    
    // Sync plot navigation
    void syncPanFromMainPlot();
    void syncPanFromHistogramPlot();
    
    // Slot to update status
    void onFilterChanged();
    void onFilterSaved();

    void on_selectNavMode_clicked();

    void on_removeNavMode_clicked();

    void on_undoButton_clicked();

    void on_redoButton_clicked();

    void on_deselectButton_clicked();

private:
    void setupConnections();
    void loadTrackingData(const QString& filePath);
    void updateUIState(bool hasData);
    void updatePlots();
    void clearStatistics();
    
    int threshFilter();

    Ui::MainWindow *ui;
    TrackingData* m_trackingData; // pointer to filepath where point data is
    QString m_currentFilePath;
    bool m_isChanged;
};
