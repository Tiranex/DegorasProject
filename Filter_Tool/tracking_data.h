#pragma once

#include <QString>
#include <QList>
#include <QDate>
#include <QFileInfo>
#include <QSet>

#include <Tracking/trackingfilemanager.h>

class TrackingData
{
public:
    struct Echo
    {
        Echo(long long unsigned int t, long long int ft, long long int diff, long long int diff_cm,
             double az, double el, bool v, int mjd):
            time(t), flight_time(ft), difference(diff), difference_cm(diff_cm),
            azimuth(az), elevation(el), valid(v), mjd(mjd){}
        long long unsigned int time;  // Nanoseconds.
        long long int flight_time;    // Picoseconds.
        long long int difference;
        long long int difference_cm;
        double azimuth;
        double elevation;
        bool valid;
        int mjd;
    };

    explicit TrackingData(QString path_file, bool reset_tracing = true);
    ~TrackingData() {
        qDeleteAll(list_echoes);
        qDeleteAll(list_noise);
        // list_all contains pointers from the other two, so we don't delete them again.
        list_echoes.clear();
        list_noise.clear();
        list_all.clear();
    }


    const QList<Echo*>& listEchoes() const {return this->list_echoes;}
    const QList<Echo*>& listNoise() const {return this->list_noise;}
    const QList<Echo*>& listAll() const {return this->list_all;}
    int meanCal() const {return this->mean_cal;}

    QString satel_name;
    QString file_name;

    bool et_tracking = false;
    bool et_filtered_tracking = false;
    bool dp_tracking = false;
    Tracking data;

private:
    QList<Echo*> list_echoes;
    QList<Echo*> list_noise;
    QList<Echo*> list_all;
    int mean_cal = 0;
};
