#include "Tracking/calibrationfilemanager.h"
#include "Tracking/tracking.h"
#include "degoras_settings.h"
#include "window_message_box.h"
#include "LibDegorasBase/Helpers/string_helpers.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QString>


const QString kDateStartKey = QStringLiteral("date");
const QString kCfgIdKey = QStringLiteral("cfg_id");
const QString kStationNameKey = QStringLiteral("station_name");
const QString kStationIdKey = QStringLiteral("station_id");
const QString kCalTypeKey = QStringLiteral("cal_type");
const QString kTgtDistKey = QStringLiteral("tgt_dist_2w");
const QString kTgtToFKey = QStringLiteral("tgt_tof_2w");
const QString kRFKey = QStringLiteral("rf");
const QString kNShotsKey = QStringLiteral("nshots");
const QString kRNShotsKey = QStringLiteral("rnshots");
const QString kUNShotsKey = QStringLiteral("unshots");
const QString kTRORRFRMSKey = QStringLiteral("tror_rfrms");
const QString kTROR1RMSKey = QStringLiteral("tror_1rms");
const QString kCalValRFRMSKey = QStringLiteral("cal_val_rfrms");
const QString kCalVal1RMSKey = QStringLiteral("cal_val_1rms");
const QString kMeteoKey = QStringLiteral("meteo");
const QString kStatsRFRMSKey = QStringLiteral("stats_rfrms");
const QString kStats1RMSKey = QStringLiteral("stats_1rms");
const QString kRangesKey = QStringLiteral("ranges_data");
const QString kFlagKey = QStringLiteral("flag");
const QString kStartKey = QStringLiteral("start");
const QString kToFKey = QStringLiteral("tof_2w");
const QString kEtKey = QStringLiteral("et_data");
const QString kTAKey = QStringLiteral("tA");
const QString kTBKey = QStringLiteral("tB");
const QString kETPrecisionKey = QStringLiteral("precision");

const QMap<CalibrationFileManager::ErrorEnum, QString> CalibrationFileManager::ErrorListStringMap =
{
    {CalibrationFileManager::ErrorEnum::CALIBFILE_INVALID,
     "The calibration json file %1 is not valid."},
    {CalibrationFileManager::ErrorEnum::CALIBFILE_NOT_OPEN,
     "The calibration json file %1 could not be opened."},
};

// TODO: long double?
// TODO: validation of values?
DegorasInformation CalibrationFileManager::readCalibration(const QString &cal_name, const QString &dir_path, Calibration &calib)
{
    return dir_path.isEmpty() ? CalibrationFileManager::readCalibration(cal_name, calib) :
                                CalibrationFileManager::readCalibrationFromFile(dir_path + '/' + cal_name, calib);
}

DegorasInformation CalibrationFileManager::readCalibration(const QString &cal_name, Calibration &calib)
{
    QString file_path = CalibrationFileManager::findCalibration(cal_name);
    DegorasInformation result;
    if (file_path.isEmpty())
        result = {{CalibrationFileManager::ErrorEnum::CALIBFILE_NOT_OPEN,
                   ErrorListStringMap[ErrorEnum::CALIBFILE_NOT_OPEN].arg(cal_name)}};
    else
        result = CalibrationFileManager::readCalibrationFromFile(file_path, calib);

    return result;
}

DegorasInformation CalibrationFileManager::writeCalibration(const Calibration& calib, const QString& dest_dir,
                                                           const QString& dest_file)
{
    // If dest_file name is empty, then use default
    QString file_path = dest_dir + '/';
    if (dest_file.isEmpty())
        file_path += CalibrationFileManager::calibrationFilename(calib);
    else
        file_path += dest_file;

    QFile calib_file(file_path);
    // Check if file could be opened to write.
    if(!calib_file.open(QIODevice::WriteOnly | QIODevice::Text))
        return DegorasInformation({CalibrationFileManager::ErrorEnum::CALIBFILE_NOT_OPEN,
                                  ErrorListStringMap[ErrorEnum::CALIBFILE_NOT_OPEN].arg(file_path)});

    QJsonObject calib_object;

    calib_object.insert(kDateStartKey, calib.date_start.toString(Qt::ISODateWithMs));
    calib_object.insert(kCfgIdKey, calib.cfg_id);
    calib_object.insert(kStationNameKey, calib.station_name);
    calib_object.insert(kStationIdKey, static_cast<int>(calib.station_id));
    calib_object.insert(kCalTypeKey, static_cast<int>(calib.type));
    calib_object.insert(kTgtDistKey, static_cast<double>(calib.target_dist_2w));
    calib_object.insert(kTgtToFKey, static_cast<double>(calib.target_tof_2w));
    calib_object.insert(kRFKey, calib.rf);
    calib_object.insert(kNShotsKey, static_cast<int>(calib.nshots));
    calib_object.insert(kRNShotsKey, static_cast<int>(calib.rnshots));
    calib_object.insert(kUNShotsKey, static_cast<int>(calib.unshots));
    calib_object.insert(kTRORRFRMSKey, calib.tror_rfrms);
    calib_object.insert(kTROR1RMSKey, calib.tror_1rms);
    calib_object.insert(kCalValRFRMSKey, calib.cal_val_rfrms);
    calib_object.insert(kCalVal1RMSKey, calib.cal_val_1rms);

    calib_object.insert(kMeteoKey, calib.meteo.toJson());

    calib_object.insert(kStatsRFRMSKey, StatstoJson(calib.stats_rfrms));
    calib_object.insert(kStats1RMSKey, StatstoJson(calib.stats_1rms));

    QJsonArray array;
    for (const auto& elem : calib.ranges)
    {
        QJsonObject obj;
        // TODO: check values of enum
        obj.insert(kFlagKey, static_cast<int>(elem.flag));
        obj.insert(kStartKey, static_cast<double>(elem.start_time));
        obj.insert(kToFKey, elem.flag == Calibration::RangeData::FilterFlag::UNKNOWN ?
                       QJsonValue() : static_cast<double>(elem.tof_2w));
        array.push_back(obj);
    }
    calib_object.insert(kRangesKey, array.empty() ? QJsonValue() : array);

    QJsonObject et_object;
    array = {};
    std::transform(calib.tA.begin(), calib.tA.end(), std::back_inserter(array),
                   [](const auto& a){return QString::fromStdString(dpbase::helpers::strings::numberToStr(a, 18, 12));});
    if (!array.empty())
        et_object.insert(kTAKey, array);

    array = {};
    std::transform(calib.tB.begin(), calib.tB.end(), std::back_inserter(array),
                   [](const auto& a){return QString::fromStdString(dpbase::helpers::strings::numberToStr(a, 18, 12));});
    if (!array.empty())
        et_object.insert(kTBKey, array);

    if (!et_object.empty())
        et_object.insert(kETPrecisionKey, static_cast<int>(calib.et_precision));

    // Only insert ET precission if there is tA or tB
    calib_object.insert(kEtKey, et_object.empty() ? QJsonValue() : et_object);

    QJsonDocument calib_jsondocument(calib_object);
    calib_file.write(calib_jsondocument.toJson(QJsonDocument::Indented));
    calib_file.close();

    // Return the errors
    return {};
}

DegorasInformation CalibrationFileManager::readCalibrationDir(const QString &dir, std::vector<Calibration> &calibs)
{
    Calibration c;
    DegorasInformation result;
    for (auto&& file : QDir(dir).entryList({"*.dpcr"}, QDir::Files))
    {
        auto errors = CalibrationFileManager::readCalibration(file, dir, c);
        result.append(errors);
        if (!errors.hasError())
            calibs.push_back(c);
    }
    return result;
}

DegorasInformation CalibrationFileManager::readLastCalib(Calibration &calib)
{
    QString hist_calpath =
            DegorasSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_HistoricalCalibrations");
    QStringList dirs = QDir(hist_calpath).entryList(QDir::Dirs, QDir::Name);

    QDateTime last_calib_dt(QDateTime::fromMSecsSinceEpoch(0, Qt::UTC));
    QString last_calib_name;
    DegorasInformation errors;
    auto dir_it = dirs.crbegin();

    if (!dirs.empty())
    {
        do
        {
            for (const auto& filename : QDir(hist_calpath + '/' + *dir_it).entryList(QDir::Files))
            {
                QDateTime calib_dt(CalibrationFileManager::startDateTime(filename));
                if (calib_dt > last_calib_dt)
                {
                    last_calib_name = filename;
                    last_calib_dt = calib_dt;
                }
            }

            dir_it++;

        } while (last_calib_name.isEmpty() && dir_it != dirs.crend());
    }

    if (last_calib_name.isEmpty())
    {
        errors = {{ErrorEnum::CALIB_NOT_FOUND, "Last calibration file not found"}};
    }
    else
    {
        dir_it--;
        errors = CalibrationFileManager::readCalibrationFromFile(
                    hist_calpath + '/' + *dir_it + '/' + last_calib_name, calib);
    }

    return errors;
}


QString CalibrationFileManager::calibrationFilename(const Calibration &calib)
{
    return QString::number(calib.station_id) + '_' + calib.cfg_id + "_" +
            calib.date_start.toString("yyyyMMddhhmm") + ".dpcr";
}

QString CalibrationFileManager::findCalibration(const QString &calib_name)
{   
    QString result;
    QDate date_start = CalibrationFileManager::startDate(calib_name);

    if (date_start.isValid())
    {
        QString hist_calpath =
                DegorasSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_HistoricalCalibrations");

        result = hist_calpath + "/" + date_start.toString("yyyyMMdd") + '/' + calib_name;

        if (!QFile::exists(result))
            result = QString();
    }

    return result;
}

QDate CalibrationFileManager::startDate(const QString &calib_name)
{
    QString name(calib_name);
    name.remove(".dpcr");
    QStringList splitted = name.split('_');
    QDate date;
    if (splitted.size() > 2)
    {
        QString date_string;
        if (12 == splitted[2].size())
            date_string = splitted[2].left(8);
        else if (10 == splitted[2].size())
            date_string = "20" + splitted[2].left(6);
        date = QDate::fromString(date_string, "yyyyMMdd");
    }
    return date;
}

QDateTime CalibrationFileManager::startDateTime(const QString &calib_name)
{
    QString name(calib_name);
    name.remove(".dpcr");
    QStringList splitted = name.split('_');
    QDateTime datetime;
    if (splitted.size() > 2)
    {
        QString datetime_string;
        if (12 == splitted[2].size())
            datetime_string = splitted[2].left(12);
        else if (10 == splitted[2].size())
            datetime_string = "20" + splitted[2].left(10);
        datetime = QDateTime::fromString(datetime_string, "yyyyMMddhhmm");
    }
    return datetime;
}

DegorasInformation CalibrationFileManager::readCalibrationFromFile(const QString &filepath, Calibration &calib)
{
    QFile calib_file(filepath);
    // Check if file could be opened.
    if(!calib_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return DegorasInformation({CalibrationFileManager::ErrorEnum::CALIBFILE_NOT_OPEN,
                                  ErrorListStringMap[ErrorEnum::CALIBFILE_NOT_OPEN].arg(filepath)});

    // Read all.
    QString calib_string = calib_file.readAll().simplified();
    calib_file.close();
    // Loads the json document.
    QJsonDocument json = QJsonDocument::fromJson(calib_string.toUtf8());

    // Check if scheme is valid
    DegorasInformation::ErrorList error_list;

    // Check if data file is valid
    if (json.isNull())
    {
        error_list.append({ErrorEnum::CALIBFILE_INVALID, ErrorListStringMap[ErrorEnum::CALIBFILE_INVALID].arg(filepath)});
    }
    else
    {
        // Ensure object is cleared
        calib = Calibration();

        calib.date_start = QDateTime::fromString(json[kDateStartKey].toString(), Qt::ISODateWithMs);
        calib.cfg_id = json[kCfgIdKey].toString();
        calib.station_name = json[kStationNameKey].toString();
        calib.station_id = json[kStationIdKey].toInt();
        calib.type = static_cast<Calibration::Type>(json[kCalTypeKey].toInt());
        calib.target_dist_2w = json[kTgtDistKey].toDouble();
        calib.target_tof_2w = {json[kTgtToFKey].toDouble(), decltype(calib.target_tof_2w)::Unit::LIGHT_PS};
        calib.rf = json[kRFKey].toDouble();
        calib.nshots = json[kNShotsKey].toInt();
        calib.rnshots = json[kRNShotsKey].toInt();
        calib.unshots = json[kUNShotsKey].toInt();
        calib.tror_rfrms = json[kTRORRFRMSKey].toDouble();
        calib.tror_1rms = json[kTROR1RMSKey].toDouble();
        calib.cal_val_rfrms = json[kCalValRFRMSKey].toDouble();
        calib.cal_val_1rms = json[kCalVal1RMSKey].toDouble();

        calib.meteo = MeteoData::fromJson(json[kMeteoKey].toObject());

        calib.stats_rfrms = StatsFromJson(json[kStatsRFRMSKey].toObject());
        calib.stats_1rms = StatsFromJson(json[kStats1RMSKey].toObject());

        QJsonArray array = json[kRangesKey].toArray();
        for (const auto& elem : std::as_const(array))
        {
            QJsonObject obj = elem.toObject();
            Calibration::RangeData echo;
            // TODO: check values of enum
            echo.flag = static_cast<Calibration::RangeData::FilterFlag>(obj[kFlagKey].toInt());
            echo.start_time = obj[kStartKey].toDouble();
            echo.tof_2w = obj[kToFKey].toDouble();
            calib.ranges.push_back(echo);
        }

        // TODO: include fail for failed converssions?
        QJsonObject et_object = json[kEtKey].toObject();
        if (!et_object.empty())
        {
            array = et_object[kTAKey].toArray();
            for (const auto& elem : std::as_const(array))
            {
                try
                {
                    calib.tA.push_back(std::stold(elem.toString().toStdString()));
                }
                catch(...){}
            }

            array = et_object[kTBKey].toArray();
            for (const auto& elem : std::as_const(array))
            {
                try
                {
                    calib.tB.push_back(std::stold(elem.toString().toStdString()));
                }
                catch(...){}
            }

            calib.et_precision = et_object[kETPrecisionKey].toInt();
        }
    }

    // Return the errors
    return DegorasInformation(error_list);
}

