#include "tracking_data.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <QFileDialog>
#include <QDate>
#include <set>
#include <algorithm>

#include <window_message_box.h>

TrackingData::TrackingData(QString path_file, bool reset_tracing)
{
    // Lectura del fichero de seguimiento.
    QFile file(path_file);
    this->file_name = file.fileName();
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);

        if (this->file_name.contains("dptr"))
        {
            qInfo() << "New DP Tracking file";
            this->dp_tracking = true;
            QFileInfo info(file);
            DegorasInformation errors = TrackingFileManager::readTrackingFromFile(info.absoluteFilePath(), {}, this->data);
            if (errors.hasError())
            {
                errors.showErrors("Filter Tool", DegorasInformation::WARNING, "");
                return;
            }

            this->mean_cal = this->data.cal_val_overall;
            qint64 mjd = this->data.date_start.date().toJulianDay();// + dpslr::utils::kJulianToModifiedJulian;
            long double prev_start = -1.L;
            long double offset = 0.L;

            for (const auto& shot : this->data.ranges)
            {
                if (shot.start_time < prev_start)
                {
                    offset += 86400.L;
                }
                prev_start = shot.start_time;

                double resid = shot.tof_2w - shot.pre_2w - shot.trop_corr_2w - static_cast<long long>(this->data.cal_val_overall);
                if (reset_tracing || shot.flag == Tracking::RangeData::FilterFlag::DATA )
                    this->list_echoes.append(new Echo(static_cast<unsigned long long>((shot.start_time + offset) * 1e9), static_cast<long long>(shot.tof_2w), static_cast<long long>(resid), static_cast<long long>(resid*0.0299792458), {}, {}, true, mjd));
                else if (shot.flag == Tracking::RangeData::FilterFlag::NOISE)
                    this->list_noise.append(new Echo( static_cast<unsigned long long>((shot.start_time + offset) * 1e9), static_cast<long long>(shot.tof_2w), static_cast<long long>(resid), static_cast<long long>(resid*0.0299792458), {}, {}, true, mjd));
            }
            this->satel_name = this->data.obj_name;
        }
        else
        {
            // ECOS BRUTOS
            int yy = 2000 + this->file_name.split('/').last().mid(0, 2).toInt();
            int MM = this->file_name.split('/').last().mid(2, 2).toInt();
            int dd = this->file_name.split('/').last().mid(4, 2).toInt();
            int mjd = QDate(yy, MM, dd).toJulianDay(); //+ dpslr::utils::kJulianToModifiedJulian;

            for(int i=0; i<=10; i++)
            {
                QString aux = in.readLine();
                if(i==1) this->satel_name = aux.simplified().split(',')[0];
                else if(i==9) this->mean_cal = aux.simplified().split(',')[0].toInt();
            }

            while (!in.atEnd())
            {
                QStringList splitter = in.readLine().simplified().replace(" ", "").split(',');
                if(splitter.size()==10)
                {
                    QString time_string = splitter[0];
                    long long int flight_time = splitter[1].toDouble();
                    long long int difference = splitter[2].toDouble();
                    double azimuth = splitter[8].toDouble();
                    double elevation = splitter[9].toDouble();
                    double difference_cm = difference*0.0299792458;
                    time_string.append("00");

                    if(reset_tracing == true && time_string[0]=='-')
                    {
                        time_string.remove(0,1);
                        long long unsigned int time = time_string.toULongLong();
                        this->list_echoes.append(new Echo(time, flight_time, difference, difference_cm, azimuth, elevation, true, mjd));
                    }
                    else if(reset_tracing == false && time_string[0]=='-')
                    {
                        time_string.remove(0,1);
                        long long unsigned int time = time_string.toULongLong();
                        this->list_noise.append(new Echo(time, flight_time, difference, difference_cm, azimuth, elevation, true, mjd));
                    }
                    else
                    {
                        long long unsigned int time = time_string.toULongLong();
                        this->list_echoes.append(new Echo(time, flight_time, difference, difference_cm, azimuth, elevation, true, mjd));
                    }
                }
            }
        }
        file.close();
        this->list_all = this->listEchoes() + this->listNoise();
        std::sort(this->list_all.begin(), this->list_all.end(),
                  [](const auto& a, const auto& b){return a->time < b->time;});
    }
}
