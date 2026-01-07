#pragma once

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

/**
 * @brief Represents a named set of space objects.
 *
 * This class encapsulates a collection of space objects identified by a name
 * and a list of enabled items.
 */
class SpaceObjectSet
{
public:
    /**
     * @brief Constructs a new SpaceObjectSet.
     * @param name The name of the set.
     * @param enabled The list of enabled items in this set.
     */
    SpaceObjectSet(const QString& name, const QStringList& enabled);

    /**
     * @brief Gets the name of the set.
     * @return A constant reference to the name string.
     */
    inline const QString& getName() const { return this->name; }

    /**
     * @brief Sets the name of the set.
     * @param name The new name string.
     */
    inline void setName(const QString& name) { this->name = name; }

    /**
     * @brief Gets the list of enabled items.
     * @return A constant reference to the string list.
     */
    inline const QStringList& getEnabled() const { return this->list_enabled; }

    /**
     * @brief Sets the list of enabled items.
     * @param enabled The new list of enabled items.
     */
    inline void setEnabled(const QStringList& enabled) { this->list_enabled = enabled; }

private:
    QString name; ///< The name of the set.
    QStringList list_enabled; ///< The list of enabled object identifiers.
};

/**
 * @brief Represents a physical space object (e.g., satellite, debris).
 *
 * This class stores all relevant metadata for a space object, including its identifiers
 * (NORAD, COSPAR, etc.), physical properties (RCS, Altitude), and tracking policies.
 * It provides methods for serialization to and from JSON (using Qt's JSON classes).
 *
 * @note This is a "Legacy" class required for binary compatibility with older plugins.
 * The core application logic may use different JSON structures (nlohmann::json).
 */
class SpaceObject
{
public:
    static constexpr const char* kAllCPFProvider = "All";
    static constexpr const char* kNoTLEProvider = "No TLE";

    /** @brief Defines tracking behavior policies. */
    enum TrackPolicy {
        NO_TRACK_POLICY,
        TRACK_ALWAYS,
        TRACK_ONLY_IF_VISIBLE,
        TRACK_ONLY_AT_NIGHT,
        CUSTOM_TRACK_POLICY_1,
        CUSTOM_TRACK_POLICY_2
    };

    /** @brief Defines operational status policies. */
    enum EnablementPolicy {
        NO_ENABLEMENT_POLICY,
        DISABLED,
        ENABLED,
        ALWAYS_DISABLED,
        ALWAYS_ENABLED
    };

    // Static mappings for Enum <-> String conversion
    static const QMap<TrackPolicy, QString> TrackPolicyStringMap;
    static const QMap<EnablementPolicy, QString> EnablementPolicyStringMap;
    static const QMap<QString, SpaceObject::EnablementPolicy> EnablementPolicyEnumMap;

    /**
     * @brief Full constructor initializing all fields.
     */
    SpaceObject(QString name, QString ILRSname, QString alias, QString cospar, QString ILRScospar,
                QString norad, QString sic, QString classification, QString laserid, QString detectorid,
                QString counterid, QString cpfprovider, QString picture, QString comments,
                TrackPolicy trackpolicy, EnablementPolicy enablementpolicy,
                int npi, int bs, int altitude, int amplification, int priority,
                double inclination, double rcs, bool llr, bool debris,
                QMap<QString, QString>& extraparameters);

    /**
     * @brief Default constructor. Initializes an empty object.
     */
    SpaceObject();

    /**
     * @brief Constructs a SpaceObject from a QJsonValue.
     * @param json_value The JSON source (typically a QJsonObject).
     * @param extrakeys List of additional keys to extract into the 'extraparameters' map.
     */
    SpaceObject(const QJsonValue &json_value, const QStringList &extrakeys);

    // Getters
    inline const QString& getName()            const { return this->name; }
    inline const QString& getILRSname()        const { return this->ILRSname; }
    /** @brief Returns ILRS Name if available, otherwise the standard Name. */
    inline const QString& getPreferredName()   const { return this->ILRSname.isEmpty() ? this->name :  this->ILRSname; }
    inline const QString& getAlias()           const { return this->alias; }
    inline const QString& getCospar()          const { return this->cospar; }
    inline const QString& getILRSID()          const { return this->ILRSID; }
    inline const QString& getNorad()           const { return this->norad; }
    inline const QString& getSIC()             const { return this->sic; }
    inline const QString& getClassification()  const { return this->classification; }
    inline const QString& getLaserID()         const { return this->laserid; }
    inline const QString& getDetectorID()      const { return this->detectorid; }
    inline const QString& getCounterID()       const { return this->counterid; }
    inline const QString& getCPFProvider()     const { return this->cpfprovider; }
    inline const QString& getPicture()         const { return this->picture; }
    inline const QString& getComments()        const { return this->comments; }
    inline TrackPolicy getTrackPolicy()        const { return this->trackpolicy; }
    inline EnablementPolicy getEnablementPolicy() const { return this->enablementpolicy; }
    inline const int& getNormalPointIndicator()   const { return this->npi; }
    inline const int& getBinSize()                const { return this->bs; }
    inline const int& getAltitude()               const { return this->altitude; }
    inline const int& getAmplification()          const { return this->amplification; }
    inline const int& getPriority()               const { return this->priority; }
    inline const double& getInclination()         const { return this->inclination; }
    inline const double& getRadarCrossSection()   const { return this->rcs; }
    inline const bool& hasLRR()  const { return this->lrr; }
    inline const bool& isDebris()                 const { return this->debris; }
    inline const QMap<QString, QString>& getExtraParameters() const { return this->extraparameters; }

    // Status flags
    inline const bool& isLaserRetroReflectorChecked()         const { return this->llr_checked; }
    inline const bool& isDebrisChecked()            const { return this->debris_checked; }
    inline const bool& isInitialized()              const { return this->initialized; }

    // Setters
    inline void setName(QString name)                 { this->name = name; }
    inline void setILRSname(QString ILRSname)         { this->ILRSname = ILRSname; }
    inline void setAlias(QString alias)               { this->alias = alias; }
    inline void setCospar(QString cospar)             { this->cospar = cospar; }
    inline void setILRScospar(QString ILRScospar)     { this->ILRSID = ILRScospar; }
    inline void setNorad(QString norad)               { this->norad = norad; }
    inline void setSic(QString sic)                   { this->sic = sic; }
    inline void setClassification(QString classifi)   { this->classification = classifi; }
    inline void setLaserID(QString laserid)           { this->laserid = laserid; }
    inline void setDetectorID(QString detectorid)     { this->detectorid = detectorid; }
    inline void setCounterID(QString counterid)       { this->counterid = counterid; }
    inline void setCPFProvider(QString cpf)           { this->cpfprovider = cpf; }
    inline void setPicture(QString picture)           { this->picture = picture; }
    inline void setComments(QString comments)         { this->comments = comments; }
    inline void setTrackPolicy(TrackPolicy policy)          { this->trackpolicy = policy; }
    inline void setEnablementPolicy(EnablementPolicy policy){ this->enablementpolicy = policy; }
    inline void setNormalPointIndicator(int npi)      { this->npi = npi; }
    inline void setBinSize(int bs)                    { this->bs = bs; }
    inline void setAltitude(int altitude)             { this->altitude = altitude; }
    inline void setAmplification(int amplification)   { this->amplification = amplification; }
    inline void setPriority(int priority)             { this->priority = priority; }
    inline void setInclination(double inclination)    { this->inclination = inclination; }
    inline void setRadarCrossSection(double rcs)      { this->rcs = rcs; }
    inline void setHasLRR(bool llr) { this->lrr = llr; this->llr_checked = true; }
    inline void setIsDebris(bool debris)              { this->debris = debris; this->debris_checked = true; }
    inline void setExtraParameters(const QMap<QString, QString>& extra){ this->extraparameters = extra; }
    inline void setInitialized(bool ini)  { this->initialized = ini; }

    /**
     * @brief Returns a string representation of the object (Name and NORAD).
     */
    const QString toString() const;

    /**
     * @brief Serializes the object into a QJsonObject.
     * Used for saving data or passing to legacy plugins.
     */
    const QJsonObject toJson() const;

    /**
     * @brief Helper to generate an empty JSON object.
     */
    static QJsonObject generateEmptyJsonobject();

protected:
    // Core Identity Fields
    QString name;
    QString ILRSname;
    QString alias;
    QString cospar;
    QString ILRSID;
    QString norad;
    QString sic;
    QString classification;
    QString laserid;
    QString detectorid;
    QString counterid;
    QString cpfprovider;
    QString picture;
    QString comments;

    // Policies and Properties
    TrackPolicy trackpolicy;
    EnablementPolicy enablementpolicy;
    int npi;
    int bs;
    int altitude;
    int amplification;
    int priority;
    double inclination;
    double rcs;
    bool lrr;
    bool debris;

    // Auxiliary
    QMap<QString, QString> extraparameters;
    bool llr_checked;
    bool debris_checked;
    bool initialized;
};

// Register types for use with QVariant
Q_DECLARE_METATYPE(SpaceObject*)
Q_DECLARE_METATYPE(SpaceObjectSet*)
