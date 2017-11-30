// ======================================================================
/*!
 * \brief Implementation of Config
 */
// ======================================================================

#include "Config.h"
#include <spine/Exception.h>
#include <stdexcept>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>

using namespace std;
using namespace boost;

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
string parse_config_key(const char* str1 = 0, const char* str2 = 0, const char* str3 = 0)
{
  string string1(str1 ? str1 : "");
  string string2(str2 ? str2 : "");
  string string3(str3 ? str3 : "");

  string retval(string1 + string2 + string3);

  return retval;
}

void parseConfigurationItem(const libconfig::Config& itsConfig,
                            const std::string& key,
                            const vector<string>& allowed_sections,
                            config_item_vector& config_item_container)
{
  try
  {
    if (itsConfig.exists(key))
    {
      if (itsConfig.lookup(key).isGroup())
      {
        bool processSection(false);
        for (unsigned int i = 0; i < allowed_sections.size(); i++)
        {
          string allowed_key(allowed_sections[i]);
          string section_key(key);
          if (allowed_key.size() > section_key.size())
            allowed_key = allowed_key.substr(0, section_key.size());
          else if (allowed_key.size() < section_key.size())
            section_key = section_key.substr(0, allowed_key.size());

          if (allowed_key.compare(section_key) == 0 || allowed_key.compare("*") == 0)
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
        stringstream ss;
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
            string stringValue = itsConfig.lookup(key);
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

        config_item_container.push_back(make_pair(key, ss.str()));
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void handleIncludedSections(const libconfig::Config& itsConfig,
                            config_item_vector& config_item_container)
{
  try
  {
    config_item_vector all_output_document_config_items;
    vector<string> allowed_sections;
    allowed_sections.push_back("*");
    parseConfigurationItem(
        itsConfig, "output_document", allowed_sections, all_output_document_config_items);

    size_t numberOfConfigs = config_item_container.size();
    // these configurations are included by another and thus not use directly
    config_item_vector included_config_items;

    bool reProcess(false);
    for (unsigned int i = 0; i < numberOfConfigs; i++)
    {
      string config_key_original(config_item_container[i].first);
      string config_value_original(config_item_container[i].second);

      if (config_value_original.size() > 4 &&
          config_value_original.substr(0, 4).compare("use ") == 0)
      {
        string section_to_include(config_value_original.substr(4));
        // cout << "section_to_include: " << section_to_include << endl;

        for (unsigned int k = 0; k < all_output_document_config_items.size(); k++)
        {
          string item_key(all_output_document_config_items[k].first);
          if (item_key.size() >= section_to_include.size() &&
              item_key.substr(0, section_to_include.size()).compare(section_to_include) == 0)
          {
            string config_key_first_part(
                config_key_original.substr(0, config_key_original.find_last_of(".")));
            string config_key_second_part(item_key.substr(section_to_include.size()));
            string parsed_config_key(config_key_first_part + config_key_second_part);
            string config_value(all_output_document_config_items[k].second);
            if (config_value.size() > 4 && config_value.substr(0, 4).compare("use ") == 0)
              reProcess = true;

            included_config_items.push_back(make_pair(parsed_config_key, config_value));
          }
        }
      }
    }

    if (reProcess)
      handleIncludedSections(itsConfig, included_config_items);

    if (included_config_items.size() > 0)
      config_item_container.insert(config_item_container.begin(),
                                   included_config_items.begin(),
                                   included_config_items.end());

    sort(config_item_container.begin(), config_item_container.end());
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// read all string type parameters
void parseTypeStringConfiguationItem(const libconfig::Config& itsConfig,
                                     const std::string& key,
                                     config_item_vector& config_item_container)
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
              if (value.size() > 4 && value.substr(0, 4).compare("use ") == 0)
              {
                std::string included_value = itsConfig.lookup(value.substr(4));
                value = included_value;
              }
              config_item_container.push_back(make_pair(name, value));
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
          config_item_container.push_back(make_pair(name, value));
        }
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace

Config::Config(const string& configfile)
    : itsDefaultUrl(default_url), itsForecastTextCacheSize(DEFAULT_FORECAST_TEXT_CACHE_SIZE)
{
  try
  {
    libconfig::Config lconf;
    lconf.readFile(configfile.c_str());

    // For relative path lookups
    boost::filesystem::path config_path(configfile);

    config_item_vector config_items;
    vector<string> allowed_sections;
    allowed_sections.push_back("*");

    lconf.lookupValue("forecast_text_cache_size", itsForecastTextCacheSize);
    lconf.lookupValue("url", itsDefaultUrl);

    parseConfigurationItem(lconf, "product_config", allowed_sections, config_items);

    for (size_t i = 0; i < config_items.size(); i++)
    {
      string config_key(config_items[i].first);
      string config_name = config_key.substr(config_key.find(".") + 1);
      string config_value(config_items[i].second);

      // Make relative paths absolute with respect to the configuration file
      if (!config_value.empty() && config_value[0] != '/')
        config_value = config_path.parent_path().string() + "/" + config_value;

      itsProductConfigs.insert(make_pair(
          config_name, boost::shared_ptr<ProductConfig>(new ProductConfig(config_value))));
    }

    // check that certain parameters has been defined either in default configuration file or
    // product-specific configuration file
    BOOST_FOREACH (const product_config_item& pci, itsProductConfigs)
    {
      if (pci.first == default_textgen_config_name)
        continue;

      boost::shared_ptr<ProductConfig> pProductConfig = pci.second;

      if (itsProductConfigs.find(default_textgen_config_name) != itsProductConfigs.end())
        pProductConfig->setDefaultConfig(itsProductConfigs.at(default_textgen_config_name));

      // last resort: use hard-coded default values
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
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

const ProductConfig& Config::getProductConfig(const std::string& config_name) const
{
  try
  {
    if (itsProductConfigs.find(config_name) == itsProductConfigs.end())
      throw SmartMet::Spine::Exception(BCP, config_name + " configuration not found!");

    return *(itsProductConfigs.at(config_name));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

vector<string> Config::getProductNames() const
{
  try
  {
    vector<string> retval;

    BOOST_FOREACH (const product_config_item& pci, itsProductConfigs)
    {
      retval.push_back(pci.first);
    }

    return retval;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

ProductConfig::ProductConfig(const string& configfile)
    : itsLanguage(""),
      itsFormatter(""),
      itsLocale(""),
      itsTimeFormat(""),
      itsDictionary(""),
      itsForestFireWarningDirectory(""),
      itsFrostSeason(DEFAULT_FROSTSEASON)
{
  try
  {
    if (configfile.empty())
      return;

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
      postgis_identifiers.insert(make_pair(postgis_identifier_key, postgis_default_identifier));
    }

    if (itsConfig.exists("postgis.config_items"))
    {
      libconfig::Setting& config_items = itsConfig.lookup("postgis.config_items");

      if (!config_items.isArray())
      {
        throw SmartMet::Spine::Exception(
            BCP,
            "postgis.config_items not an array in textgenplugin configuration file line " +
                boost::lexical_cast<string>(config_items.getSourceLine()));
      }

      for (int i = 0; i < config_items.getLength(); ++i)
      {
        if (!itsConfig.exists(parse_config_key("postgis.", config_items[i])))
          throw SmartMet::Spine::Exception(BCP,
                                           parse_config_key("postgis.", config_items[i]) +
                                               " -section does not exists in configuration file");

        Engine::Gis::postgis_identifier postgis_id(
            postgis_identifiers[itsDefaultPostGISIdentifierKey]);

        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".host").c_str(),
                              postgis_id.host);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".port").c_str(),
                              postgis_id.port);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".database").c_str(),
                              postgis_id.database);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".username").c_str(),
                              postgis_id.username);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".password").c_str(),
                              postgis_id.password);
        itsConfig.lookupValue(
            parse_config_key("postgis.", config_items[i], ".client_encoding").c_str(),
            postgis_id.encoding);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".schema").c_str(),
                              postgis_id.schema);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".table").c_str(),
                              postgis_id.table);
        itsConfig.lookupValue(parse_config_key("postgis.", config_items[i], ".field").c_str(),
                              postgis_id.field);

        std::string key(postgis_id.key());
        if (postgis_identifiers.find(key) == postgis_identifiers.end())
          postgis_identifiers.insert(make_pair(postgis_id.key(), postgis_id));
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
            masks.push_back(make_pair(maskName, maskValue));
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
          stringstream ss;
          ss << value;
          forestfirewarning_areacodes.push_back(
              make_pair("qdtext::forestfirewarning::areacodes::" + name, ss.str()));
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
      config_item_vector output_document_config_item_container;
      // sections
      libconfig::Setting& sections = itsConfig.lookup("output_document.sections");

      if (!sections.isArray())
      {
        throw SmartMet::Spine::Exception(
            BCP,
            "output_document.sections not an array in textgenplugin configuration file line " +
                boost::lexical_cast<string>(sections.getSourceLine()));
        ;
      }

      string sections_parameter_value;
      vector<string> allowed_sections;
      // allowed_sections.clear();
      for (int i = 0; i < sections.getLength(); ++i)
      {
        string section_name = sections[i];

        if (i > 0)
          sections_parameter_value += ",";
        sections_parameter_value += section_name;
        allowed_sections.push_back("output_document." + section_name + ".period");
        allowed_sections.push_back("output_document." + section_name + ".subperiod");
        allowed_sections.push_back("output_document." + section_name + ".header");

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
              string story_name = content;
              if (story_name.compare("none") == 0)
                allowed_sections.push_back("output_document." + section_name);
              else
              {
                allowed_sections.push_back("output_document." + section_name + ".story." +
                                           story_name);
                allowed_sections.push_back("output_document." + section_name + ".day1.story." +
                                           story_name);
                allowed_sections.push_back("output_document." + section_name + ".day2.story." +
                                           story_name);
              }
            }
          }
          else
          {
            string content_parameter_value;
            for (int k = 0; k < content.getLength(); ++k)
            {
              string story_name = content[k];
              if (k > 0)
                content_parameter_value += ",";
              content_parameter_value += story_name;

              //						  cout << "CONTENT: " << story_name
              //<< endl;

              allowed_sections.push_back("output_document." + section_name + ".story." +
                                         story_name);
            }
            output_document_config_item_container.push_back(
                make_pair("output_document." + section_name + ".content", content_parameter_value));
          }
        }
        parseConfigurationItem(itsConfig,
                               "output_document." + section_name,
                               allowed_sections,
                               output_document_config_item_container);
      }
      output_document_config_item_container.push_back(
          make_pair("output_document.sections", sections_parameter_value));

      handleIncludedSections(itsConfig, output_document_config_item_container);

      BOOST_FOREACH (config_item& ci, output_document_config_item_container)
      {
        string config_key(ci.first);
        string config_value(ci.second);

        if (config_value.size() > 4 && config_value.substr(0, 4).compare("use ") == 0)
          continue;

        replace_all(config_key, ".", "::");
        replace_all(config_key, "output_document", "textgen");
        output_document_config_items.push_back(make_pair(config_key, config_value));
      }
    }

    // area configuration
    if (itsConfig.exists("area"))
    {
      config_item_vector area_config_item_container;
      vector<string> allowed_sections;
      allowed_sections.push_back("*");
      // sections
      parseConfigurationItem(itsConfig, "area", allowed_sections, area_config_item_container);

      BOOST_FOREACH (config_item& ci, area_config_item_container)
      {
        string config_key(ci.first);
        string config_value(ci.second);

        if (config_value.size() > 4 && config_value.substr(0, 4).compare("use ") == 0)
          continue;

        replace_all(config_key, ".", "::");
        replace_all(config_key, "area", "qdtext");
        if (config_key.compare(0, 18, "qdtext::timezone::") == 0)
          area_timezones.insert(make_pair(config_key, config_value));
        else
          area_config_items.push_back(make_pair(config_key, config_value));
      }
    }
    // add default timezone if it doesn't exists
    if (area_timezones.find("qdtext::timezone::default") == area_timezones.end())
      area_timezones.insert(make_pair("qdtext::timezone::default", default_timezone));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void ProductConfig::setDefaultConfig(const boost::shared_ptr<ProductConfig> pDefaultConf)
{
  try
  {
    pDefaultConfig = pDefaultConf;

    if (postgis_identifiers.size() == 0)
    {
      if (!pDefaultConfig || pDefaultConfig->numberOfPostGISIdentifiers() == 0)
        throw SmartMet::Spine::Exception(
            BCP, "PostGIS configuration error: postgis.default-section missing, cannot continue!");

      postgis_identifiers = pDefaultConfig->postgis_identifiers;
      itsDefaultPostGISIdentifierKey = pDefaultConfig->itsDefaultPostGISIdentifierKey;
    }

    if (forecast_data_config_items.size() == 0)
    {
      if (!pDefaultConfig || pDefaultConfig->numberOfForecastDataConfigs() == 0)
        throw SmartMet::Spine::Exception(BCP, "forecast_data-section missing, cannot continue!");

      forecast_data_config_items = pDefaultConfig->forecast_data_config_items;
    }

    if (masks.size() == 0)
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

      if (unit_format_config_items.size() == 0)
        unit_format_config_items = pDefaultConfig->unit_format_config_items;

      if (forecast_data_config_items.size() == 0)
        forecast_data_config_items = pDefaultConfig->forecast_data_config_items;

      if (area_config_items.size() == 0)
        area_config_items = pDefaultConfig->area_config_items;

      // area timezones
      BOOST_FOREACH (const config_item& timezone_item, pDefaultConfig->area_timezones)
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

Engine::Gis::PostGISIdentifierVector ProductConfig::getPostGISIdentifiers() const
{
  Engine::Gis::PostGISIdentifierVector ret;
  for (auto item : postgis_identifiers)
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

const std::pair<string, string>& ProductConfig::getForecastDataConfig(
    const unsigned int& index) const
{
  try
  {
    if (index >= forecast_data_config_items.size())
      throw SmartMet::Spine::Exception(
          BCP,
          "ProductConfig::getForecastDataConfig(index)-function invalid index parameter: " +
              boost::lexical_cast<string>(index));

    return forecast_data_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
              boost::lexical_cast<string>(index));

    return unit_format_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
              boost::lexical_cast<string>(index));

    return output_document_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
              boost::lexical_cast<string>(index));

    return area_config_items.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
              boost::lexical_cast<string>(index));

    return masks.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
              boost::lexical_cast<string>(index));

    return forestfirewarning_areacodes.at(index);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
