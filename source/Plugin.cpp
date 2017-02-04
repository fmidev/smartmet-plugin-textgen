#include "Plugin.h"
#include "MySQLDictionariesPlusGeonames.h"

#include <spine/Convenience.h>
#include <spine/ValueFormatter.h>
#include <spine/Table.h>
#include <spine/TableFormatterFactory.h>
#include <spine/SmartMet.h>
#include <spine/Exception.h>
#include <spine/Location.h>
#include <engines/geonames/Engine.h>

#include <calculator/TimeTools.h>
#include <calculator/TextGenPosixTime.h>
#include <calculator/Settings.h>

#include <textgen/DictionaryFactory.h>
#include <textgen/Document.h>
#include <textgen/TextFormatter.h>
#include <textgen/TextFormatterFactory.h>
#include <textgen/TextGenerator.h>

#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiFileSystem.h>
#include <newbase/NFmiStringTools.h>

#include <macgyver/TimeFormatter.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tokenizer.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace NFmiStringTools;

using namespace TextGen;

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
#define LAND_MASK_NAME "land"
#define COAST_MASK_NAME "coast"
#define PRODUCT_PARAM "product"
#define DEFAULT_PRODUCT_NAME "default"
#define LOCALE_PARAM "locale"
#define AREA_PARAM "area"
#define FORECASTTIME_PARAM "forecasttime"
#define LANGUAGE_PARAM "language"
#define FORMATTER_PARAM "formatter"
#define LONLAT_PARAM "lonlat"
#define GEOID_PARAM "geoid"
#define POSTGIS_PARAM "postgis"
#define POSTGIS_HOST_PARAM "host"
#define POSTGIS_PORT_PARAM "port"
#define POSTGIS_DBNAME_PARAM "dbname"
#define POSTGIS_USERNAME_PARAM "username"
#define POSTGIS_PASSWORD_PARAM "password"
#define POSTGIS_SCHEMA_PARAM "schema"
#define POSTGIS_TABLE_PARAM "table"
#define POSTGIS_FIELD_PARAM "field"
#define POSTGIS_CLIENT_ENCODING_PARAM "client_encoding"

namespace
{
void normalize_string(std::string& str)
{
  // convert to lower case and skandinavian characters
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  boost::algorithm::replace_all(str, "Ä", "a");
  boost::algorithm::replace_all(str, "ä", "a");
  boost::algorithm::replace_all(str, "Å", "a");
  boost::algorithm::replace_all(str, "å", "a");
  boost::algorithm::replace_all(str, "Ö", "o");
  boost::algorithm::replace_all(str, "ö", "o");
}

std::string mmap_string(const SmartMet::Spine::HTTP::ParamMap& mmap,
                        const std::string& key,
                        const std::string& default_value = "")
{
  try
  {
    if (mmap.find(key) == mmap.end())
      return default_value;

    return mmap.find(key)->second;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

bool parse_forecasttime_parameter(const std::string& forecasttime_string,
                                  TextGenPosixTime& forecasttime,
                                  std::string& errorMessage)
{
  try
  {
    if (!forecasttime_string.empty())
    {
      if (forecasttime_string.size() < 12)
      {
        errorMessage = "forecasttime parameter format is YYYYMMDDHHMM[SS]";
        return false;
      }
      short year = boost::lexical_cast<short>(forecasttime_string.substr(0, 4).c_str());
      short month = boost::lexical_cast<short>(forecasttime_string.substr(4, 2).c_str());
      short day = boost::lexical_cast<short>(forecasttime_string.substr(6, 2).c_str());
      short hour = boost::lexical_cast<short>(forecasttime_string.substr(8, 2).c_str());
      short min = boost::lexical_cast<short>(forecasttime_string.substr(10, 2).c_str());
      short sec = 0;
      if (forecasttime_string.size() > 12)
        sec = boost::lexical_cast<short>(forecasttime_string.substr(12, 2).c_str());

      forecasttime = TextGenPosixTime(year, month, day, hour, min, sec);
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

bool parse_lonlat_parameter(const std::string& lonlat_string,
                            const boost::shared_ptr<TextGen::Dictionary>& theDictionary,
                            vector<WeatherArea>& weatherAreaVector,
                            std::string& errorMessage)
{
  try
  {
    if (!lonlat_string.empty())
    {
      // format: longitude,latitude[:radius km];longitude,latitude[:radius km]
      // for example 24.1514,61.4356:2.5,25.6547,66.1009
      vector<string> parts;
      boost::algorithm::split(parts, lonlat_string, boost::algorithm::is_any_of(","));
      if (parts.size() % 2 != 0)
      {
        errorMessage =
            "format of lonlat parameter is ff.ff,ff.ff[:ff.ff] (longitude latitude:radius)";
        return false;
      }

      for (unsigned int j = 0; j < parts.size(); j += 2)
      {
        string longitude_string(parts[j]);
        string latitude_string(parts[j + 1]);

        // 5 km by default
        float radius(5.0);
        unsigned long radius_index(latitude_string.find(':'));
        bool radius_defined(radius_index != std::string::npos);
        if (radius_defined)
        {
          radius = boost::lexical_cast<float>(latitude_string.substr(radius_index + 1).c_str());
        }

        unsigned long latitude_end_index(radius_defined ? radius_index : longitude_string.size());
        if (radius_defined)
          latitude_string = latitude_string.substr(0, latitude_end_index);
        float longitude(kFloatMissing);
        float latitude(kFloatMissing);
        try
        {
          longitude = boost::lexical_cast<float>(longitude_string);
          latitude = boost::lexical_cast<float>(latitude_string);
        }
        catch (...)
        {
          errorMessage = "Invalid lonlat-option: " + longitude_string + ", " + latitude_string;
          return false;
        }

        //				SmartMet::Spine::WriteLock lock(itsGeoNamesMutex);
        std::string geoname(theDictionary->geofind(longitude, latitude, radius));

        normalize_string(geoname);
        weatherAreaVector.push_back(
            TextGen::WeatherArea(NFmiPoint(longitude, latitude),
                                 geoname,
                                 (radius_defined && radius >= 5.0) ? radius : 0.0));
      }
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

bool parse_postgis_parameters(const SmartMet::Spine::HTTP::ParamMap& queryParameters,
                              const ProductConfig& config,
                              SmartMet::Spine::PostGISDataSource& postGISDataSource,
                              SmartMet::Spine::MutexType& thePostGISMutex,
                              std::string& errorMessage)
{
  try
  {
    if (queryParameters.find(POSTGIS_PARAM) == queryParameters.end() &&
        config.numberOfPostGISIdentifiers() == 0)
    {
      throw SmartMet::Spine::Exception(BCP, "PostGIS parameters not defined, cannot continue!");
    }

    pair<SmartMet::Spine::HTTP::ParamMap::const_iterator,
         SmartMet::Spine::HTTP::ParamMap::const_iterator> iter_range =
        queryParameters.equal_range(POSTGIS_PARAM);
    SmartMet::Spine::HTTP::ParamMap::const_iterator iter;

    for (iter = iter_range.first; iter != iter_range.second; ++iter)
    {
      std::string postgis_string = iter->second;

      // format: schema=schema1,port=port1,dbname=dbname1,table=table1
      if (postgis_string.find(",") != std::string::npos)
      {
        const SmartMet::Spine::postgis_identifier& default_pgis_id =
            config.getDefaultPostGISIdentifier();

        std::string pgis_host(default_pgis_id.postGISHost);
        std::string pgis_port(default_pgis_id.postGISPort);
        std::string pgis_dbname(default_pgis_id.postGISDatabase);
        std::string pgis_username(default_pgis_id.postGISUsername);
        std::string pgis_password(default_pgis_id.postGISPassword);
        std::string pgis_schema(default_pgis_id.postGISSchema);
        std::string pgis_table(default_pgis_id.postGISTable);
        std::string pgis_field(default_pgis_id.postGISField);
        std::string pgis_client_encoding(default_pgis_id.postGISClientEncoding);

        char_separator<char> sep(",");
        tokenizer<char_separator<char> > tokens(postgis_string, sep);
        BOOST_FOREACH (const string& t, tokens)
        {
          unsigned long assign_index = t.find("=");
          if (assign_index == std::string::npos)
          {
            errorMessage = "Erroneous PostGIS parameter: " + t;
            return false;
          }
          std::string key(t.substr(0, assign_index));
          std::string value(t.substr(assign_index + 1));

          if (key.compare(POSTGIS_HOST_PARAM) == 0)
            pgis_host = value;
          else if (key.compare(POSTGIS_PORT_PARAM) == 0)
            pgis_port = value;
          else if (key.compare(POSTGIS_DBNAME_PARAM) == 0)
            pgis_dbname = value;
          else if (key.compare(POSTGIS_SCHEMA_PARAM) == 0)
            pgis_schema = value;
          else if (key.compare(POSTGIS_TABLE_PARAM) == 0)
            pgis_table = value;
          else if (key.compare(POSTGIS_FIELD_PARAM) == 0)
            pgis_field = value;
          else if (key.compare(POSTGIS_USERNAME_PARAM) == 0)
            pgis_username = value;
          else if (key.compare(POSTGIS_PASSWORD_PARAM) == 0)
            pgis_password = value;
          else if (key.compare(POSTGIS_CLIENT_ENCODING_PARAM) == 0)
            pgis_client_encoding = value;
        }
#ifdef MYDEBUG
        std::cout << "Reading data from PostGIS: " << std::endl
                  << "host: " << pgis_host << std::endl
                  << "port: " << pgis_port << std::endl
                  << "dbname: " << pgis_dbname << std::endl
                  << "username: " << pgis_username << std::endl
                  << "password: " << pgis_password << std::endl
                  << "schema: " << pgis_schema << std::endl
                  << "encoding: " << pgis_client_encoding << std::endl
                  << "table: " << pgis_table << std::endl
                  << "field: " << pgis_field << std::endl
                  << std::endl;
#endif

        SmartMet::Spine::WriteLock lock(thePostGISMutex);
        std::string log_message;

        postGISDataSource.readData(pgis_host,
                                   pgis_port,
                                   pgis_dbname,
                                   pgis_username,
                                   pgis_password,
                                   pgis_schema,
                                   pgis_table,
                                   pgis_field,
                                   pgis_client_encoding,
                                   log_message);
      }
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

TextGen::WeatherArea make_area(const std::string& postGISName,
                               const SmartMet::Spine::PostGISDataSource& postGISDataSource)
{
  try
  {
    std::string areaName(postGISName);
    normalize_string(areaName);

    if (postGISDataSource.isPolygon(postGISName))
    {
      stringstream svg_string_stream(postGISDataSource.getSVGPath(postGISName));
      NFmiSvgPath svgPath;
      svgPath.Read(svg_string_stream);
      return WeatherArea(svgPath, areaName);
    }
    else  // if not polygon, it must be a point
    {
      std::pair<float, float> std_point(postGISDataSource.getPoint(postGISName));
      NFmiPoint point(std_point.first, std_point.second);
      return WeatherArea(point, areaName);
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

bool parse_area_parameter(const std::string& areaParameter,
                          const SmartMet::Spine::PostGISDataSource& postGISDataSource,
                          vector<WeatherArea>& weatherAreaVector,
                          SmartMet::Spine::MutexType& thePostGISMutex,
                          std::string& errorMessage)
{
  try
  {
    const vector<string> area_name_vector = NFmiStringTools::Split(areaParameter);

    for (unsigned int i = 0; i < area_name_vector.size(); i++)
    {
      SmartMet::Spine::ReadLock lock(thePostGISMutex);
      const std::string area_name(area_name_vector[i]);
      if (postGISDataSource.geoObjectExists(area_name))
      {
        weatherAreaVector.push_back(WeatherArea(make_area(area_name, postGISDataSource)));
      }
      else
      {
        errorMessage += "Area ";
        errorMessage += area_name;
        errorMessage += " not found in PostGIS database!";
        return false;
      }
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

bool parse_geoid_parameter(const std::string& geoidParameter,
                           const SmartMet::Spine::PostGISDataSource& postGISDataSource,
                           const SmartMet::Engine::Geonames::Engine& geoEngine,
                           const std::string& language,
                           vector<WeatherArea>& weatherAreaVector,
                           std::string& errorMessage)
{
  try
  {
    const vector<string> geoid_vector = NFmiStringTools::Split(geoidParameter);

    int geoid(0);
    for (unsigned int i = 0; i < geoid_vector.size(); i++)
    {
      try
      {
        geoid = boost::lexical_cast<int>(geoid_vector[i]);
      }
      catch (...)
      {
        errorMessage = (geoid_vector[i] + " is not valid geoid!");
        return false;
      }

      SmartMet::Spine::LocationPtr loc = geoEngine.idSearch(geoid, language);

      if (!loc)
      {
        errorMessage = (geoid_vector[i] + " not found in database!");
        return false;
      }

      std::string feature_code(loc->feature);
      std::string loc_name(loc->name);

      // if feature code is ADM1,ADM2,ADM3 and name is found in PostGIS database
      // fetch area definition form there
      if (feature_code.substr(0, 3) == "ADM" && postGISDataSource.geoObjectExists(loc_name))
      {
        weatherAreaVector.push_back(WeatherArea(make_area(loc_name, postGISDataSource)));
      }
      else
      {
        normalize_string(loc_name);
        // otherwise use the lonlat coordinates
        weatherAreaVector.push_back(
            TextGen::WeatherArea(NFmiPoint(loc->longitude, loc->latitude), loc_name, 0.0));
      }

#ifdef MYDEBUG
      std::cout << "name: " << loc->name << std::endl;
      std::cout << "lon,lat: " << loc->longitude << ", " << loc->latitude << std::endl;
      std::cout << "country: " << loc->country << std::endl;
      std::cout << "feature: " << loc->feature << std::endl;
      std::cout << "timezone: " << loc->timezone << std::endl;
      std::cout << "population: " << loc->population << std::endl;
      std::cout << "iso2: " << loc->iso2 << std::endl;
      std::cout << "id: " << loc->geoid << std::endl;
      std::cout << "elevation: " << loc->elevation << std::endl;
#endif
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void set_textgen_settings(const ProductConfig& config)
{
  try
  {
#ifdef MYDEBUG
    std::cout << "*** Test generator settings ***" << std::endl;
#endif
    // frostseason-parameter is used by old stories
    Settings::set("textgen::frostseason", (config.isFrostSeason() ? "true" : "false"));

    // MySQL database
    Settings::set("textgen::host", config.mySQLDictionaryHost());
    Settings::set("textgen::user", config.mySQLDictionaryUsername());
    Settings::set("textgen::passwd", config.mySQLDictionaryPassword());
    Settings::set("textgen::database", config.mySQLDictionaryDatabase());

#ifdef MYDEBUG
    std::cout << "MySQL dictionary: " << std::endl
              << "textgen::host=" << config.mySQLDictionaryHost() << std::endl
              << "textgen::user=" << config.mySQLDictionaryUsername() << std::endl
              << "textgen::passwd=" << config.mySQLDictionaryPassword() << std::endl
              << "textgen::database=" << config.mySQLDictionaryDatabase() << std::endl
              << "textgen::frostseason=" << (config.isFrostSeason() ? "true" : "false") << std::endl
              << std::endl;
#endif

    // forecasts and the querydata
    for (unsigned int i = 0; i < config.numberOfForecastDataConfigs(); i++)
    {
      const std::pair<string, string>& forecast_item = config.getForecastDataConfig(i);
      Settings::set("textgen::" + forecast_item.first, forecast_item.second);
#ifdef MYDEBUG
      if (i == 0)
        std::cout << "Querydata: " << std::endl;
      std::cout << "textgen::" + forecast_item.first << "=" << forecast_item.second << std::endl;
#endif
    }

    // unit formats
    for (unsigned int i = 0; i < config.numberOfUnitFormatConfigs(); i++)
    {
      const std::pair<string, string>& unit_format_item = config.getUnitFormatConfig(i);
      Settings::set("textgen::units::" + unit_format_item.first + "::format",
                    unit_format_item.second);
#ifdef MYDEBUG
      if (i == 0)
        std::cout << std::endl << "Units: " << std::endl;
      std::cout << "textgen::units::" + unit_format_item.first + "::format"
                << "=" << unit_format_item.second << std::endl;
#endif
    }

    // output document
    for (unsigned int i = 0; i < config.numberOfOutputDocumentConfigs(); i++)
    {
      const std::pair<string, string>& output_document_config_item =
          config.getOutputDocumentConfig(i);
      Settings::set(output_document_config_item.first, output_document_config_item.second);
#ifdef MYDEBUG
      if (i == 0)
        std::cout << std::endl << "Output document: " << std::endl;
      std::cout << output_document_config_item.first << "=" << output_document_config_item.second
                << std::endl;
#endif
    }

    // area parameters
    for (unsigned int i = 0; i < config.numberOfAreaConfigs(); i++)
    {
      const std::pair<string, string>& area_config_item = config.getAreaConfig(i);
      Settings::set(area_config_item.first, area_config_item.second);
#ifdef MYDEBUG
      if (i == 0)
        std::cout << std::endl << "Areas: " << std::endl;
      std::cout << area_config_item.first << "=" << area_config_item.second << std::endl;
#endif
    }

    // forestfirewarnig parameters
    Settings::set("qdtext::forestfirewarning::directory", config.forestfirewarning_directory());
    for (unsigned int i = 0; i < config.numberOfFireWarningAreaCodes(); i++)
    {
      const std::pair<string, string>& area_code_config_item = config.getFireWarningAreaCode(i);
      Settings::set(area_code_config_item.first, area_code_config_item.second);
#ifdef MYDEBUG
      if (i == 0)
        std::cout << std::endl << "Area codes: " << std::endl;
      std::cout << area_code_config_item.first << "=" << area_code_config_item.second << std::endl;
#endif
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void handle_exception(const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse,
                      const std::string& what,
                      SmartMet::Spine::HTTP::Status debugStatus,
                      bool isdebug)
{
  try
  {
    std::cerr << boost::posix_time::second_clock::local_time() << " error: " << what << std::endl
              << "Query: " << theRequest.getURI() << std::endl;

    if (isdebug)
    {
      string msg = string("Error: ") + what;
      theResponse.setContent(msg);
      theResponse.setStatus(debugStatus);
    }
    else
    {
      theResponse.setStatus(SmartMet::Spine::HTTP::Status::internal_server_error);
    }

    // Remove newlines, make sure length is reasonable
    std::string msg = what;
    boost::algorithm::replace_all(msg, "\n", " ");
    msg = msg.substr(0, 100);
    theResponse.setHeader("X-TextGen-Error", msg.c_str());
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // anonymous namespace

bool Plugin::queryIsFast(const SmartMet::Spine::HTTP::Request& theRequest) const
{
  // Uses databases and such
  // Also, is rarely used interactively
  return false;
}
// ----------------------------------------------------------------------
/*!
 * \brief Perform a TextGen query
 */
// ----------------------------------------------------------------------
string Plugin::query(SmartMet::Spine::Reactor& theReactor,
                     const SmartMet::Spine::HTTP::Request& theRequest,
                     SmartMet::Spine::HTTP::Response& /* theResponse */)
{
  try
  {
    TextGenPosixTime timestamp;
    vector<WeatherArea> weatherAreaVector;
    SmartMet::Spine::HTTP::ParamMap queryParameters(theRequest.getParameterMap());
    std::string errorMessage;

    if (!verifyHttpRequestParameters(queryParameters, errorMessage))
      throw SmartMet::Spine::Exception(BCP, errorMessage);

    std::string product_name(mmap_string(queryParameters, PRODUCT_PARAM, DEFAULT_PRODUCT_NAME));
    const ProductConfig& config = itsConfig.getProductConfig(product_name);

    // set text generator settings (stored in thread local storage)
    set_textgen_settings(config);

    boost::shared_ptr<TextGen::Dictionary> theDictionary;
    std::string dictionary_name(config.dictionary());
    if (dictionary_name.compare("multimysqlplusgeonames") == 0)
      theDictionary = (static_cast<boost::shared_ptr<TextGen::Dictionary> >(
          new MySQLDictionariesPlusGeonames()));
    else
      theDictionary = (static_cast<boost::shared_ptr<TextGen::Dictionary> >(
          (TextGen::DictionaryFactory::create(dictionary_name))));

    theDictionary->init(mmap_string(queryParameters, LANGUAGE_PARAM));
    theDictionary->geoinit(&theReactor);

    if (!parse_forecasttime_parameter(
            mmap_string(queryParameters, FORECASTTIME_PARAM), timestamp, errorMessage))
    {
      throw SmartMet::Spine::Exception(BCP, errorMessage);
    }

    if (!parse_area_parameter(mmap_string(queryParameters, AREA_PARAM),
                              itsPostGISDataSource,
                              weatherAreaVector,
                              itsPostGISMutex,
                              errorMessage))
    {
      throw SmartMet::Spine::Exception(BCP, errorMessage);
    }

    if (!parse_lonlat_parameter(mmap_string(queryParameters, LONLAT_PARAM),
                                theDictionary,
                                weatherAreaVector,
                                errorMessage))
    {
      throw SmartMet::Spine::Exception(BCP, errorMessage);
    }

    if (!parse_geoid_parameter(mmap_string(queryParameters, GEOID_PARAM),
                               itsPostGISDataSource,
                               *itsGeoEngine,
                               mmap_string(queryParameters, LANGUAGE_PARAM),
                               weatherAreaVector,
                               errorMessage))
    {
      throw SmartMet::Spine::Exception(BCP, errorMessage);
    }

    // set locale
    const string loc = mmap_string(queryParameters, LOCALE_PARAM, config.locale());

    boost::locale::generator gen;
    std::locale::global(gen(loc));

    std::string formatter_name(mmap_string(queryParameters, FORMATTER_PARAM));

    stringstream timestamp_cachekey_ss;
    // generate forecast at least every 60 secods
    timestamp_cachekey_ss << (timestamp.EpochTime() / 60);
    std::string cache_key_common_part(
        mmap_string(queryParameters, PRODUCT_PARAM) + ";" +
        mmap_string(queryParameters, LANGUAGE_PARAM) + ";" + formatter_name + ";" +
        mmap_string(queryParameters, POSTGIS_PARAM) + ";" + timestamp_cachekey_ss.str());

    mask_map theMaskContainer = itsProductMasks[product_name];
    bool masksExists(theMaskContainer.find(LAND_MASK_NAME) != theMaskContainer.end() &&
                     theMaskContainer.find(COAST_MASK_NAME) != theMaskContainer.end());

    TextGen::TextGenerator generator(
        masksExists ? TextGen::TextGenerator(*(theMaskContainer[LAND_MASK_NAME]),
                                             *(theMaskContainer[COAST_MASK_NAME]))
                    : TextGen::TextGenerator());

    std::string forecast_text("");
    for (unsigned int i = 0; i < weatherAreaVector.size(); i++)
    {
      std::string forecast_text_area("");
      std::string area_name((weatherAreaVector[i]).name());
      std::string cache_key(cache_key_common_part + ";" + area_name);

      // set timezone for the area (stored in thread local storage)
      TextGenPosixTime::SetThreadTimeZone(config.getAreaTimeZone(area_name));

      SmartMet::Spine::UpgradeReadLock cacheReadLock(itsForecastTextCacheMutex);

      auto cache_result = itsForecastTextCache.find(cache_key);
      if (cache_result)
      {
#ifdef MYDEBUG
        std::cout << "Fetching forecast from cache." << std::endl;
#endif
        forecast_text_area = cache_result->member;
      }
      else
      {
        const WeatherArea& area = weatherAreaVector[i];

        // create formatter
        boost::shared_ptr<TextGen::TextFormatter> formatter(
            TextGen::TextFormatterFactory::create(formatter_name));
        formatter->dictionary(theDictionary);

#ifdef MYDEBUG
        std::cout << "Generating new forecast" << std::endl;
#endif

        TextGenPosixTime forecasttime;
        parse_forecasttime_parameter(
            mmap_string(queryParameters, FORECASTTIME_PARAM), forecasttime, errorMessage);

        generator.time(forecasttime);

        const TextGen::Document document = generator.generate(area);
        forecast_text_area = formatter->format(document);

        cache_item ci;
        ci.member = forecast_text_area;
        SmartMet::Spine::UpgradeWriteLock cacheWriteLock(cacheReadLock);
        itsForecastTextCache.insert(cache_key, ci);
      }
      forecast_text += forecast_text_area;
    }

    return forecast_text;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Main content handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(SmartMet::Spine::Reactor& theReactor,
                            const SmartMet::Spine::HTTP::Request& theRequest,
                            SmartMet::Spine::HTTP::Response& theResponse)
{
  try
  {
    bool isdebug = SmartMet::Spine::optional_bool(theRequest.getParameter("debug"), false);

    // Default expiration time
    const int expires_seconds = 60;

    // Now
    ptime t_now = second_clock::universal_time();

    try
    {
      std::string response = query(theReactor, theRequest, theResponse);
      theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
      theResponse.setContent(response);

      // Build cache expiration time info
      ptime t_expires = t_now + seconds(expires_seconds);

      // The headers themselves
      boost::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));

      std::string cachecontrol =
          "public, max-age=" + boost::lexical_cast<std::string>(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");
      theResponse.setHeader("Cache-Control", cachecontrol.c_str());
      theResponse.setHeader("Expires", expiration.c_str());
      theResponse.setHeader("Last-Modified", modification.c_str());

      if (response.size() == 0)
      {
        std::cerr << "Warning: Empty input for request " << theRequest.getQueryString() << " from "
                  << theRequest.getClientIP() << std::endl;
      }

#ifdef MYDEBUG
      std::cout << "Output:" << std::endl << response << std::endl;
#endif
    }
    catch (...)
    {
      SmartMet::Spine::Exception exception(BCP, "Operation failed!", NULL);
      handle_exception(
          theRequest, theResponse, exception.what(), SmartMet::Spine::HTTP::Status::ok, isdebug);
    }

    // delete textgen settings of current thread
    Settings::release();
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig)
    : SmartMetPlugin(),
      itsReactor(theReactor),
      itsModuleName("Textgen"),
      itsConfig(theConfig),
      itsForecastTextCache(boost::numeric_cast<size_t>(itsConfig.getForecastTextCacheSize()))
{
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
    {
      std::cerr << "*** TextGen plugin and Server SmartMet  API version mismatch ***" << std::endl;
      return;
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin initialization
 */
// ----------------------------------------------------------------------

void Plugin::init()
{
  try
  {
    // read alla PostGIS data in at startup

    vector<string> product_names(itsConfig.getProductNames());
    BOOST_FOREACH (const string& product_name, product_names)
    {
      const ProductConfig& config = itsConfig.getProductConfig(product_name);

      // PostGIS
      std::vector<std::string> postgis_identifier_keys(config.getPostGISIdentifierKeys());
      for (unsigned int i = 0; i < postgis_identifier_keys.size(); i++)
      {
        SmartMet::Spine::postgis_identifier pgis_identifier(
            config.getPostGISIdentifier(postgis_identifier_keys[i]));
        std::string log_message;
        itsPostGISDataSource.readData(pgis_identifier, log_message);
#ifdef MYDEBUG
        std::cout << "Reading data from PostGIS: " << std::endl
                  << "host: " << pgis_identifier.postGISHost << std::endl
                  << "port: " << pgis_identifier.postGISPort << std::endl
                  << "dbname: " << pgis_identifier.postGISDatabase << std::endl
                  << "username: " << pgis_identifier.postGISUsername << std::endl
                  << "password: " << pgis_identifier.postGISPassword << std::endl
                  << "schema: " << pgis_identifier.postGISSchema << std::endl
                  << "table: " << pgis_identifier.postGISTable << std::endl
                  << "field: " << pgis_identifier.postGISField << std::endl
                  << "client encoding: " << pgis_identifier.postGISClientEncoding << std::endl
                  << std::endl;
#endif
      }

#ifdef MYDEBUG
      std::cout << "Masks for product " << product_name << ":" << std::endl;
#endif
      // masks
      mask_map prod_mask;
      for (unsigned int i = 0; i < config.numberOfMasks(); i++)
      {
        std::string name(config.getMask(i).first);
        std::string value(config.getMask(i).second);

#ifdef MYDEBUG
        std::cout << name << "=" << value << std::endl;
#endif
        // first check if mask can be found in PostGIS database
        if (itsPostGISDataSource.geoObjectExists(value))
        {
          std::string area_name(value);
          prod_mask.insert(make_pair(name,
                                     boost::shared_ptr<TextGen::WeatherArea>(new WeatherArea(
                                         make_area(area_name, itsPostGISDataSource)))));
        }
        else
        {
          std::string filename(value);
          if (filename.find(':') != std::string::npos)
            filename = filename.substr(0, filename.find(':'));
          // mask is probably a svg-file
          if (NFmiFileSystem::FileExists(filename))
          {
            prod_mask.insert(make_pair(
                name, boost::shared_ptr<TextGen::WeatherArea>(new WeatherArea(value, name))));
          }
        }
      }
      itsProductMasks.insert(make_pair(product_name, prod_mask));
#ifdef MYDEBUG
      std::cout << std::endl;
#endif
    }

    /* GeoEngine */
    auto engine = itsReactor->getSingleton("Geonames", NULL);
    if (!engine)
      throw SmartMet::Spine::Exception(BCP, "Geonames engine unavailable");
    itsGeoEngine = reinterpret_cast<SmartMet::Engine::Geonames::Engine*>(engine);

    if (!itsReactor->addContentHandler(this,
                                       itsConfig.defaultUrl(),
                                       boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register textgen content handler");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the plugin
 */
// ----------------------------------------------------------------------

void Plugin::shutdown()
{
  std::cout << "  -- Shutdown requested (textgenplugin)\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

Plugin::~Plugin()
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the plugin name
 */
// ----------------------------------------------------------------------

const std::string& Plugin::getPluginName() const
{
  return itsModuleName;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the required version
 */
// ----------------------------------------------------------------------

int Plugin::getRequiredAPIVersion() const
{
  return SMARTMET_API_VERSION;
}

// check that minimum number of parameters are defined and set default values

bool Plugin::verifyHttpRequestParameters(SmartMet::Spine::HTTP::ParamMap& queryParameters,
                                         string& errorMessage)
{
  try
  {
    std::string product_name(mmap_string(queryParameters, PRODUCT_PARAM, DEFAULT_PRODUCT_NAME));

    if (!itsConfig.productConfigExists(product_name))
    {
      errorMessage = "Configuration missing for product '" + product_name + "'";
      return false;
    }

    const ProductConfig& config(itsConfig.getProductConfig(product_name));

    if (queryParameters.find(AREA_PARAM) == queryParameters.end() &&
        queryParameters.find(LONLAT_PARAM) == queryParameters.end() &&
        queryParameters.find(GEOID_PARAM) == queryParameters.end())
    {
      errorMessage = "area, lonlat or geoid parameter must be defined!";
      return false;
    }

    // set default values
    if (queryParameters.find(LANGUAGE_PARAM) == queryParameters.end())
      queryParameters.insert(make_pair(LANGUAGE_PARAM, config.language()));

    if (queryParameters.find(FORMATTER_PARAM) == queryParameters.end())
      queryParameters.insert(make_pair(FORMATTER_PARAM, config.formatter()));

    if (queryParameters.find(LOCALE_PARAM) == queryParameters.end())
      queryParameters.insert(make_pair(LOCALE_PARAM, config.locale()));

    if (!parse_postgis_parameters(
            queryParameters, config, itsPostGISDataSource, itsPostGISMutex, errorMessage))
    {
      return false;
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

/*
 * Server knows us through the 'SmartMetPlugin' virtual interface, which
 * the 'Plugin' class implements.
 */

extern "C" SmartMetPlugin* create(SmartMet::Spine::Reactor* them, const char* config)
{
  return new SmartMet::Plugin::Textgen::Plugin(them, config);
}

extern "C" void destroy(SmartMetPlugin* us)
{
  // This will call 'Plugin::~Plugin()' since the destructor is virtual
  delete us;
}
