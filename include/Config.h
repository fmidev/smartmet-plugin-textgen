// ======================================================================
/*!
 * \brief Configuration file API
 */
// ======================================================================

#ifndef TEXTGEN_CONFIG_H
#define TEXTGEN_CONFIG_H

#include <spine/PostGISDataSource.h>

#include <libconfig.h++>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <map>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
typedef std::pair<std::string, std::string> config_item;
typedef std::vector<config_item> config_item_vector;

class ProductConfig : private boost::noncopyable
{
 public:
  ProductConfig(const std::string& configfile);

  const std::string& mySQLDictionaryHost() const { return itsMySQLDictionaryHost; }
  const std::string& mySQLDictionaryDatabase() const { return itsMySQLDictionaryDatabase; }
  const std::string& mySQLDictionaryUsername() const { return itsMySQLDictionaryUsername; }
  const std::string& mySQLDictionaryPassword() const { return itsMySQLDictionaryPassword; }
  const std::string& language() const { return itsLanguage; }
  const std::string& formatter() const { return itsFormatter; }
  const std::string& locale() const { return itsLocale; }
  const std::string& timeFormat() const { return itsTimeFormat; }
  const std::string& dictionary() const { return itsDictionary; }
  const std::string& forestfirewarning_directory() const { return itsForestFireWarningDirectory; }
  const libconfig::Config& config() const { return itsConfig; }
  const std::string& getAreaTimeZone(const std::string& area) const;

  std::vector<std::string> getPostGISIdentifierKeys() const;
  const SmartMet::Spine::postgis_identifier& getDefaultPostGISIdentifier() const;
  const SmartMet::Spine::postgis_identifier& getPostGISIdentifier(const std::string& key) const;
  std::size_t numberOfPostGISIdentifiers() const { return postgis_identifiers.size(); }
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

  void setDefaultConfig(const boost::shared_ptr<ProductConfig> pDefaultConf);

  const std::map<std::string, SmartMet::Spine::postgis_identifier>& getPostGISIdentifiers() const
  {
    return postgis_identifiers;
  }
  const config_item_vector& getForecastDataConfigs() const { return forecast_data_config_items; }
  const config_item_vector& getUnitFormatConfigs() const { return unit_format_config_items; }
  const config_item_vector& getOutputDocumentConfigs() const
  {
    return output_document_config_items;
  }
  const config_item_vector& getAreaConfigs() const { return area_config_items; }
  const config_item_vector& getMasks() const { return masks; }
  const config_item_vector& getForestFireWarningAreaCodes() const
  {
    return forestfirewarning_areacodes;
  }
  bool isFrostSeason() const { return itsFrostSeason; }
 private:
  libconfig::Config itsConfig;
  std::map<std::string, std::string> area_timezones;
  std::map<std::string, SmartMet::Spine::postgis_identifier> postgis_identifiers;
  std::string itsDefaultPostGISIdentifierKey;
  config_item_vector masks;

  config_item_vector forecast_data_config_items;
  config_item_vector unit_format_config_items;
  config_item_vector output_document_config_items;
  config_item_vector area_config_items;
  config_item_vector forestfirewarning_areacodes;

  std::string itsLanguage;
  std::string itsFormatter;
  std::string itsLocale;
  std::string itsTimeFormat;
  std::string itsDictionary;
  std::string itsMySQLDictionaryHost;
  std::string itsMySQLDictionaryDatabase;
  std::string itsMySQLDictionaryUsername;
  std::string itsMySQLDictionaryPassword;
  std::string itsForestFireWarningDirectory;

  bool itsFrostSeason;

  boost::shared_ptr<ProductConfig> pDefaultConfig;

  friend class Config;
};

class Config : private boost::noncopyable
{
 public:
  Config(const std::string& configfile);

  int getForecastTextCacheSize() { return itsForecastTextCacheSize; }
  const ProductConfig& getProductConfig(const std::string& config_name) const;
  std::vector<std::string> getProductNames() const;
  bool productConfigExists(const std::string& config_name)
  {
    return itsProductConfigs.find(config_name) != itsProductConfigs.end();
  }

  const std::string& defaultUrl() const { return itsDefaultUrl; }
 private:
  typedef std::pair<std::string, boost::shared_ptr<ProductConfig> > product_config_item;
  std::map<std::string, boost::shared_ptr<ProductConfig> > itsProductConfigs;
  std::string itsDefaultUrl;
  int itsForecastTextCacheSize;
};  // class Config

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  //  TEXTGEN_CONFIG_H

// ======================================================================
