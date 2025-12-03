// ======================================================================
/*!
 * \brief Configuration file API
 */
// ======================================================================

#ifndef TEXTGEN_CONFIG_H
#define TEXTGEN_CONFIG_H

#include <calculator/WeatherArea.h>
#include <engines/gis/Engine.h>
#include <engines/gis/GeometryStorage.h>
#include <macgyver/AsyncTask.h>
#include <macgyver/DirectoryMonitor.h>
#include <spine/Thread.h>
#include <libconfig.h++>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class ProductConfig;
using ConfigItem = std::pair<std::string, std::string>;
using ConfigItemVector = std::vector<ConfigItem>;
using ProductConfigMap = std::map<std::string, std::shared_ptr<ProductConfig> >;
using ParameterMappings = std::map<std::string, std::string>;
using WeatherAreas = std::map<std::string, TextGen::WeatherArea>;
using ProductWeatherAreaMap = std::map<std::string, WeatherAreas>;

struct db_connect_info
{
  std::string host;
  unsigned int port{5432};
  std::string username;
  std::string password;
  std::string database;
  std::string schema{"textgen"};
  std::string encoding{"UTF8"};
  unsigned int connect_timeout{30};
};

using DatabaseConnectInfo = std::map<std::string, db_connect_info>;

class ProductConfig
{
 public:
  ProductConfig(const std::string& configfile,
                const std::shared_ptr<ProductConfig>& pDefaultConf,
                const std::string& dictionary);
  ProductConfig(const ProductConfig& other) = delete;
  ProductConfig& operator=(const ProductConfig& other) = delete;

  const std::string& language() const { return itsLanguage; }
  const std::string& formatter() const { return itsFormatter; }
  const std::string& timeFormat() const { return itsTimeFormat; }
  const std::string& forestfirewarning_directory() const { return itsForestFireWarningDirectory; }
  const libconfig::Config& config() const { return itsConfig; }
  const std::string& getAreaTimeZone(const std::string& area) const;

  const Engine::Gis::postgis_identifier& getDefaultPostGISIdentifier() const;
  std::size_t numberOfPostGISIdentifiers() const { return postgis_identifiers.size(); }
  Engine::Gis::PostGISIdentifierVector getPostGISIdentifiers() const;

  std::size_t numberOfForecastDataConfigs() const { return forecast_data_config_items.size(); }
  // returns forecast name, querydata name pair
  const std::pair<std::string, std::string>& getForecastDataConfig(const unsigned int& index) const;
  std::size_t numberOfUnitFormatConfigs() const { return unit_format_config_items.size(); }
  // returns unit name, format pair, e.g ("celcius", "SI")
  const std::pair<std::string, std::string>& getUnitFormatConfig(const unsigned int& index) const;
  std::size_t numberOfOutputDocumentConfigs() const { return output_document_config_items.size(); }
  // returns parameter name, value pair
  const std::pair<std::string, std::string>& getOutputDocumentConfig(
      const unsigned int& index) const;
  std::size_t numberOfAreaConfigs() const { return area_config_items.size(); }
  // returns parameter name, value pair
  const std::pair<std::string, std::string>& getAreaConfig(const unsigned int& index) const;
  std::size_t numberOfMasks() const { return masks.size(); }
  // returns mask name and value, value is either svg-file path or PostGIS name of the mask
  const std::pair<std::string, std::string>& getMask(const unsigned int& index) const;
  std::size_t numberOfFireWarningAreaCodes() const { return forestfirewarning_areacodes.size(); }
  // returns areacode name and number
  const std::pair<std::string, std::string>& getFireWarningAreaCode(
      const unsigned int& index) const;
  const ParameterMappings& getParameterMappings() const { return itsParameterMappings; }

  void setDefaultConfig(const std::shared_ptr<ProductConfig>& pDefaultConf);

  const ConfigItemVector& getForecastDataConfigs() const { return forecast_data_config_items; }
  const ConfigItemVector& getUnitFormatConfigs() const { return unit_format_config_items; }
  const ConfigItemVector& getOutputDocumentConfigs() const { return output_document_config_items; }
  const ConfigItemVector& getAreaConfigs() const { return area_config_items; }
  const ConfigItemVector& getMasks() const { return masks; }
  const ConfigItemVector& getForestFireWarningAreaCodes() const
  {
    return forestfirewarning_areacodes;
  }
  bool isFrostSeason() const { return itsFrostSeason; }
  bool isModified(size_t interval) const;

 private:
  libconfig::Config itsConfig;
  std::map<std::string, std::string> area_timezones;
  std::map<std::string, Engine::Gis::postgis_identifier> postgis_identifiers;
  std::string itsDefaultPostGISIdentifierKey;
  ConfigItemVector masks;

  ConfigItemVector forecast_data_config_items;
  ConfigItemVector unit_format_config_items;
  ConfigItemVector output_document_config_items;
  ConfigItemVector area_config_items;
  ConfigItemVector forestfirewarning_areacodes;

  std::string itsLanguage;
  std::string itsFormatter;
  std::string itsTimeFormat;

  std::string itsForestFireWarningDirectory;
  ParameterMappings itsParameterMappings;
  bool itsFrostSeason = false;
  size_t itsLastModifiedTime = 0;  // epoch seconds

  std::shared_ptr<ProductConfig> pDefaultConfig;
  const std::map<std::string, Engine::Gis::postgis_identifier>& getPostGISIdentifiersPrivate()
  {
    return postgis_identifiers;
  }

  friend class Config;
};

class Config : private boost::noncopyable
{
 public:
  Config(std::string configfile);
  virtual ~Config();
  void init(SmartMet::Engine::Gis::Engine* pGisEngine);
  void shutdown();

  int getForecastTextCacheSize() const { return itsForecastTextCacheSize; }
  const ProductConfig& getProductConfig(const std::string& config_name) const;
  bool geoObjectExists(const std::string& postGISName, const std::string& areasource) const;
  TextGen::WeatherArea makePostGisArea(const std::string& postGISName,
                                       const std::string& areasource) const;
  const WeatherAreas& getProductMasks(const std::string& product_name) const;

  bool productConfigExists(const std::string& config_name) const;

  const std::string& defaultUrl() const { return itsDefaultUrl; }
  const std::set<std::string>& supportedLanguages() const { return itsSupportedLanguages; }
  const std::string& dictionary() const { return itsDictionary; }
  const db_connect_info& getDatabaseConnectInfo(const std::string& dbId) const
  {
    return itsDatabaseConnectInfo.at(dbId);
  }
  db_connect_info& getDatabaseConnectInfo(const std::string& dbId)
  {
    return itsDatabaseConnectInfo.at(dbId);
  }
  //  bool exists(const std::string& dbId) const { return (itsDatabaseConnectInfo.find(dbId) !=
  //  itsDatabaseConnectInfo.end()); }
  const std::string& fileDictionaries() const { return itsFileDictionaries; }

  // Mutex for updating Configuration
  SmartMet::Spine::MutexType itsConfigUpdateMutex;

 private:
  std::unique_ptr<ProductConfigMap> itsProductConfigs;
  // Geometries and their svg-representations are stored here
  std::unique_ptr<Engine::Gis::GeometryStorage> itsGeometryStorage;
  // Here we store masks by product
  std::unique_ptr<ProductWeatherAreaMap> itsProductMasks;

  std::string itsDefaultUrl;
  int itsForecastTextCacheSize = 0;

  Fmi::DirectoryMonitor itsMonitor;
  boost::thread itsMonitorThread;

  // callback requests
  void update(Fmi::DirectoryMonitor::Watcher id,
              const std::filesystem::path& dir,
              const boost::regex& pattern,
              const Fmi::DirectoryMonitor::Status& status);
  ConfigItemVector readMainConfig() const;
  std::unique_ptr<ProductConfigMap> updateProductConfigs(const ConfigItemVector& configItems,
                                                         const std::set<std::string>& deletedFiles,
                                                         const std::set<std::string>& modifiedFiles,
                                                         const std::set<std::string>& newFiles);
  std::set<std::string> getDirectoriesToMonitor(const ConfigItemVector& configItems) const;
  void setDefaultConfigValues(ProductConfigMap& productConfigs);
  std::unique_ptr<Engine::Gis::GeometryStorage> loadGeometries(
      const std::unique_ptr<ProductConfigMap>& pgs);

  bool itsShowFileMessages = false;
  std::string itsMainConfigFile;
  std::string itsDictionary;
  std::set<std::string> itsSupportedLanguages;
  DatabaseConnectInfo itsDatabaseConnectInfo;
  std::string itsFileDictionaries;

  SmartMet::Engine::Gis::Engine* itsGisEngine = nullptr;

  std::unique_ptr<Fmi::AsyncTask> config_update_task;
};  // class Config

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  //  TEXTGEN_CONFIG_H

// ======================================================================
