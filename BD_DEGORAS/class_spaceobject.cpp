#include "class_spaceobject.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>

SpaceObjectSet::SpaceObjectSet(const QString &name, const QStringList &enabled)
    : name(name), list_enabled(enabled)
{
}

// Initialization of static maps for Enums
const QMap<SpaceObject::EnablementPolicy, QString> SpaceObject::EnablementPolicyStringMap = {
    {SpaceObject::EnablementPolicy::NO_ENABLEMENT_POLICY, "No Policy"},
    {SpaceObject::EnablementPolicy::DISABLED, "Disabled"},
    {SpaceObject::EnablementPolicy::ENABLED, "Enabled"},
    {SpaceObject::EnablementPolicy::ALWAYS_DISABLED, "Always Disabled"},
    {SpaceObject::EnablementPolicy::ALWAYS_ENABLED, "Always Enabled"}
};

const QMap<QString, SpaceObject::EnablementPolicy> SpaceObject::EnablementPolicyEnumMap = {
    {"No Policy", SpaceObject::EnablementPolicy::NO_ENABLEMENT_POLICY},
    {"Disabled", SpaceObject::EnablementPolicy::DISABLED},
    {"Enabled", SpaceObject::EnablementPolicy::ENABLED},
    {"Always Disabled", SpaceObject::EnablementPolicy::ALWAYS_DISABLED},
    {"Always Enabled", SpaceObject::EnablementPolicy::ALWAYS_ENABLED}
};

const QMap<SpaceObject::TrackPolicy, QString> SpaceObject::TrackPolicyStringMap = {
    {SpaceObject::TrackPolicy::NO_TRACK_POLICY, "No Policy"},
    {SpaceObject::TrackPolicy::TRACK_ALWAYS, "Always"},
    {SpaceObject::TrackPolicy::TRACK_ONLY_IF_VISIBLE, "Only If Visible"},
    {SpaceObject::TrackPolicy::TRACK_ONLY_AT_NIGHT, "Only At Night"},
    {SpaceObject::TrackPolicy::CUSTOM_TRACK_POLICY_1, "Custom Policy 1"},
    {SpaceObject::TrackPolicy::CUSTOM_TRACK_POLICY_2, "Custom Policy 2"}
};

// Full Constructor
SpaceObject::SpaceObject(QString name, QString ILRSname, QString alias, QString cospar, QString ILRScospar,
                         QString norad, QString sic, QString classification, QString laserid, QString detectorid,
                         QString counterid, QString cpfprovider, QString picture, QString comments,
                         TrackPolicy trackpolicy, EnablementPolicy enablementpolicy,
                         int npi, int bs, int altitude, int amplification, int priority,
                         double inclination, double rcs, bool llr, bool debris,
                         QMap<QString, QString>& extraparameters)
    : name(name), ILRSname(ILRSname), alias(alias), cospar(cospar), ILRSID(ILRScospar),
    norad(norad), sic(sic), classification(classification), laserid(laserid), detectorid(detectorid),
    counterid(counterid), cpfprovider(cpfprovider), picture(picture), comments(comments),
    trackpolicy(trackpolicy), enablementpolicy(enablementpolicy),
    npi(npi), bs(bs), altitude(altitude), amplification(amplification), priority(priority),
    inclination(inclination), rcs(rcs), lrr(llr), debris(debris),
    extraparameters(extraparameters), llr_checked(true), debris_checked(true), initialized(true)
{
}

// Default Constructor
SpaceObject::SpaceObject()
    : name(""), ILRSname(""), alias(""), cospar(""), ILRSID(""), norad(""), sic(""), classification(""),
    laserid(""), detectorid(""), counterid(""), cpfprovider(""), picture(""), comments(""),
    trackpolicy(NO_TRACK_POLICY), enablementpolicy(NO_ENABLEMENT_POLICY), npi(0), bs(0), altitude(0),
    amplification(0), priority(-1), inclination(0), rcs(0), lrr(false), debris(false),
    llr_checked(false), debris_checked(false), initialized(false)
{
}

const QJsonObject SpaceObject::toJson() const
{
    QJsonObject object;

    // Lambda to insert non-empty values or explicit nulls
    auto insertSafe = [&](const QString& key, const QString& val) {
        if (val.isEmpty()) object.insert(key, QJsonValue::Null);
        else object.insert(key, val);
    };

    insertSafe("Name", name);
    insertSafe("ILRSName", ILRSname);
    insertSafe("Alias", alias);
    insertSafe("COSPAR", cospar);
    insertSafe("ILRSID", ILRSID);
    insertSafe("NORAD", norad);
    insertSafe("SIC", sic);
    insertSafe("Classification", classification);
    insertSafe("LaserID", laserid);
    insertSafe("DetectorID", detectorid);
    insertSafe("CounterID", counterid);
    insertSafe("ProviderCPF", cpfprovider);
    insertSafe("Comments", comments);

    // Cast enums to int for serialization
    object.insert("TrackPolicy", static_cast<int>(this->trackpolicy));
    object.insert("EnablementPolicy", static_cast<int>(this->enablementpolicy));

    object.insert("NormalPointIndicator", this->npi);
    object.insert("BinSize", this->bs);
    object.insert("Altitude", this->altitude);
    object.insert("Amplification", this->amplification);
    object.insert("Priority", this->priority);
    object.insert("Inclination", this->inclination);
    object.insert("RadarCrossSection", this->rcs);
    object.insert("LaserRetroReflector", this->lrr);
    object.insert("IsDebris", this->debris);

    // Serialize map of extra parameters
    QJsonObject extra;
    QMapIterator<QString, QString> i(this->extraparameters);
    while (i.hasNext()) {
        i.next();
        extra.insert(i.key(), i.value());
    }
    object.insert("ExtraParameters", extra);

    return object;
}

const QString SpaceObject::toString() const
{
    return QString("SpaceObject(Name: %1, NORAD: %2)").arg(this->name, this->norad);
}

QJsonObject SpaceObject::generateEmptyJsonobject()
{
    SpaceObject empty;
    return empty.toJson();
}

SpaceObject::SpaceObject(const QJsonValue &json_value, const QStringList &extrakeys)
    : SpaceObject()
{
    if(json_value.isObject()){
        QJsonObject obj = json_value.toObject();

        auto readStr = [&](const QString& key) { return obj[key].toString(""); };

        this->name = readStr("Name");
        this->ILRSname = readStr("ILRSName");
        this->alias = readStr("Alias");
        this->cospar = readStr("COSPAR");
        this->ILRSID = readStr("ILRSID");
        this->norad = readStr("NORAD");
        this->sic = readStr("SIC");
        this->classification = readStr("Classification");
        this->laserid = readStr("LaserID");
        this->detectorid = readStr("DetectorID");
        this->counterid = readStr("CounterID");
        this->cpfprovider = readStr("ProviderCPF");
        this->comments = readStr("Comments");

        this->trackpolicy = static_cast<TrackPolicy>(obj["TrackPolicy"].toInt(0));
        this->enablementpolicy = static_cast<EnablementPolicy>(obj["EnablementPolicy"].toInt(0));

        this->npi = obj["NormalPointIndicator"].toInt(0);
        this->bs = obj["BinSize"].toInt(0);
        this->altitude = obj["Altitude"].toInt(0);
        this->amplification = obj["Amplification"].toInt(0);
        this->priority = obj["Priority"].toInt(0);

        this->inclination = obj["Inclination"].toDouble(0.0);
        this->rcs = obj["RadarCrossSection"].toDouble(0.0);

        this->lrr = obj["LaserRetroReflector"].toBool(false);
        this->debris = obj["IsDebris"].toBool(false);

        this->llr_checked = true;
        this->debris_checked = true;

        // Parse Extra Parameters map
        if (obj.contains("ExtraParameters")) {
            QJsonObject extraObj = obj["ExtraParameters"].toObject();
            for(auto key : extraObj.keys()) {
                this->extraparameters.insert(key, extraObj[key].toString());
            }
        }

        // Dynamically add specified extra keys if they exist in root JSON
        for (const QString &key : extrakeys) {
            if (obj.contains(key) && !this->extraparameters.contains(key)) {
                this->extraparameters.insert(key, obj[key].toString());
            }
        }

        this->initialized = true;
    }
}
