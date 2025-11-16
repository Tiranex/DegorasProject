#include <QFileDialog>
#include <QPointF>
#include <QTextStream>

#include "class_mainwindow.h"
#include <DP_Core/include/Tracking/tracking.h>
#include "ui_form_mainwindow.h"
#include <DP_Core/include/Tracking/calibrationfilemanager.h>
#include <DP_Core/include/Tracking/trackingfilemanager.h>
#include <DP_Core/include/degoras_settings.h>
#include <DP_Core/include/algorithms.h>
#include <LibDegorasBase/Helpers/container_helpers.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(this->ui->actionOpenFIle, &QAction::triggered, this, &MainWindow::openFile);
    QObject::connect(this->ui->pb_w_apply, &QPushButton::clicked, this, &MainWindow::applyWindow);
    QObject::connect(this->ui->pb_pre_apply, &QPushButton::clicked, this, &MainWindow::applyPrefilter);
    QObject::connect(this->ui->pb_post_apply, &QPushButton::clicked, this, &MainWindow::applyPostfilter);

    QObject::connect(this->ui->pb_rnoise, &QPushButton::clicked, this, &MainWindow::removeNoise);
    QObject::connect(this->ui->pb_detrend, &QPushButton::pressed, this, &MainWindow::detrend);
    this->ui->xb_adjusjt_visible->setChecked(true);
    QObject::connect(this->ui->xb_adjusjt_visible, &QCheckBox::toggled,
                     this->ui->plot_staticres, &StaticResidualsPlot::setAdjustCurveVisible);

}


void MainWindow::openFile()
{
    // Auxiliar container.
    QVector<QPointF> all_resids;

    // Clear the container.
    this->times.clear();
    this->resids.clear();
    this->ui->plot_staticres->clearAll();

    QString file_path = QFileDialog::getOpenFileName(this, "Select file for simulation");

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QFileInfo fileinfo(file);
    QTextStream in(&file);

    // Check the file type and open the file.
    if(fileinfo.suffix() == "ks")
    {
        // Skip the first line.
        in.readLine();

        // Read all the lines.
        while (!in.atEnd())
        {
            QStringList line = in.readLine().trimmed().simplified().split(' ');
            try
            {
                // Get the range data.
                double x = std::stod(line[0].toStdString());
                double y = std::stod(line[2].toStdString()) * 1000.0;
                this->times.push_back(x);
                this->resids.push_back(y);
            }
            catch(...){}
        }
    }
    else if (fileinfo.suffix() == "dptr")
    {
        Tracking tr;
        QStringList tokens = fileinfo.fileName().split("_");
        if (tokens.size() > 3)
        {
            //QString calib_path = DegorasSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_HistoricalCalibrations");
            //calib_path += "/" + tokens[2].left(8) ;

            auto errors = TrackingFileManager::readTrackingFromFile(fileinfo.fileName(), {}, tr);
            if (errors.hasError())
            {
                errors.showErrors("AlgorithmsTester", DegorasInformation::WARNING, "Errors at tracking reading", this);
            }
            else
            {
                for (const auto& range : tr.ranges)
                {
                    this->times.push_back(range.start_time);
                    this->resids.push_back(range.tof_2w - range.pre_2w - range.trop_corr_2w);
                }
            }
        }
    }

    // Populate the data.
    this->ui->plot_staticres->fastPopulate(this->times, this->resids, StaticResidualsPlot::DataTypeEnum::GENERIC);
}

void MainWindow::applyWindow()
{
    // Block the signals.
    this->blockSignals(true);

    // Clear the data.
    this->ui->plot_staticres->clearAll();

    // Auxiliar containers.
    std::vector<std::size_t> win_idxs;

    // Get the configuration data.
    double w_upper = this->ui->sb_w_upper->value()*1000;
    double w_lower = this->ui->sb_w_lower->value()*1000;

    // Apply the window to the residuals.
    win_idxs = algorithm::windowPrefilterPrivate(this->resids, w_upper, w_lower);

    // Get the data in the configured window.
    this->times_win = dpbase::helpers::containers::extract(this->times, win_idxs);
    this->resids_win = dpbase::helpers::containers::extract(this->resids, win_idxs);

    // Populate the graph.
    this->ui->plot_staticres->fastPopulate(this->times_win, this->resids_win, StaticResidualsPlot::DataTypeEnum::GENERIC);

    // Unblock the signals.
    this->blockSignals(false);
}

void MainWindow::applyPrefilter()
{
    // Auxiliar containers.
    std::vector<std::size_t> data_idxs;

    // Get the configuration data.
    double bs = this->ui->sb_hs_bins->value();
    double depth = this->ui->sb_hs_depth->value()*3335.640951982;
    double mph = this->ui->sb_hs_mph->value();
    double div = this->ui->sb_hs_div->value();

    // Apply the prefilter.
    data_idxs = algorithm::histPrefilterSLR(this->times_win, this->resids_win, bs, depth, mph, div);

    // Extract the data.
    this->times_pre = dpbase::helpers::containers::extract(this->times_win, data_idxs);
    // this->resids_pre = dpbase::helpers::containers::extract(this->resids_win, data_idxs);

    // Populate the graph.
    this->ui->plot_staticres->fastPopulate(this->times_win, this->resids_win, StaticResidualsPlot::DataTypeEnum::GENERIC);
    this->ui->plot_staticres->fastPopulate(this->times_pre, this->resids_pre, StaticResidualsPlot::DataTypeEnum::DATA);
}

void MainWindow::applyPostfilter()
{
    std::vector<std::size_t> data_idxs;

    // Get the configuration data.
    double bs = this->ui->sb_post_bins->value();
    double depth = this->ui->sb_post_depth->value()*3335.640951982;

    // Apply the postfilter.
    data_idxs = algorithm::histPostfilterSLR(this->times_pre, this->resids_pre, bs, depth);

    // Extract the data.
    this->times_post = dpbase::helpers::containers::extract(this->times_pre, data_idxs);
    this->resids_post = dpbase::helpers::containers::extract(this->resids_pre, data_idxs);

    // Populate the graph.
    this->ui->plot_staticres->fastPopulate(this->times_pre, this->resids_pre, StaticResidualsPlot::DataTypeEnum::GENERIC);
    this->ui->plot_staticres->fastPopulate(this->times_post, this->resids_post, StaticResidualsPlot::DataTypeEnum::DATA);

//    QVector<QPointF> all_resids;
//    QVector<QPointF> data_resids;
//    std::vector<long double> dtimes;
//    std::vector<long double> dresids;
//    std::vector<long double> wtimes;
//    std::vector<long double> wresids;

//    // Get the configuration data.
//    double hs_bs = this->ui->sb_post_bins->value();
//    double hs_depth = this->ui->sb_post_depth->value();

//    // Get the data in the configured window.
//    for(const std::size_t& index : this->win_idxs)
//    {
//        wtimes.push_back(this->times[index]);
//        wresids.push_back(this->resids[index]);
//    }

//    // Get the current considered data.
//    for(const std::size_t& index : this->data_idxs)
//    {
//        dtimes.push_back(wtimes[index]);
//        dresids.push_back(wresids[index]);
//    }

//    this->post_data_idxs = dpslr::algorithms::histPostfilterSLR(dtimes, dresids, hs_bs, hs_depth*3335.640951982);


//    //this->post_data_idxs = dpslr::algorithms::histPostfilterSLR(wtimes, wresids, hs_bs, hs_depth*3335.640951982);

////    for(std::size_t i = 0; i<dtimes.size();i++)
////    {
////        QPointF point;
////        point.setX(dtimes[i]);
////        point.setY(dresids[i]);
////        all_resids.append(point);
////    }

//    for(std::size_t i = 0; i<wtimes.size();i++)
//    {
//        QPointF point;

//        point.setX(wtimes[i]);
//        point.setY(wresids[i]);

//        all_resids.append(point);
//    }

//    for(const std::size_t& index: this->post_data_idxs)
//    {
//        QPointF point;
//        point.setX(dtimes[index]);
//        point.setY(dresids[index]);
//        //point.setX(wtimes[index]);
//        //point.setY(wresids[index]);
//        data_resids.append(point);
//    }

//    this->ui->plot_staticres->clearAll();
//    this->ui->plot_staticres->populateAll(all_resids);
//    this->ui->plot_staticres->populateData(data_resids);
}

void MainWindow::removeNoise()
{
    this->ui->plot_staticres->clearGeneric();
//    QVector<QPointF> data_resids;
//    std::vector<long double> wtimes;
//    std::vector<long double> wresids;
//    std::vector<long double> dtimes;
//    std::vector<long double> dresids;

//    // Get the data in the configured window.
//    for(const std::size_t& index : this->win_idxs)
//    {
//        wtimes.push_back(this->times[index]);
//        wresids.push_back(this->resids[index]);
//    }


//    if (this->post_data_idxs.empty())
//    {
//        for(const std::size_t& index: this->data_idxs)
//        {
//            QPointF point;

//            point.setX(wtimes[index]);
//            point.setY(wresids[index]);

//            data_resids.append(point);
//        }
//    }
//    else
//    {
//        // Get the current considered data.
//        for(const std::size_t& index : this->data_idxs)
//        {
//            dtimes.push_back(wtimes[index]);
//            dresids.push_back(wresids[index]);
//        }

//        for(const std::size_t& index: this->post_data_idxs)
//        {
//            QPointF point;

//            point.setX(dtimes[index]);
//            point.setY(dresids[index]);

//            data_resids.append(point);
//        }
//    }



//    this->ui->plot_staticres->clearAll();

//    this->ui->plot_staticres->populateAll(data_resids);
}

void MainWindow::detrend()
{
    this->detrend_resids = dpbase::stats::detrend(this->times_win, this->resids_win, this->times_pre, this->resids_pre, 9);
    this->detrend_resids_data = dpbase::stats::detrend(this->times_pre, this->resids_pre, 9);

    // Populate the graph.
    this->ui->plot_staticres->clearAll();
    this->ui->plot_staticres->fastPopulate(this->times_win, this->detrend_resids, StaticResidualsPlot::DataTypeEnum::GENERIC);
    this->ui->plot_staticres->fastPopulate(this->times_pre, this->detrend_resids_data, StaticResidualsPlot::DataTypeEnum::DATA);

//    QVector<QPointF> all_resids;
//    QVector<QPointF> data_resids;
//    std::vector<long double> wtimes;
//    std::vector<long double> wresids;
//    std::vector<long double> ftimes;
//    std::vector<long double> fresids;

//    for(std::size_t i = 0; i < this->detrend_resids.size(); i++)
//    {
//        QPointF point;
//        point.setX(ftimes[i]);
//        point.setY(fresids[i]);
//        all_resids.append(point);
//    }


//    // Get the configuration data.
//    double hs_bs = this->ui->sb_post_bins->value();
//    double hs_depth = this->ui->sb_post_depth->value();

//    this->post_data_idxs = dpslr::algorithms::histPostfilterSLR(ftimes, this->detrend_resids, hs_bs, hs_depth*3335.640951982);

//    dpslr::common::ResidualsTimeData rdata;

//    for(const std::size_t& index: this->post_data_idxs)
//    {
//        QPointF point;
//        point.setX(ftimes[index]);
//        point.setY(fresids[index]);
//        data_resids.append(point);

//        rdata.push_back({ftimes[index], fresids[index]});
//    }

//    dpslr::algorithms::ResidualsStats stats;

//    dpslr::algorithms::calculateResidualsStats(30, rdata, stats);



//    qDebug()<< QString::fromStdString(std::to_string(stats.total_bin_stats.stats_rfrms.rms));
//    qDebug()<< QString::fromStdString(std::to_string(stats.total_bin_stats.stats_rfrms.aptn));
//    qDebug()<< QString::fromStdString(std::to_string(stats.total_bin_stats.stats_rfrms.rptn));
//    qDebug()<< QString::fromStdString(std::to_string(stats.total_bin_stats.stats_rfrms.arate));

//    QVector<QPointF> final_data_resids;

//    for(std::size_t i = 0; i < stats.total_bin_stats.amask_rfrms.size(); i++)
//    {
//        if(stats.total_bin_stats.amask_rfrms[i])
//        {
//            QPointF point;
//            point.setX(rdata[i].first);
//            point.setY(rdata[i].second);
//            final_data_resids.append(point);
//        }
//    }

//    this->ui->plot_staticres->clearAll();

//    this->ui->plot_staticres->fastPopulate(all_resids);


//    this->ui->plot_staticres->populateData(final_data_resids);

}

void MainWindow::applyStatisticalFilter()
{

}
