#include "Plugin.h"
#include "FileDictionaryPlusGeonames.h"
#include "MySQLDictionariesPlusGeonames.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/locale.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <calculator/Settings.h>
#include <calculator/TextGenPosixTime.h>
#include <calculator/TimeTools.h>
#include <engines/geonames/Engine.h>
#include <engines/geonames/WktGeometry.h>
#include <engines/gis/Engine.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TimeFormatter.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiFileSystem.h>
#include <newbase/NFmiStringTools.h>
#include <spine/Convenience.h>
#include <spine/Exception.h>
#include <spine/Location.h>
#include <spine/SmartMet.h>
#include <spine/Table.h>
#include <spine/TableFormatterFactory.h>
#include <spine/ValueFormatter.h>
#include <textgen/DictionaryFactory.h>
#include <textgen/Document.h>
#include <textgen/MessageLogger.h>
#include <textgen/TextFormatter.h>
#include <textgen/TextFormatterFactory.h>
#include <textgen/TextGenerator.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
#define CACHE_EXPIRATION_TIME_SEC 60
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
      auto year = boost::lexical_cast<short>(forecasttime_string.substr(0, 4).c_str());
      auto month = boost::lexical_cast<short>(forecasttime_string.substr(4, 2).c_str());
      auto day = boost::lexical_cast<short>(forecasttime_string.substr(6, 2).c_str());
      auto hour = boost::lexical_cast<short>(forecasttime_string.substr(8, 2).c_str());
      auto min = boost::lexical_cast<short>(forecasttime_string.substr(10, 2).c_str());
      short sec = 0;
      if (forecasttime_string.size() > 12)
        sec = boost::lexical_cast<short>(forecasttime_string.substr(12, 2).c_str());

      forecasttime = TextGenPosixTime(year, month, day, hour, min, sec);
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

TextGen::WeatherArea make_area(const std::string& postGISName,
                               const Engine::Gis::GeometryStorage& geometryStorage)
{
  try
  {
    std::string areaName(postGISName);
    normalize_string(areaName);

    if (geometryStorage.isPolygon(postGISName))
    {
      std::stringstream svg_string_stream(geometryStorage.getSVGPath(postGISName));
      NFmiSvgPath svgPath;
      svgPath.Read(svg_string_stream);
      return {svgPath, areaName};
    }

    // if not polygon, it must be a point
    std::pair<float, float> std_point(geometryStorage.getPoint(postGISName));
    NFmiPoint point(std_point.first, std_point.second);
    return {point, areaName};
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

bool parse_location_parameters(const Spine::HTTP::Request& theRequest,
                               SmartMet::Spine::MutexType& thePostGISMutex,
                               const Engine::Gis::GeometryStorage& geometryStorage,
                               const SmartMet::Engine::Geonames::Engine& geoEngine,
                               const boost::shared_ptr<TextGen::Dictionary>& theDictionary,
                               const std::string& language,
                               std::vector<TextGen::WeatherArea>& weatherAreaVector,
                               std::string& errorMessage)
{
  try
  {
    Spine::HTTP::Request httpRequest = theRequest;

    // If bbox-parameter exists, convert it to wkt-parameter
    boost::optional<std::string> bbox_param_value = httpRequest.getParameter("bbox");
    if (bbox_param_value)
    {
      std::string bbox_name = "";
      std::string bbox_string = *bbox_param_value;
      size_t name_pos = bbox_string.find(" as ");
      if (name_pos != std::string::npos)
      {
        bbox_name = bbox_string.substr(name_pos + 4);
        bbox_string = bbox_string.substr(0, name_pos);
      }
      size_t radius_pos = bbox_string.find(":");
      std::string radius = "";
      if (radius_pos != std::string::npos)
      {
        radius = bbox_string.substr(radius_pos + 1);
        bbox_string = bbox_string.substr(0, radius_pos);
      }
      std::vector<std::string> parts;
      boost::algorithm::split(parts, bbox_string, boost::algorithm::is_any_of(","));
      if (parts.size() != 4)
        throw Spine::Exception(BCP,
                               "Invalid bbox parameter " + bbox_string +
                                   ", should be in format 'lon,lat,lon,lat[:radius] [as name]'!");

      std::string wktString("POLYGON((");
      wktString += (parts[0] + " " + parts[1] + ", ");
      wktString += (parts[0] + " " + parts[3] + ", ");
      wktString += (parts[2] + " " + parts[3] + ", ");
      wktString += (parts[2] + " " + parts[1] + ", ");
      wktString += (parts[0] + " " + parts[1] + "))");
      if (!radius.empty())
        wktString += (":" + radius);
      if (!bbox_name.empty())
        wktString += (" as " + bbox_name);

      httpRequest.removeParameter("bbox");
      httpRequest.setParameter("wkt", wktString);
    }

    auto tagged_locations = geoEngine.parseLocations(httpRequest);

    if (tagged_locations.empty())
    {
      errorMessage += "No locations specified";
      return false;
    }

    for (const auto& tagged_loc : tagged_locations.locations())
    {
      const auto& loc = *tagged_loc.loc;
      switch (loc.type)
      {
        case Spine::Location::LocationType::Place:
        case Spine::Location::LocationType::CoordinatePoint:
        {
          if (loc.feature.substr(0, 3) == "ADM" && geometryStorage.geoObjectExists(loc.name))
            weatherAreaVector.emplace_back(
                TextGen::WeatherArea(make_area(loc.name, geometryStorage)));
          else
          {
            auto geoname = loc.name;
            normalize_string(geoname);
            weatherAreaVector.emplace_back(
                TextGen::WeatherArea(NFmiPoint(loc.longitude, loc.latitude),
                                     geoname,
                                     (loc.radius && loc.radius >= 5.0) ? loc.radius : 0.0));
          }
          break;
        }
        case Spine::Location::LocationType::Area:
        {
          SmartMet::Spine::ReadLock lock(thePostGISMutex);
          if (geometryStorage.geoObjectExists(loc.name))
            weatherAreaVector.emplace_back(
                TextGen::WeatherArea(make_area(loc.name, geometryStorage)));
          else
          {
            errorMessage += "Area " + loc.name + " not found in PostGIS database!";
            return false;
          }
          break;
        }
        case Spine::Location::LocationType::BoundingBox:
        {
          // We should never end up here because bbox parameter is converted to wkt parameter
          throw Spine::Exception(BCP,
                                 "Something wrong: BoundingBox should be handled as WKT POLYGON!");
          break;
        }
        case Spine::Location::LocationType::Wkt:
        {
          Engine::Geonames::WktGeometries wktGeometries =
              geoEngine.getWktGeometries(tagged_locations, language);

          Spine::LocationPtr wktLocation = wktGeometries.getLocation(loc.name);
          Spine::Location::LocationType wktType = wktLocation->type;
          //        TextGen::WeatherArea wktWeatherArea(NFmiPoint(0.0, 0.0));
          std::string wktName = loc.name;
          size_t wktNamePos = wktName.find(" as ");
          if (wktNamePos != std::string::npos)
            wktName = wktName.substr(wktNamePos + 4);
          switch (wktType)
          {
            case Spine::Location::LocationType::CoordinatePoint:
            {
              int coordinate_string_len = (loc.name.find(")") - loc.name.find("(")) - 1;
              std::string coordinates =
                  loc.name.substr(loc.name.find("(") + 1, coordinate_string_len);
              double lon = Fmi::stod(coordinates.substr(0, coordinates.find(" ")));
              double lat = Fmi::stod(coordinates.substr(coordinates.find(" ") + 1));
              weatherAreaVector.emplace_back(
                  TextGen::WeatherArea(NFmiPoint(lon, lat),
                                       wktName,
                                       (loc.radius && loc.radius >= 5.0) ? loc.radius : 0.0));
            }
            break;
            case Spine::Location::LocationType::Area:
            case Spine::Location::LocationType::Path:
            {
              weatherAreaVector.emplace_back(
                  TextGen::WeatherArea(wktGeometries.getSvgPath(loc.name), wktName));
            }
            break;
            default:
              std::cout << "WKT type not supported: " << wktType << std::endl;
              return false;
              break;
          }
          break;
        }
        case Spine::Location::LocationType::Path:
        {
          errorMessage += "paths not supported";
          return false;
        }
      }
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Location parameter parsing failed!");
  }
}

bool parse_postgis_parameters(const SmartMet::Spine::HTTP::ParamMap& queryParameters,
                              const ProductConfig& config,
                              const SmartMet::Engine::Gis::Engine* gisEngine,
                              Engine::Gis::GeometryStorage& geometryStorage,
                              SmartMet::Spine::MutexType& thePostGISMutex,
                              std::string& errorMessage)
{
  try
  {
    if (queryParameters.find(POSTGIS_PARAM) == queryParameters.end() &&
        config.numberOfPostGISIdentifiers() == 0)
    {
      return true;
    }

    std::pair<SmartMet::Spine::HTTP::ParamMap::const_iterator,
              SmartMet::Spine::HTTP::ParamMap::const_iterator>
        iter_range = queryParameters.equal_range(POSTGIS_PARAM);
    SmartMet::Spine::HTTP::ParamMap::const_iterator iter;

    for (iter = iter_range.first; iter != iter_range.second; ++iter)
    {
      std::string postgis_string = iter->second;

      // format: schema=schema1,port=port1,dbname=dbname1,table=table1
      if (postgis_string.find(',') != std::string::npos)
      {
        const Engine::Gis::postgis_identifier& default_pgis_id =
            config.getDefaultPostGISIdentifier();

        std::string pgis_host = default_pgis_id.host;
        std::string pgis_port = default_pgis_id.port;
        std::string pgis_dbname = default_pgis_id.database;
        std::string pgis_username = default_pgis_id.username;
        std::string pgis_password = default_pgis_id.password;
        std::string pgis_schema = default_pgis_id.schema;
        std::string pgis_table = default_pgis_id.table;
        std::string pgis_field = default_pgis_id.field;
        std::string pgis_client_encoding = default_pgis_id.encoding;

        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char> > tokens(postgis_string, sep);
        for (const std::string& t : tokens)
        {
          unsigned long assign_index = t.find('=');
          if (assign_index == std::string::npos)
          {
            errorMessage = "Erroneous PostGIS parameter: " + t;
            return false;
          }
          std::string key(t.substr(0, assign_index));
          std::string value(t.substr(assign_index + 1));

          if (key == POSTGIS_HOST_PARAM)
            pgis_host = value;
          else if (key == POSTGIS_PORT_PARAM)
            pgis_port = value;
          else if (key == POSTGIS_DBNAME_PARAM)
            pgis_dbname = value;
          else if (key == POSTGIS_SCHEMA_PARAM)
            pgis_schema = value;
          else if (key == POSTGIS_TABLE_PARAM)
            pgis_table = value;
          else if (key == POSTGIS_FIELD_PARAM)
            pgis_field = value;
          else if (key == POSTGIS_USERNAME_PARAM)
            pgis_username = value;
          else if (key == POSTGIS_PASSWORD_PARAM)
            pgis_password = value;
          else if (key == POSTGIS_CLIENT_ENCODING_PARAM)
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

        Engine::Gis::postgis_identifier pgid;
        pgid.host = pgis_host;
        pgid.port = pgis_port;
        pgid.database = pgis_dbname;
        pgid.username = pgis_username;
        pgid.password = pgis_password;
        pgid.schema = pgis_schema;
        pgid.table = pgis_table;
        pgid.field = pgis_field;
        pgid.encoding = pgis_client_encoding;
        Engine::Gis::PostGISIdentifierVector pgIdVector;
        pgIdVector.push_back(pgid);

        SmartMet::Spine::WriteLock lock(thePostGISMutex);
        std::string log_message;

        gisEngine->populateGeometryStorage(pgIdVector, geometryStorage);
      }
    }

    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string get_setting_string(const std::string& key,
                               const std::string& default_value,
                               const SmartMet::Spine::HTTP::ParamMap& params,
                               std::string& modified_params)
{
  for (auto p : params)
  {
    if (boost::iends_with(key, p.first))
    {
      if (p.second != default_value)
      {
#ifdef MYDEBUG
        std::cout << key << " -> replacing " << default_value << " with " << p.second << std::endl;
#endif
        if (modified_params.empty())
          modified_params += ";";
        modified_params += (key + "=" + p.second);
      }
      return p.second;
    }
  }

  return default_value;
}

void set_textgen_settings(const ProductConfig& config,
                          const SmartMet::Spine::HTTP::ParamMap& params,
                          std::string& modified_params)
{
  try
  {
#ifdef MYDEBUG
    std::cout << "*** Test generator settings ***" << std::endl;
#endif
    // frostseason-parameter is used by old stories
    Settings::set("textgen::frostseason", (config.isFrostSeason() ? "true" : "false"));

    // Parameter mappings
    ParameterMappings pm = config.getParameterMappings();
    for (const auto& item : pm)
    {
      std::string configname = item.first;
      std::string qdname = item.second;
      Settings::set(configname, qdname);
    }

    // MySQL database
    Settings::set("textgen::host", config.mySQLDictionaryHost());
    Settings::set("textgen::user", config.mySQLDictionaryUsername());
    Settings::set("textgen::passwd", config.mySQLDictionaryPassword());
    Settings::set("textgen::database", config.mySQLDictionaryDatabase());
    Settings::set("textgen::filedictionaries", config.fileDictionaries());

#ifdef MYDEBUG
    std::cout << "MySQL dictionary: " << std::endl
              << "textgen::host=" << config.mySQLDictionaryHost() << std::endl
              << "textgen::user=" << config.mySQLDictionaryUsername() << std::endl
              << "textgen::passwd=" << config.mySQLDictionaryPassword() << std::endl
              << "textgen::database=" << config.mySQLDictionaryDatabase() << std::endl
              << "textgen::filedictionaries=" << config.fileDictionaries() << std::endl
              << "textgen::frostseason=" << (config.isFrostSeason() ? "true" : "false") << std::endl
              << std::endl;
#endif

    // forecasts and the querydata
    for (unsigned int i = 0; i < config.numberOfForecastDataConfigs(); i++)
    {
      const auto& forecast_item = config.getForecastDataConfig(i);
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
      const auto& unit_format_item = config.getUnitFormatConfig(i);
      std::string setting_string = get_setting_string(
          unit_format_item.first, unit_format_item.second, params, modified_params);

      Settings::set("textgen::units::" + unit_format_item.first + "::format", setting_string);
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
      const auto& output_document_config_item = config.getOutputDocumentConfig(i);
      std::string setting_string = get_setting_string(output_document_config_item.first,
                                                      output_document_config_item.second,
                                                      params,
                                                      modified_params);
      Settings::set(output_document_config_item.first, setting_string);
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
      const auto& area_config_item = config.getAreaConfig(i);
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
      const auto& area_code_config_item = config.getFireWarningAreaCode(i);
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

void handle_exception(const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse,
                      const std::string& what,
                      const std::string& log,
                      SmartMet::Spine::HTTP::Status debugStatus,
                      bool isdebug)
{
  try
  {
    std::cerr << Spine::log_time_str() << " error: " << what << std::endl
              << "Query: " << theRequest.getURI() << std::endl
              << "ClientIP: " << theRequest.getClientIP() << std::endl;

    if (isdebug)
    {
      std::string msg = "Error: " + what;
      theResponse.setContent(msg + "\n\n" + log);
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
    theResponse.setHeader("X-TextGen-Error", msg);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

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
std::string Plugin::query(SmartMet::Spine::Reactor& theReactor,
                          const SmartMet::Spine::HTTP::Request& theRequest,
                          SmartMet::Spine::HTTP::Response& /* theResponse */)
{
  try
  {
    TextGenPosixTime timestamp;
    std::vector<TextGen::WeatherArea> weatherAreaVector;
    SmartMet::Spine::HTTP::ParamMap queryParameters(theRequest.getParameterMap());
    std::string errorMessage;

    if (!verifyHttpRequestParameters(queryParameters, errorMessage))
      throw SmartMet::Spine::Exception(BCP, errorMessage);

    std::string product_name(mmap_string(queryParameters, PRODUCT_PARAM, DEFAULT_PRODUCT_NAME));
    const ProductConfig& config = itsConfig.getProductConfig(product_name);
    bool configIsModified = config.isModified(CACHE_EXPIRATION_TIME_SEC);

    // set text generator settings (stored in thread local storage)
    std::string modified_params;
    set_textgen_settings(config, queryParameters, modified_params);

    boost::shared_ptr<TextGen::Dictionary> theDictionary;
    const auto& dictionary_name = config.dictionary();
    if (dictionary_name == "multimysqlplusgeonames")
      theDictionary = boost::make_shared<MySQLDictionariesPlusGeonames>();
    else if (dictionary_name == "fileplusgeonames")
      theDictionary = boost::make_shared<FileDictionaryPlusGeonames>();
    else
      theDictionary = static_cast<boost::shared_ptr<TextGen::Dictionary> >(
          (TextGen::DictionaryFactory::create(dictionary_name)));

    theDictionary->init(mmap_string(queryParameters, LANGUAGE_PARAM));
    theDictionary->geoinit(&theReactor);

    if (!parse_forecasttime_parameter(
            mmap_string(queryParameters, FORECASTTIME_PARAM), timestamp, errorMessage))
    {
      throw SmartMet::Spine::Exception(BCP, errorMessage);
    }

    if (!parse_location_parameters(theRequest,
                                   itsPostGISMutex,
                                   itsGeometryStorage,
                                   *itsGeoEngine,
                                   theDictionary,
                                   mmap_string(queryParameters, LANGUAGE_PARAM),
                                   weatherAreaVector,
                                   errorMessage))
    {
      throw SmartMet::Spine::Exception(BCP, errorMessage);
    }

    // set locale
    const std::string loc = mmap_string(queryParameters, LOCALE_PARAM, config.locale());

    if (!loc.empty())
    {
      boost::locale::generator gen;
      std::locale::global(gen(loc));
    }

    std::string formatter_name(mmap_string(queryParameters, FORMATTER_PARAM));

    std::stringstream timestamp_cachekey_ss;
    // generate forecast at least every CACHE_EXPIRATION_TIME_SEC secods
    timestamp_cachekey_ss << (timestamp.EpochTime() / CACHE_EXPIRATION_TIME_SEC);
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

    std::string forecast_text;

    auto wktParam = queryParameters.find("wkt");
    if (wktParam != queryParameters.end())
      modified_params += (";" + wktParam->second);

    for (const auto& area : weatherAreaVector)
    {
      std::string forecast_text_area;
      const auto& area_name = area.name();
      std::string cache_key = cache_key_common_part + ";" + area_name + ";" + modified_params;

      // set timezone for the area (stored in thread local storage)
      TextGenPosixTime::SetThreadTimeZone(config.getAreaTimeZone(area_name));

      SmartMet::Spine::UpgradeReadLock cacheReadLock(itsForecastTextCacheMutex);

      auto cache_result = itsForecastTextCache.find(cache_key);
      if (!configIsModified && cache_result)
      {
#ifdef MYDEBUG
        std::cout << "Fetching forecast from cache " << cache_key << std::endl;
#endif
        forecast_text_area = cache_result->member;
      }
      else
      {
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
  // thead specific stringstream logging
  MessageLogger logger("SmartMet::Plugin::TextGen::requestHandler");
  logger.open();

  try
  {
    const bool isdebug = SmartMet::Spine::optional_bool(theRequest.getParameter("debug"), false);
    const bool print_log = Spine::optional_bool(theRequest.getParameter("printlog"), false);

    // Default expiration time
    const int expires_seconds = CACHE_EXPIRATION_TIME_SEC;

    // Now
    auto t_now = boost::posix_time::second_clock::universal_time();

    try
    {
      std::string response = query(theReactor, theRequest, theResponse);
      theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
      if (!isdebug)
        theResponse.setContent(response);
      else
        theResponse.setContent(response + "\n<pre>" + logger.str() + "</pre>");

      // Build cache expiration time info
      auto t_expires = t_now + boost::posix_time::seconds(expires_seconds);

      // The headers themselves
      boost::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));

      std::string cachecontrol = "public, max-age=" + Fmi::to_string(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");
      theResponse.setHeader("Cache-Control", cachecontrol);
      theResponse.setHeader("Expires", expiration.c_str());
      theResponse.setHeader("Last-Modified", modification);

      if (response.empty())
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
      SmartMet::Spine::Exception exception(BCP, "Operation failed!", nullptr);
      handle_exception(theRequest,
                       theResponse,
                       exception.what(),
                       logger.str(),
                       SmartMet::Spine::HTTP::Status::ok,
                       isdebug);
    }

    if (print_log)
      std::cout << logger.str() << std::endl;

    // delete textgen settings of current thread
    Settings::release();
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig)
    : itsReactor(theReactor),
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
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
    /* GeoEngine */
    auto engine = itsReactor->getSingleton("Geonames", nullptr);
    if (engine == nullptr)
      throw SmartMet::Spine::Exception(BCP, "Geonames engine unavailable");
    itsGeoEngine = reinterpret_cast<SmartMet::Engine::Geonames::Engine*>(engine);

    /* GisEngine */
    engine = itsReactor->getSingleton("Gis", nullptr);
    if (engine == nullptr)
      throw Spine::Exception(BCP, "Gis engine unavailable");
    itsGisEngine = reinterpret_cast<Engine::Gis::Engine*>(engine);

    // PostGIS
    std::vector<std::string> product_names(itsConfig.getProductNames());
    for (auto product_name : product_names)
    {
      const ProductConfig& config = itsConfig.getProductConfig(product_name);
      itsGisEngine->populateGeometryStorage(config.getPostGISIdentifiers(), itsGeometryStorage);

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
        if (itsGeometryStorage.geoObjectExists(value))
        {
          std::string area_name(value);
          prod_mask.insert(
              make_pair(name,
                        boost::shared_ptr<TextGen::WeatherArea>(
                            new TextGen::WeatherArea(make_area(area_name, itsGeometryStorage)))));
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
                name,
                boost::shared_ptr<TextGen::WeatherArea>(new TextGen::WeatherArea(value, name))));
          }
        }
      }
      itsProductMasks.insert(make_pair(product_name, prod_mask));
#ifdef MYDEBUG
      std::cout << std::endl;
#endif
    }

    if (!itsReactor->addContentHandler(this,
                                       itsConfig.defaultUrl(),
                                       boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register textgen content handler");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
                                         std::string& errorMessage)
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

    // set default values
    if (queryParameters.find(LANGUAGE_PARAM) == queryParameters.end())
      queryParameters.insert(make_pair(LANGUAGE_PARAM, config.language()));

    if (queryParameters.find(FORMATTER_PARAM) == queryParameters.end())
      queryParameters.insert(make_pair(FORMATTER_PARAM, config.formatter()));

    if (queryParameters.find(LOCALE_PARAM) == queryParameters.end())
      queryParameters.insert(make_pair(LOCALE_PARAM, config.locale()));

    return parse_postgis_parameters(
        queryParameters, config, itsGisEngine, itsGeometryStorage, itsPostGISMutex, errorMessage);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception::Trace(BCP, "Operation failed!");
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
  delete us;  // NOLINT
}
