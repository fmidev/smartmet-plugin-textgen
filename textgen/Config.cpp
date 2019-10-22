// ======================================================================
/*!
 * \brief Implementation of Config
 */
// ======================================================================

#include "Config.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <calculator/TextGenPosixTime.h>
#include <macgyver/AnsiEscapeCodes.h>
#include <macgyver/StringConversion.h>
#include <spine/Exception.h>
#include <iostream>
#include <stdexcept>

static const char* default_url = "/textgen";
static const char* default_language = "fi";
static const char* default_locale = "fi_FI.UTF-8";
static const char* default_formatter = "html";
static const char* default_timeformat = "iso";
static const char* default_dictionary = "multimysql";
static const char* default_timezone = "Europe/Helsinki";
static const char* default_postgis_client_encoding = "latin1";
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
            throw SmartMet::Spine::Exception(BCP,
                                             "TextGen: Invalid setting type for '" + key + "'");
        };

        config_item_container.push_back(std::make_pair(key, ss.str()));
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

Config::Config(const std::string& configfile)
    : itsDefaultUrl(default_url),
      itsForecastTextCacheSize(DEFAULT_FORECAST_TEXT_CACHE_SIZE),
      itsMainConfigFile(configfile)
{
  try
  {
    libconfig::Config lconf;
    lconf.readFile(configfile.c_str());

    lconf.lookupValue("forecast_text_cache_size", itsForecastTextCacheSize);
    lconf.lookupValue("url", itsDefaultUrl);

    // Set monitoring directories
    ConfigItemVector configItems = readMainConfig();
    std::set<std::string> emptyset;
    updateProductConfigs(configItems, emptyset, emptyset, emptyset);

    boost::regex pattern("^[\\w,\\s-]+\\.[A-Za-z]+$");
    for (auto dir : getDirectoriesToMonitor(configItems))
    {
      itsMonitor.watch(dir,
                       pattern,
                       boost::bind(&Config::update, this, _1, _2, _3, _4),
                       boost::bind(&Config::error, this, _1, _2, _3, _4),
                       10,
                       Fmi::DirectoryMonitor::ALL);
    }
    itsMonitorThread = boost::thread(boost::bind(&Fmi::DirectoryMonitor::run, &itsMonitor));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

void Config::setDefaultConfigValues(ProductConfigMap& productConfigs)
{
  // Check that certain parameters has been defined either in default configuration file or
  // product-specific configuration file
  for (const auto& pci : productConfigs)
  {
    if (pci.first == default_textgen_config_name)
      continue;

    boost::shared_ptr<ProductConfig> pProductConfig = pci.second;

    if (productConfigs.find(default_textgen_config_name) != productConfigs.end())
      pProductConfig->setDefaultConfig(productConfigs.at(default_textgen_config_name));

    // Use hard-coded default values
    if (pProductConfig->itsLanguage.empty())
      pProductConfig->itsLanguage = default_language;
    if (pProductConfig->itsFormatter.empty())
      pProductConfig->itsFormatter = default_formatter;
    if (pProductConfig->itsLocale.empty())
      pProductConfig->itsLocale = default_locale;
    if (pProductConfig->itsTimeFormat.empty())
      pProductConfig->itsTimeFormat = default_timeformat;
    if (pProductConfig->itsDictionary.empty())
      pProductConfig->itsDictionary = default_dictionary;
    if (pProductConfig->itsDictionary.empty())
      pProductConfig->itsDictionary = default_dictionary;
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

    throw SmartMet::Spine::Exception(BCP, "Error reading configuration file!").addDetail(details);
  }
}
void Config::updateProductConfigs(const ConfigItemVector& configItems,
                                  const std::set<std::string>& deletedFiles,
                                  const std::set<std::string>& modifiedFiles,
                                  const std::set<std::string>& newFiles)
{
  std::string config_value;
  itsProductFiles.clear();
  std::set<std::string> erroneousFiles;

  try
  {
    std::unique_ptr<ProductConfigMap> newProductConfigs(new ProductConfigMap());
    boost::filesystem::path config_path(itsMainConfigFile);
    for (const auto& item : configItems)
    {
      std::string config_key(item.first);
      std::string config_name = config_key.substr(config_key.find('.') + 1);
      config_value = item.second;

      // Make relative paths absolute
      if (!config_value.empty() && config_value[0] != '/')
        config_value = config_path.parent_path().string() + "/" + config_value;
      try
      {
        boost::shared_ptr<ProductConfig> productConfig(new ProductConfig(config_value));
        newProductConfigs->insert(std::make_pair(config_name, productConfig));
        itsProductFiles.insert(config_value);
        if (modifiedFiles.find(config_value) != modifiedFiles.end())
        {
          TextGenPosixTime timestamp;
          productConfig->itsLastModifiedTime = timestamp.EpochTime();
        }
      }
      catch (const SmartMet::Spine::Exception& e)
      {
        // If non-fatal error occurred, report it and continue processing the next file
        if (e.getDetailCount() > 0)
        {
          std::cout << ANSI_FG_RED << e.getDetailByIndex(0) << ANSI_FG_DEFAULT << std::endl;
          erroneousFiles.insert(config_value);
        }
      }
      catch (...)
      {
        throw SmartMet::Spine::Exception(
            BCP, "Error reading product configuration file '" + config_value + "'");
        erroneousFiles.insert(config_value);
      }
    }

    setDefaultConfigValues(*newProductConfigs);
    itsProductConfigs = std::move(newProductConfigs);

    if (deletedFiles.empty() && modifiedFiles.empty() && newFiles.empty())
      return;

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
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(
        BCP, "Error reading product configuration file '" + config_value + "'");
  }
}

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
  catch (const SmartMet::Spine::Exception& e)
  {
    std::string details = e.getDetailByIndex(0);
    std::cout << ANSI_FG_RED
              << (std::string(e.getDetailByIndex(0)) + " Textgen plugin is now inactive!")
              << ANSI_FG_DEFAULT << std::endl;
    itsProductConfigs->clear();
    return;
  }
  updateProductConfigs(configItems, deletedFiles, modifiedFiles, newFiles);

}  // namespace Textgen

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
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const ProductConfig& Config::getProductConfig(const std::string& config_name) const
{
  try
  {
    if (itsProductConfigs->find(config_name) == itsProductConfigs->end())
      throw SmartMet::Spine::Exception(BCP, config_name + " configuration not found!");

    return *(itsProductConfigs->at(config_name));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

std::vector<std::string> Config::getProductNames() const
{
  try
  {
    std::vector<std::string> retval;

    for (const auto& pci : *itsProductConfigs)
    {
      retval.push_back(pci.first);
    }

    return retval;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

ProductConfig::ProductConfig(const std::string& configfile)
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
      throw SmartMet::Spine::Exception(BCP, configfile + " file does not exist!");
    }

    itsConfig.readFile(configfile.c_str());

    // miscellaneous parameters
    itsConfig.lookupValue("misc.frostseason", itsFrostSeason);
    itsConfig.lookupValue("misc.timeformat", itsTimeFormat);
    itsConfig.lookupValue("misc.language", itsLanguage);
    itsConfig.lookupValue("misc.locale", itsLocale);
    itsConfig.lookupValue("misc.dictionary", itsDictionary);
    itsConfig.lookupValue("forestfirewarning.directory", itsForestFireWarningDirectory);
    itsConfig.lookupValue("misc.formatter", itsFormatter);

    // mysql-dictionary
    itsConfig.lookupValue("mysql_dictionary.host", itsMySQLDictionaryHost);
    itsConfig.lookupValue("mysql_dictionary.database", itsMySQLDictionaryDatabase);
    itsConfig.lookupValue("mysql_dictionary.username", itsMySQLDictionaryUsername);
    itsConfig.lookupValue("mysql_dictionary.password", itsMySQLDictionaryPassword);

    // PostGIS
    if (itsConfig.exists("postgis.default"))
    {
      Engine::Gis::postgis_identifier postgis_default_identifier;
      postgis_default_identifier.encoding = default_postgis_client_encoding;
      itsConfig.lookupValue("postgis.default.host", postgis_default_identifier.host);
      itsConfig.lookupValue("postgis.default.port", postgis_default_identifier.port);
      itsConfig.lookupValue("postgis.default.database", postgis_default_identifier.database);
      itsConfig.lookupValue("postgis.default.username", postgis_default_identifier.username);
      itsConfig.lookupValue("postgis.default.password", postgis_default_identifier.password);
      itsConfig.lookupValue("postgis.default.client_encoding", postgis_default_identifier.encoding);
      itsConfig.lookupValue("postgis.default.schema", postgis_default_identifier.schema);
      itsConfig.lookupValue("postgis.default.table", postgis_default_identifier.table);
      itsConfig.lookupValue("postgis.default.field", postgis_default_identifier.field);
      std::string postgis_identifier_key(postgis_default_identifier.key());
      itsDefaultPostGISIdentifierKey = postgis_identifier_key;
      postgis_identifiers.insert(
          std::make_pair(postgis_identifier_key, postgis_default_identifier));
    }

    if (itsConfig.exists("postgis.config_items"))
    {
      libconfig::Setting& configItems = itsConfig.lookup("postgis.config_items");

      if (!configItems.isArray())
      {
        throw SmartMet::Spine::Exception(
            BCP,
            "postgis.config_items not an array in textgenplugin configuration file line " +
                Fmi::to_string(configItems.getSourceLine()));
      }

      for (int i = 0; i < configItems.getLength(); ++i)
      {
        if (!itsConfig.exists(parse_config_key("postgis.", configItems[i])))
          throw SmartMet::Spine::Exception(BCP,
                                           parse_config_key("postgis.", configItems[i]) +
                                               " -section does not exists in configuration file");

        Engine::Gis::postgis_identifier postgis_id(
            postgis_identifiers[itsDefaultPostGISIdentifierKey]);

        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".host").c_str(),
                              postgis_id.host);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".port").c_str(),
                              postgis_id.port);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".database").c_str(),
                              postgis_id.database);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".username").c_str(),
                              postgis_id.username);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".password").c_str(),
                              postgis_id.password);
        itsConfig.lookupValue(
            parse_config_key("postgis.", configItems[i], ".client_encoding").c_str(),
            postgis_id.encoding);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".schema").c_str(),
                              postgis_id.schema);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".table").c_str(),
                              postgis_id.table);
        itsConfig.lookupValue(parse_config_key("postgis.", configItems[i], ".field").c_str(),
                              postgis_id.field);

        std::string key(postgis_id.key());
        if (postgis_identifiers.find(key) == postgis_identifiers.end())
          postgis_identifiers.insert(std::make_pair(postgis_id.key(), postgis_id));
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
        throw SmartMet::Spine::Exception(
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

    throw SmartMet::Spine::Exception::Trace(
        BCP, "Error processing product configuration file " + configfile)
        .addDetail(exceptionDetails);
  }
}

void ProductConfig::setDefaultConfig(const boost::shared_ptr<ProductConfig>& pDefaultConf)
{
  try
  {
    pDefaultConfig = pDefaultConf;

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
        throw SmartMet::Spine::Exception(BCP, "forecast_data-section missing, cannot continue!");

      forecast_data_config_items = pDefaultConfig->forecast_data_config_items;
    }

    if (masks.empty())
    {
      if (!pDefaultConfig || pDefaultConfig->numberOfMasks() == 0)
        throw SmartMet::Spine::Exception(BCP, "mask-section missing, cannot continue!");

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

      if (itsDictionary.empty())
        itsDictionary = pDefaultConfig->itsDictionary;
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

Engine::Gis::PostGISIdentifierVector ProductConfig::getPostGISIdentifiers() const
{
  Engine::Gis::PostGISIdentifierVector ret;
  for (const auto& item : postgis_identifiers)
    ret.push_back(item.second);

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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getForecastDataConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= forecast_data_config_items.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getForecastDataConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return forecast_data_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getUnitFormatConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= unit_format_config_items.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getUnitFormatDataConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return unit_format_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getOutputDocumentConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= output_document_config_items.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getOutputDocumentConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return output_document_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getAreaConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= area_config_items.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getAreaConfig(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return area_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getMask(const unsigned int& index) const
{
  try
  {
    if (index >= masks.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getMask(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return masks.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

const std::pair<std::string, std::string>& ProductConfig::getFireWarningAreaCode(
    const unsigned int& index) const
{
  try
  {
    if (index >= forestfirewarning_areacodes.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getFireWarningAreaCode(index)-function invalid index parameter: " +
              Fmi::to_string(index));

    return forestfirewarning_areacodes.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
