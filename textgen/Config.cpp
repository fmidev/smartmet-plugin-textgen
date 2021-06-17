// ======================================================================
/*!
 * \brief Implementation of Config
 */
// ======================================================================

#include "Config.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <calculator/TextGenPosixTime.h>
#include <engines/gis/Engine.h>
#include <engines/gis/Normalize.h>
#include <macgyver/AnsiEscapeCodes.h>
#include <macgyver/Exception.h>
#include <macgyver/StringConversion.h>
#include <newbase/NFmiFileSystem.h>
#include <spine/Convenience.h>
#include <iostream>
#include <memory>
#include <stdexcept>

static const char* default_url = "/textgen";
static const char* default_language = "fi";
static const char* default_locale = "fi_FI.UTF-8";
static const char* default_formatter = "html";
static const char* default_timeformat = "iso";
static const char* default_dictionary = "multimysql";
static const char* default_timezone = "Europe/Helsinki";
static const char* default_textgen_config_name = "default";

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
#define DEFAULT_FORECAST_TEXT_CACHE_SIZE 20
#define DEFAULT_FROSTSEASON 0

namespace
{
TextGen::WeatherArea make_area(const std::string& postGISName,
                               const std::unique_ptr<Engine::Gis::GeometryStorage>& geometryStorage)
{
  try
  {
    std::string areaName(postGISName);
    Engine::Gis::normalize_string(areaName);

    if (geometryStorage->isPolygon(postGISName))
    {
      std::stringstream svg_string_stream(geometryStorage->getSVGPath(postGISName));
      NFmiSvgPath svgPath;
      svgPath.Read(svg_string_stream);
      return {svgPath, areaName};
    }

    // if not polygon, it must be a point
    std::pair<float, float> std_point(geometryStorage->getPoint(postGISName));
    NFmiPoint point(std_point.first, std_point.second);
    return {point, areaName};
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
std::string parse_config_key(const char* str1 = nullptr,
                             const char* str2 = nullptr,
                             const char* str3 = nullptr)
{
  std::string string1(str1 != nullptr ? str1 : "");
  std::string string2(str2 != nullptr ? str2 : "");
  std::string string3(str3 != nullptr ? str3 : "");

  std::string retval(string1 + string2 + string3);

  return retval;
}

void parseConfigurationItem(const libconfig::Config& itsConfig,
                            const std::string& key,
                            const std::vector<std::string>& allowed_sections,
                            ConfigItemVector& config_item_container)
{
  try
  {
    if (itsConfig.exists(key))
    {
      if (itsConfig.lookup(key).isGroup())
      {
        bool processSection(false);
        for (auto allowed_key : allowed_sections)
        {
          std::string section_key = key;
          if (allowed_key.size() > section_key.size())
            allowed_key = allowed_key.substr(0, section_key.size());
          else if (allowed_key.size() < section_key.size())
            section_key = section_key.substr(0, allowed_key.size());

          if (allowed_key == section_key || allowed_key == "*")
          {
            processSection = true;
            break;
          }
        }
        if (!processSection)
          return;
      }

      libconfig::Setting& setting = itsConfig.lookup(key);
      if (setting.isGroup())
      {
        int length(setting.getLength());
        for (int i = 0; i < length; i++)
        {
          libconfig::Setting& childSetting = setting[i];
          parseConfigurationItem(itsConfig,
                                 key + "." + childSetting.getName(),
                                 allowed_sections,
                                 config_item_container);
        }
      }
      else if (setting.isArray())
      {
        // cout << key << " is an array" << endl;
      }
      else if (setting.isList())
      {
        // cout << key << " is a list" << endl;
      }
      else
      {
        std::stringstream ss;
        switch (setting.getType())
        {
          case libconfig::Setting::TypeInt:
          {
            int intValue = itsConfig.lookup(key);
            ss << intValue;
            break;
          }
          case libconfig::Setting::TypeInt64:
          {
            long longValue = itsConfig.lookup(key);
            ss << longValue;
            break;
          }
          case libconfig::Setting::TypeFloat:
          {
            float floatValue = itsConfig.lookup(key);
            ss << floatValue;
            break;
          }
          case libconfig::Setting::TypeString:
          {
            std::string stringValue = itsConfig.lookup(key);
            ss << stringValue;
            break;
          }
          case libconfig::Setting::TypeBoolean:
          {
            bool boolValue = itsConfig.lookup(key);
            ss << (boolValue ? "true" : "false");
            break;
          }
          case libconfig::Setting::TypeNone:
          case libconfig::Setting::TypeGroup:
          case libconfig::Setting::TypeArray:
          case libconfig::Setting::TypeList:
            throw Fmi::Exception(BCP, "TextGen: Invalid setting type for '" + key + "'");
        };

        config_item_container.push_back(std::make_pair(key, ss.str()));
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void handleIncludedSections(const libconfig::Config& itsConfig,
                            ConfigItemVector& config_item_container)
{
  try
  {
    ConfigItemVector all_output_document_config_items;
    std::vector<std::string> allowed_sections;
    allowed_sections.emplace_back("*");
    parseConfigurationItem(
        itsConfig, "output_document", allowed_sections, all_output_document_config_items);

    size_t numberOfConfigs = config_item_container.size();
    // these configurations are included by another and thus not use directly
    ConfigItemVector included_config_items;

    bool reProcess(false);
    for (unsigned int i = 0; i < numberOfConfigs; i++)
    {
      std::string config_key_original(config_item_container[i].first);
      std::string config_value_original(config_item_container[i].second);

      if (config_value_original.size() > 4 && config_value_original.substr(0, 4) == "use ")
      {
        std::string section_to_include(config_value_original.substr(4));

        for (const auto& output_document_config_item : all_output_document_config_items)
        {
          std::string item_key = output_document_config_item.first;
          if (item_key.size() >= section_to_include.size() &&
              item_key.substr(0, section_to_include.size()) == section_to_include)
          {
            std::string config_key_first_part =
                config_key_original.substr(0, config_key_original.find_last_of('.'));
            std::string config_key_second_part = item_key.substr(section_to_include.size());
            std::string parsed_config_key = config_key_first_part + config_key_second_part;
            std::string config_value = output_document_config_item.second;
            if (config_value.size() > 4 && config_value.substr(0, 4) == "use ")
              reProcess = true;

            included_config_items.push_back(std::make_pair(parsed_config_key, config_value));
          }
        }
      }
    }

    if (reProcess)
      handleIncludedSections(itsConfig, included_config_items);

    if (!included_config_items.empty())
      config_item_container.insert(config_item_container.begin(),
                                   included_config_items.begin(),
                                   included_config_items.end());

    sort(config_item_container.begin(), config_item_container.end());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// read all string type parameters
void parseTypeStringConfiguationItem(const libconfig::Config& itsConfig,
                                     const std::string& key,
                                     ConfigItemVector& config_item_container)
{
  try
  {
    if (itsConfig.exists(key))
    {
      libconfig::Setting& settings = itsConfig.lookup(key);
      if (settings.isGroup())
      {
        int length(settings.getLength());
        for (int i = 0; i < length; i++)
        {
          libconfig::Setting& childSettings = settings[i];
          if (childSettings.isGroup())
          {
            parseTypeStringConfiguationItem(
                itsConfig, childSettings.getName(), config_item_container);
          }
          else
          {
            if (childSettings.getType() == libconfig::Setting::TypeString)
            {
              std::string name = childSettings.getName();
              std::string value = childSettings;
              if (value.size() > 4 && value.substr(0, 4) == "use ")
              {
                std::string included_value = itsConfig.lookup(value.substr(4));
                value = included_value;
              }
              config_item_container.push_back(std::make_pair(name, value));
            }
          }
        }
      }
      else
      {
        if (settings.getType() == libconfig::Setting::TypeString)
        {
          std::string name = settings.getName();
          std::string value = settings;
          config_item_container.push_back(std::make_pair(name, value));
        }
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

Config::Config(const std::string& configfile)
    : itsDefaultUrl(default_url),
      itsForecastTextCacheSize(DEFAULT_FORECAST_TEXT_CACHE_SIZE),
      itsMainConfigFile(configfile)
{
}

Config::~Config()
{
  if (itsMonitorThread.joinable())
  {
    itsMonitor.stop();
    if (config_update_task)
    {
      try
      {
        config_update_task->cancel();
        config_update_task->wait();
      }
      catch (...)
      {
        Fmi::Exception::Trace(BCP, "Config update task failed").printError();
      }
    }
  }
}

void Config::shutdown()
{
  if (config_update_task)
  {
    config_update_task->cancel();
  }
}

void Config::init(SmartMet::Engine::Gis::Engine* pGisEngine)
{
  try
  {
    itsGisEngine = pGisEngine;

    libconfig::Config lconf;
    lconf.readFile(itsMainConfigFile.c_str());

    lconf.lookupValue("forecast_text_cache_size", itsForecastTextCacheSize);
    lconf.lookupValue("url", itsDefaultUrl);

    // Set monitoring directories
    ConfigItemVector configItems = readMainConfig();
    std::set<std::string> emptyset;
    itsProductConfigs.reset(
        updateProductConfigs(configItems, emptyset, emptyset, emptyset).release());
    itsGeometryStorage.reset(loadGeometries(itsProductConfigs).release());
    itsProductMasks.reset(readMasks(itsGeometryStorage, itsProductConfigs).release());

    for (const auto& item : *itsProductConfigs)
    {
      std::string config_name = item.first;
      const boost::shared_ptr<ProductConfig>& productConfig = item.second;
      std::string dictionary = productConfig->dictionary();

      if (boost::algorithm::starts_with(dictionary, "multimysql") &&
          (productConfig->mySQLDictionaryHost().empty() &&
           productConfig->mySQLDictionaryDatabase().empty() &&
           productConfig->mySQLDictionaryUsername().empty() &&
           productConfig->mySQLDictionaryPassword().empty()))
      {
        std::string messageDetails =
            "database_servers.mysql_dictionary section missing in textgenplugin configuration file "
            "'" +
            config_name + "'";
        throw Fmi::Exception::Trace(BCP, "Textgenplugin configuration error!")
            .addDetail(messageDetails);
      }
    }

    boost::regex pattern("^[\\w,\\s-]+\\.[A-Za-z]+$");
    for (auto dir : getDirectoriesToMonitor(configItems))
    {
      itsMonitor.watch(dir,
                       pattern,
                       boost::bind(&Config::update, this, _1, _2, _3, _4),
                       boost::bind(&Config::error, this, _1, _2, _3, _4),
                       5,
                       Fmi::DirectoryMonitor::CREATE | Fmi::DirectoryMonitor::DELETE |
                           Fmi::DirectoryMonitor::MODIFY | Fmi::DirectoryMonitor::ERROR);
    }

    config_update_task.reset(
        new Fmi::AsyncTask("[TextGen] config update watch", [this]() { itsMonitor.run(); }));
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::set<std::string> Config::getDirectoriesToMonitor(const ConfigItemVector& configItems) const
{
  std::set<std::string> ret;

  boost::filesystem::path mainPath = itsMainConfigFile;
  std::string mainPathString = mainPath.parent_path().string() + "/";
  for (auto item : configItems)
  {
    boost::filesystem::path p = item.second;
    std::string path = (p.is_relative() ? mainPathString : "") + p.parent_path().string();
    ret.insert(path);
  }

  return ret;
}

ConfigItemVector Config::readMainConfig() const
{
  try
  {
    libconfig::Config lconf;
    lconf.readFile(itsMainConfigFile.c_str());
    ConfigItemVector ret;
    std::vector<std::string> allowed_sections;
    allowed_sections.emplace_back("*");

    if (lconf.exists("products"))
    {
      libconfig::Setting& setting = lconf.lookup("products");
      if (setting.isArray())
      {
        for (int i = 0; i < setting.getLength(); i++)
        {
          std::string value = setting[i];
          boost::filesystem::path p = value;
          std::string product_name = p.stem().string();
          std::string product_file = p.string();
          ret.emplace_back(std::make_pair(product_name, product_file));
        }
      }
    }
    else
    {
      parseConfigurationItem(lconf, "product_config", allowed_sections, ret);
    }

    return ret;
  }
  catch (...)
  {
    std::string details;
    if (!boost::filesystem::exists(itsMainConfigFile))
      details = "File '" + itsMainConfigFile + "' does not exist!";
    else
      details = "Syntax error in file '" + itsMainConfigFile + "'!";

    throw Fmi::Exception(BCP, "Error reading configuration file!").addDetail(details);
  }
}

std::unique_ptr<ProductConfigMap> Config::updateProductConfigs(
    const ConfigItemVector& configItems,
    const std::set<std::string>& deletedFiles,
    const std::set<std::string>& modifiedFiles,
    const std::set<std::string>& newFiles)
{
  std::string config_value;
  std::set<std::string> erroneousFiles;

  Fmi::AsyncTask::interruption_point();

  try
  {
    boost::filesystem::path config_path(itsMainConfigFile);
    // Read configured products
    std::map<std::string, std::string> products;  // name -> config file
    for (const auto& item : configItems)
    {
      std::string config_key(item.first);
      std::string product_name = config_key.substr(config_key.find('.') + 1);
      std::string product_file = item.second;

      // Make relative paths absolute
      if (!product_file.empty() && product_file[0] != '/')
        product_file = config_path.parent_path().string() + "/" + product_file;
      products[product_name] = product_file;
    }

    auto newProductConfigs = std::unique_ptr<ProductConfigMap>(new ProductConfigMap());
    boost::shared_ptr<ProductConfig> pDefultConfig;
    if (products.find(default_textgen_config_name) != products.end())
    {
      boost::shared_ptr<ProductConfig> productConfig(
          new ProductConfig(products.at(default_textgen_config_name), pDefultConfig));
      newProductConfigs->insert(std::make_pair(default_textgen_config_name, productConfig));
      if (modifiedFiles.find(default_textgen_config_name) != modifiedFiles.end())
      {
        TextGenPosixTime timestamp;
        productConfig->itsLastModifiedTime = timestamp.EpochTime();
      }
      pDefultConfig = newProductConfigs->at(default_textgen_config_name);
    }

    std::string config_file;
    for (const auto& product : products)
    {
      try
      {
        if (product.first == default_textgen_config_name)
          continue;
        std::string config_name = product.first;
        config_file = product.second;

        Fmi::AsyncTask::interruption_point();

        boost::shared_ptr<ProductConfig> productConfig(
            new ProductConfig(config_file, pDefultConfig));
        productConfig->setDefaultConfig(pDefultConfig);
        newProductConfigs->insert(std::make_pair(config_name, productConfig));
        if (modifiedFiles.find(config_file) != modifiedFiles.end())
        {
          TextGenPosixTime timestamp;
          productConfig->itsLastModifiedTime = timestamp.EpochTime();
        }
      }
      catch (const Fmi::Exception& e)
      {
        // If non-fatal error occurred, report it and continue processing the next file
        if (e.getDetailCount() > 0)
        {
          std::cout << ANSI_FG_RED << e.getDetailByIndex(0) << ANSI_FG_DEFAULT << std::endl;
          erroneousFiles.insert(config_file);
        }
      }
      catch (...)
      {
        throw Fmi::Exception(BCP, "Error reading product configuration file '" + config_file + "'");
        erroneousFiles.insert(config_file);
      }
    }

    if (deletedFiles.empty() && modifiedFiles.empty() && newFiles.empty())
      return newProductConfigs;

    if (itsShowFileMessages)
    {
      for (const auto& f : deletedFiles)
        std::cout << ANSI_FG_RED << "File '" << f << "' deleted!" << ANSI_FG_DEFAULT << std::endl;
      for (const auto& f : modifiedFiles)
      {
        // Report successful update
        if (erroneousFiles.find(f) == erroneousFiles.end())
          std::cout << ANSI_FG_GREEN << "File '" << f << "' updated!" << ANSI_FG_DEFAULT
                    << std::endl;
      }
      for (const auto& f : newFiles)
      {
        // Report successful update
        if (erroneousFiles.find(f) == erroneousFiles.end())
          std::cout << ANSI_FG_GREEN << "New file '" << f << "' created!" << ANSI_FG_DEFAULT
                    << std::endl;
      }
    }
    itsShowFileMessages = true;

    Fmi::AsyncTask::interruption_point();

    return newProductConfigs;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Error reading product configuration file '" + config_value + "'");
  }
}  // namespace Textgen

void Config::update(Fmi::DirectoryMonitor::Watcher id,
                    const boost::filesystem::path& dir,
                    const boost::regex& pattern,
                    const Fmi::DirectoryMonitor::Status& status)
{
  std::set<std::string> modifiedFiles;
  std::set<std::string> deletedFiles;
  std::set<std::string> newFiles;
  for (const auto& file_status : *status)
  {
    std::string filename = file_status.first.string();
    if (file_status.second == Fmi::DirectoryMonitor::DELETE)
      deletedFiles.insert(filename);

    if (file_status.second == Fmi::DirectoryMonitor::CREATE)
      newFiles.insert(filename);

    if (file_status.second == Fmi::DirectoryMonitor::MODIFY)
      modifiedFiles.insert(filename);
  }

  ConfigItemVector configItems;
  try
  {
    configItems = readMainConfig();
  }
  catch (const Fmi::Exception& e)
  {
    std::string details = e.getDetailByIndex(0);
    std::cout << ANSI_FG_RED
              << (std::string(e.getDetailByIndex(0)) + " Textgen plugin is now inactive!")
              << ANSI_FG_DEFAULT << std::endl;
    itsProductConfigs->clear();
    return;
  }

  std::unique_ptr<ProductConfigMap> prodConf =
      updateProductConfigs(configItems, deletedFiles, modifiedFiles, newFiles);
  std::unique_ptr<Engine::Gis::GeometryStorage> geomStorage = loadGeometries(prodConf);
  std::unique_ptr<ProductWeatherAreaMap> productMasks = readMasks(geomStorage, prodConf);

  SmartMet::Spine::WriteLock lock(itsConfigUpdateMutex);

  itsProductConfigs.reset(prodConf.release());
  itsGeometryStorage.reset(geomStorage.release());
  itsProductMasks.reset(productMasks.release());
}

void Config::error(Fmi::DirectoryMonitor::Watcher id,
                   const boost::filesystem::path& dir,
                   const boost::regex& pattern,
                   const std::string& message)
{
  try
  {
    std::cout << ANSI_FG_RED << "Error in directory " << dir << " : " << message << ANSI_FG_DEFAULT
              << std::endl;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const ProductConfig& Config::getProductConfig(const std::string& config_name) const
{
  try
  {
    if (itsProductConfigs->find(config_name) == itsProductConfigs->end())
      throw Fmi::Exception(BCP, config_name + " configuration not found!");

    return *(itsProductConfigs->at(config_name));
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::vector<std::string> Config::getProductNames(const std::unique_ptr<ProductConfigMap>& pgs) const
{
  try
  {
    std::vector<std::string> retval;

    for (const auto& pci : *pgs)
    {
      retval.push_back(pci.first);
    }

    return retval;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::unique_ptr<Engine::Gis::GeometryStorage> Config::loadGeometries(
    const std::unique_ptr<ProductConfigMap>& pgs)
{
  auto newGeometryStorage =
      std::unique_ptr<Engine::Gis::GeometryStorage>(new Engine::Gis::GeometryStorage());

  for (const auto& pci : *pgs)
  {
    boost::shared_ptr<ProductConfig> productConfig = pci.second;
    Fmi::AsyncTask::interruption_point();
    itsGisEngine->populateGeometryStorage(productConfig->getPostGISIdentifiers(),
                                          *newGeometryStorage);
  }

  Fmi::AsyncTask::interruption_point();

  return newGeometryStorage;
}

std::unique_ptr<ProductWeatherAreaMap> Config::readMasks(
    const std::unique_ptr<Engine::Gis::GeometryStorage>& gs,
    const std::unique_ptr<ProductConfigMap>& pgs)
{
  try
  {
    auto newProductMasks = std::unique_ptr<ProductWeatherAreaMap>(new ProductWeatherAreaMap());

    // PostGIS
    std::vector<std::string> product_names(getProductNames(pgs));
    for (auto product_name : product_names)
    {
      if (pgs->find(product_name) == pgs->end())
        throw Fmi::Exception(BCP, product_name + " configuration not found!");

      const ProductConfig& config = *(pgs->at(product_name));

      // masks
      WeatherAreas prod_mask;
      for (unsigned int i = 0; i < config.numberOfMasks(); i++)
      {
        std::string name(config.getMask(i).first);
        std::string value(config.getMask(i).second);

        Fmi::AsyncTask::interruption_point();

        // first check if mask can be found in PostGIS database
        if (gs->geoObjectExists(value))
        {
          std::string area_name(value);
          prod_mask.insert(make_pair(name, TextGen::WeatherArea(make_area(area_name, gs))));
        }
        else
        {
          std::string filename(value);
          if (filename.find(':') != std::string::npos)
            filename = filename.substr(0, filename.find(':'));
          // mask is probably a svg-file
          if (NFmiFileSystem::FileExists(filename))
          {
            prod_mask.insert(make_pair(name, TextGen::WeatherArea(value, name)));
          }
        }
      }
      newProductMasks->insert(make_pair(product_name, prod_mask));
    }

    Fmi::AsyncTask::interruption_point();

    return newProductMasks;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool Config::geoObjectExists(const std::string& postGISName, const std::string& areasource) const
{
  std::string shapeKey = (postGISName + areasource);
  Engine::Gis::normalize_string(shapeKey);
  return itsGeometryStorage->geoObjectExists(shapeKey);
}

const WeatherAreas& Config::getProductMasks(const std::string& product_name) const
{
  return itsProductMasks->at(product_name);
}

bool Config::productConfigExists(const std::string& config_name) const
{
  return itsProductConfigs->find(config_name) != itsProductConfigs->end();
}

TextGen::WeatherArea Config::makePostGisArea(const std::string& postGISName,
                                             const std::string& areasource) const
{
  try
  {
    std::string areaName = postGISName;
    std::string shapeKey = (postGISName + areasource);
    Engine::Gis::normalize_string(areaName);
    Engine::Gis::normalize_string(shapeKey);

    if (itsGeometryStorage->isPolygon(shapeKey))
    {
      std::stringstream svg_string_stream(itsGeometryStorage->getSVGPath(shapeKey));
      NFmiSvgPath svgPath;
      svgPath.Read(svg_string_stream);
      return {svgPath, areaName};
    }

    // if not polygon, it must be a point
    std::pair<float, float> std_point(itsGeometryStorage->getPoint(shapeKey));
    NFmiPoint point(std_point.first, std_point.second);
    return {point, areaName};
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

ProductConfig::ProductConfig(const std::string& configfile,
                             const boost::shared_ptr<ProductConfig>& pDefaultConf)
    : itsLanguage(""),
      itsFormatter(""),
      itsLocale(""),
      itsTimeFormat(""),
      itsDictionary(""),
      itsForestFireWarningDirectory(""),
      itsFrostSeason(DEFAULT_FROSTSEASON)
{
  std::string exceptionDetails;
  try
  {
    if (configfile.empty())
      return;

    if (!boost::filesystem::exists(configfile))
    {
      exceptionDetails =
          "Product configuration file '" + configfile + "' not found, please check the filename!";
      throw Fmi::Exception(BCP, configfile + " file does not exist!");
    }

    itsConfig.readFile(configfile.c_str());

    // Parameter mappings
    if (itsConfig.exists("parameter_mapping"))
    {
      ConfigItemVector parameter_mappings;
      std::vector<std::string> allowed_sections;
      allowed_sections.emplace_back("*");
      parseConfigurationItem(itsConfig, "parameter_mapping", allowed_sections, parameter_mappings);
      for (auto item : parameter_mappings)
      {
        std::string configname = item.first;
        std::string qdname = item.second;
        boost::algorithm::replace_all(configname, "parameter_mapping.", "textgen::");
        itsParameterMappings[configname] = qdname;
      }
    }

    // Miscellaneous parameters
    itsConfig.lookupValue("misc.frostseason", itsFrostSeason);
    itsConfig.lookupValue("misc.timeformat", itsTimeFormat);
    itsConfig.lookupValue("misc.language", itsLanguage);
    itsConfig.lookupValue("misc.locale", itsLocale);
    itsConfig.lookupValue("misc.dictionary", itsDictionary);
    itsConfig.lookupValue("misc.filedictionaries", itsFileDictionaries);
    itsConfig.lookupValue("misc.formatter", itsFormatter);

    itsConfig.lookupValue("forestfirewarning.directory", itsForestFireWarningDirectory);

    // MySQL-dictionary
    if (itsConfig.exists("database_servers.mysql_dictionary"))
    {
      itsConfig.lookupValue("database_servers.mysql_dictionary.host", itsMySQLDictionaryHost);
      itsConfig.lookupValue("database_servers.mysql_dictionary.database",
                            itsMySQLDictionaryDatabase);
      itsConfig.lookupValue("database_servers.mysql_dictionary.username",
                            itsMySQLDictionaryUsername);
      itsConfig.lookupValue("database_servers.mysql_dictionary.password",
                            itsMySQLDictionaryPassword);
    }

    // PostGIS

    // Database connection
    Engine::Gis::postgis_identifier default_postgis_id;
    if (pDefaultConf)
      default_postgis_id = pDefaultConf->getDefaultPostGISIdentifier();

    // Geometry tables
    if (itsConfig.exists("geometry_tables"))
    {
      std::string default_source_name;
      std::string default_server;
      std::string default_schema;
      std::string default_table;
      std::string default_field;
      itsConfig.lookupValue("geometry_tables.name", default_source_name);
      itsConfig.lookupValue("geometry_tables.server", default_server);
      itsConfig.lookupValue("geometry_tables.schema", default_schema);
      itsConfig.lookupValue("geometry_tables.table", default_table);
      itsConfig.lookupValue("geometry_tables.field", default_field);

      if (!default_schema.empty() && !default_table.empty() && !default_field.empty())
      {
        default_postgis_id.source_name = default_source_name;
        default_postgis_id.pgname = default_server;
        default_postgis_id.schema = default_schema;
        default_postgis_id.table = default_table;
        default_postgis_id.field = default_field;
        std::string key(default_postgis_id.key());
        itsDefaultPostGISIdentifierKey = key;
        postgis_identifiers.insert(std::make_pair(key, default_postgis_id));
      }

      if (itsConfig.exists("geometry_tables.additional_tables"))
      {
        libconfig::Setting& additionalTables =
            itsConfig.lookup("geometry_tables.additional_tables");

        for (int i = 0; i < additionalTables.getLength(); i++)
        {
          libconfig::Setting& tableConfig = additionalTables[i];
          std::string source_name;
          std::string server =
              (default_server.empty() ? default_postgis_id.pgname : default_server);
          std::string schema =
              (default_schema.empty() ? default_postgis_id.schema : default_schema);
          std::string table = (default_table.empty() ? default_postgis_id.table : default_table);
          std::string field = (default_field.empty() ? default_postgis_id.field : default_field);

          tableConfig.lookupValue("name", source_name);
          tableConfig.lookupValue("server", server);
          tableConfig.lookupValue("schema", schema);
          tableConfig.lookupValue("table", table);
          tableConfig.lookupValue("field", field);

          Engine::Gis::postgis_identifier postgis_id;
          postgis_id.source_name = source_name;
          postgis_id.pgname = server;
          postgis_id.schema = schema;
          postgis_id.table = table;
          postgis_id.field = field;

          std::string key(postgis_id.key());
          if (itsDefaultPostGISIdentifierKey.empty())
            itsDefaultPostGISIdentifierKey = key;
          postgis_identifiers.insert(std::make_pair(key, postgis_id));

          std::string err_msg;
          if (default_schema.empty() && schema.empty())
            err_msg = "No 'schema' defined. ";
          if (default_table.empty() && table.empty())
            err_msg += "No 'table' defined. ";
          if (default_field.empty() && field.empty())
            err_msg += "No 'field' defined.";

          if (!err_msg.empty())
          {
            exceptionDetails =
                "Textgenplugin configuration error in geometry_tables section: " + err_msg;
            throw Fmi::Exception(BCP, "Textgenplugin configuration error!");
          }
        }
      }
    }

    // masks
    if (itsConfig.exists("mask"))
    {
      libconfig::Setting& setting = itsConfig.lookup("mask");
      if (setting.isGroup())
      {
        int length(setting.getLength());
        for (int i = 0; i < length; i++)
        {
          libconfig::Setting& childSetting = setting[i];
          if (childSetting.getType() == libconfig::Setting::TypeString)
          {
            std::string maskName = childSetting.getName();
            std::string maskValue = itsConfig.lookup("mask." + maskName);
            masks.emplace_back(std::make_pair(maskName, maskValue));
          }
        }
      }
    }

    // forestfirewarning
    if (itsConfig.exists("forestfirewarning"))
    {
      libconfig::Setting& setting = itsConfig.lookup("forestfirewarning.areacodes");
      if (setting.isGroup())
      {
        int length(setting.getLength());
        for (int i = 0; i < length; i++)
        {
          libconfig::Setting& childSetting = setting[i];
          std::string name(childSetting.getName());
          std::string key(std::string("forestfirewarning.areacodes.") + childSetting.getName());
          int value = itsConfig.lookup(key);
          std::stringstream ss;
          ss << value;
          forestfirewarning_areacodes.emplace_back(
              std::make_pair("qdtext::forestfirewarning::areacodes::" + name, ss.str()));
        }
      }
    }

    // forecast data
    if (itsConfig.exists("forecast_data"))
    {
      parseTypeStringConfiguationItem(itsConfig, "forecast_data", forecast_data_config_items);
    }

    // unit formats
    if (itsConfig.exists("unit_format"))
    {
      parseTypeStringConfiguationItem(itsConfig, "unit_format", unit_format_config_items);
    }

    // ouput document
    if (itsConfig.exists("output_document"))
    {
      ConfigItemVector output_document_config_item_container;
      // sections
      libconfig::Setting& sections = itsConfig.lookup("output_document.sections");

      if (!sections.isArray())
      {
        throw Fmi::Exception(
            BCP,
            "output_document.sections not an array in textgenplugin configuration file line " +
                Fmi::to_string(sections.getSourceLine()));
      }

      std::string sections_parameter_value;
      std::vector<std::string> allowed_sections;
      // allowed_sections.clear();
      for (int i = 0; i < sections.getLength(); ++i)
      {
        std::string section_name = sections[i];

        if (i > 0)
          sections_parameter_value += ",";
        sections_parameter_value += section_name;
        allowed_sections.emplace_back("output_document." + section_name + ".period");
        allowed_sections.emplace_back("output_document." + section_name + ".subperiod");
        allowed_sections.emplace_back("output_document." + section_name + ".header");

        bool contentExists(itsConfig.exists("output_document." + section_name + ".content"));

        // content
        if (contentExists)
        {
          libconfig::Setting& content =
              itsConfig.lookup("output_document." + section_name + ".content");
          if (!content.isArray())
          {
            if (content.getType() == libconfig::Setting::TypeString)
            {
              std::string story_name = content;
              if (story_name == "none")
                allowed_sections.emplace_back("output_document." + section_name);
              else
              {
                allowed_sections.emplace_back("output_document." + section_name + ".story." +
                                              story_name);
                allowed_sections.emplace_back("output_document." + section_name + ".day1.story." +
                                              story_name);
                allowed_sections.emplace_back("output_document." + section_name + ".day2.story." +
                                              story_name);
              }
            }
          }
          else
          {
            std::string content_parameter_value;
            for (int k = 0; k < content.getLength(); ++k)
            {
              std::string story_name = content[k];
              if (k > 0)
                content_parameter_value += ",";
              content_parameter_value += story_name;

              allowed_sections.emplace_back("output_document." + section_name + ".story." +
                                            story_name);
            }
            output_document_config_item_container.emplace_back(std::make_pair(
                "output_document." + section_name + ".content", content_parameter_value));
          }
        }
        parseConfigurationItem(itsConfig,
                               "output_document." + section_name,
                               allowed_sections,
                               output_document_config_item_container);
      }
      output_document_config_item_container.emplace_back(
          std::make_pair("output_document.sections", sections_parameter_value));

      handleIncludedSections(itsConfig, output_document_config_item_container);

      for (ConfigItem& ci : output_document_config_item_container)
      {
        std::string config_key(ci.first);
        std::string config_value(ci.second);

        if (config_value.size() > 4 && config_value.substr(0, 4) == "use ")
          continue;

        boost::algorithm::replace_all(config_key, ".", "::");
        boost::algorithm::replace_all(config_key, "output_document", "textgen");
        output_document_config_items.emplace_back(std::make_pair(config_key, config_value));
      }
    }

    // area configuration
    if (itsConfig.exists("area"))
    {
      ConfigItemVector area_config_item_container;
      std::vector<std::string> allowed_sections;
      allowed_sections.emplace_back("*");
      // sections
      parseConfigurationItem(itsConfig, "area", allowed_sections, area_config_item_container);

      for (ConfigItem& ci : area_config_item_container)
      {
        std::string config_key(ci.first);
        std::string config_value(ci.second);

        if (config_value.size() > 4 && config_value.substr(0, 4) == "use ")
          continue;

        boost::algorithm::replace_all(config_key, ".", "::");
        boost::algorithm::replace_all(config_key, "area", "qdtext");
        if (config_key.compare(0, 18, "qdtext::timezone::") == 0)
          area_timezones.insert(std::make_pair(config_key, config_value));
        else
          area_config_items.emplace_back(std::make_pair(config_key, config_value));
      }
    }
    // add default timezone if it doesn't exists
    if (area_timezones.find("qdtext::timezone::default") == area_timezones.end())
      area_timezones.insert(std::make_pair("qdtext::timezone::default", default_timezone));
  }
  catch (...)
  {
    if (exceptionDetails.empty())
      exceptionDetails =
          "Error reading product configuration file '" + configfile + "', please check the syntax!";

    throw Fmi::Exception::Trace(BCP, "Error processing product configuration file " + configfile)
        .addDetail(exceptionDetails);
  }
}

void ProductConfig::setDefaultConfig(const boost::shared_ptr<ProductConfig>& pDefaultConf)
{
  try
  {
    pDefaultConfig = pDefaultConf;

    for (const auto& item : pDefaultConfig->itsParameterMappings)
      if (itsParameterMappings.find(item.first) == itsParameterMappings.end())
        itsParameterMappings[item.first] = item.second;

    if (postgis_identifiers.empty())
    {
      if (!pDefaultConfig || pDefaultConfig->numberOfPostGISIdentifiers() == 0)
      {
        // not fatal
      }
      else
      {
        postgis_identifiers = pDefaultConfig->postgis_identifiers;
        itsDefaultPostGISIdentifierKey = pDefaultConfig->itsDefaultPostGISIdentifierKey;
      }
    }

    if (forecast_data_config_items.empty())
    {
      if (!pDefaultConfig || pDefaultConfig->numberOfForecastDataConfigs() == 0)
        throw Fmi::Exception(BCP, "forecast_data-section missing, cannot continue!");

      forecast_data_config_items = pDefaultConfig->forecast_data_config_items;
    }

    if (masks.empty())
    {
      if (!pDefaultConfig || pDefaultConfig->numberOfMasks() == 0)
        throw Fmi::Exception(BCP, "mask-section missing, cannot continue!");

      masks = pDefaultConfig->masks;
    }

    if (pDefaultConfig)
    {
      // MySQL dictionary
      if (itsMySQLDictionaryHost.empty())
        itsMySQLDictionaryHost = pDefaultConfig->itsMySQLDictionaryHost;

      if (itsMySQLDictionaryDatabase.empty())
        itsMySQLDictionaryDatabase = pDefaultConfig->itsMySQLDictionaryDatabase;

      if (itsMySQLDictionaryUsername.empty())
        itsMySQLDictionaryUsername = pDefaultConfig->itsMySQLDictionaryUsername;

      if (itsMySQLDictionaryPassword.empty())
        itsMySQLDictionaryPassword = pDefaultConfig->itsMySQLDictionaryPassword;

      if (unit_format_config_items.empty())
        unit_format_config_items = pDefaultConfig->unit_format_config_items;

      if (forecast_data_config_items.empty())
        forecast_data_config_items = pDefaultConfig->forecast_data_config_items;

      if (area_config_items.empty())
        area_config_items = pDefaultConfig->area_config_items;

      // area timezones
      for (const auto& timezone_item : pDefaultConfig->area_timezones)
      {
        if (area_timezones.find(timezone_item.first) == area_timezones.end())
          area_timezones.insert(timezone_item);
      }

      if (itsLanguage.empty())
        itsLanguage = pDefaultConfig->itsLanguage;

      if (itsFormatter.empty())
        itsFormatter = pDefaultConfig->itsFormatter;

      if (itsLocale.empty())
        itsLocale = pDefaultConfig->itsLocale;

      if (itsTimeFormat.empty())
        itsTimeFormat = pDefaultConfig->itsTimeFormat;

      if (itsDictionary.empty())
        itsDictionary = pDefaultConfig->itsDictionary;

      if (itsFileDictionaries.empty())
        itsFileDictionaries = pDefaultConfig->itsFileDictionaries;

      // Use hard-coded default values
      if (itsLanguage.empty())
        itsLanguage = default_language;
      if (itsFormatter.empty())
        itsFormatter = default_formatter;
      if (itsLocale.empty())
        itsLocale = default_locale;
      if (itsTimeFormat.empty())
        itsTimeFormat = default_timeformat;
      if (itsDictionary.empty())
        itsDictionary = default_dictionary;
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::string& ProductConfig::getAreaTimeZone(const std::string& area) const
{
  try
  {
    if (area_timezones.find("qdtext::timezone::" + area) == area_timezones.end())
    {
      return area_timezones.at("qdtext::timezone::default");
    }

    return area_timezones.at("qdtext::timezone::" + area);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

Engine::Gis::PostGISIdentifierVector ProductConfig::getPostGISIdentifiers() const
{
  Engine::Gis::PostGISIdentifierVector ret;
  for (const auto& item : postgis_identifiers)
  {
    ret.push_back(item.second);
  }

  return ret;
}

const Engine::Gis::postgis_identifier& ProductConfig::getDefaultPostGISIdentifier() const
{
  try
  {
    return postgis_identifiers.at(itsDefaultPostGISIdentifierKey);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getForecastDataConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= forecast_data_config_items.size())
      throw Fmi::Exception(
          BCP,
          "ProductConfig::getForecastDataConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return forecast_data_config_items.at(index);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getUnitFormatConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= unit_format_config_items.size())
      throw Fmi::Exception(
          BCP,
          "ProductConfig::getUnitFormatDataConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return unit_format_config_items.at(index);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getOutputDocumentConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= output_document_config_items.size())
      throw Fmi::Exception(
          BCP,
          "ProductConfig::getOutputDocumentConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return output_document_config_items.at(index);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getAreaConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= area_config_items.size())
      throw Fmi::Exception(
          BCP,
          "ProductConfig::getAreaConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return area_config_items.at(index);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getMask(const unsigned int& index) const
{
  try
  {
    if (index >= masks.size())
      throw Fmi::Exception(BCP,
                           "ProductConfig::getMask(index)-function invalid index parameter: " +
                               Fmi::to_string(index));

    return masks.at(index);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getFireWarningAreaCode(
    const unsigned int& index) const
{
  try
  {
    if (index >= forestfirewarning_areacodes.size())
      throw Fmi::Exception(
          BCP,
          "ProductConfig::getFireWarningAreaCode(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return forestfirewarning_areacodes.at(index);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Checks if configuration has been modified within given interval (seconds)
bool ProductConfig::isModified(size_t interval) const
{
  TextGenPosixTime timestamp;

  return (timestamp.EpochTime() - itsLastModifiedTime <= interval);
}

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
