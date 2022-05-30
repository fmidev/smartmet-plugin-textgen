#include "Plugin.h"
#include "DatabaseDictionariesPlusGeonames.h"
#include "FileDictionariesPlusGeonames.h"
#include "FileDictionaryPlusGeonames.h"
#include <boost/locale.hpp>
#include <calculator/Settings.h>
#include <engines/geonames/Engine.h>
#include <engines/gis/Engine.h>
#include <engines/gis/Normalize.h>
#include <macgyver/Exception.h>
#include <macgyver/TimeFormatter.h>
#include <spine/Convenience.h>
#include <spine/Location.h>
#include <spine/Thread.h>
#include <textgen/Document.h>
#include <textgen/MessageLogger.h>
#include <textgen/TextFormatter.h>
#include <textgen/TextFormatterFactory.h>
#include <textgen/TextGenerator.h>

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
SmartMet::Spine::MutexType gDictionaryMutex;

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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool parse_location_parameters(const Spine::HTTP::Request& theRequest,
                               const Config& config,
                               const SmartMet::Engine::Geonames::Engine& geoEngine,
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
      std::string bbox_name;
      std::string bbox_string = *bbox_param_value;
      size_t name_pos = bbox_string.find(" as ");
      if (name_pos != std::string::npos)
      {
        bbox_name = bbox_string.substr(name_pos + 4);
        bbox_string = bbox_string.substr(0, name_pos);
      }
      size_t radius_pos = bbox_string.find(':');
      std::string radius;
      if (radius_pos != std::string::npos)
      {
        radius = bbox_string.substr(radius_pos + 1);
        bbox_string = bbox_string.substr(0, radius_pos);
      }
      std::vector<std::string> parts;
      boost::algorithm::split(parts, bbox_string, boost::algorithm::is_any_of(","));
      if (parts.size() != 4)
        throw Fmi::Exception(BCP,
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

    boost::optional<std::string> areasource = httpRequest.getParameter("areasource");
    if (!areasource)
      areasource = "";

    for (const auto& tagged_loc : tagged_locations.locations())
    {
      const auto& loc = *tagged_loc.loc;
      std::string shapename = (loc.name + *areasource);

      switch (loc.type)
      {
        case Spine::Location::LocationType::Place:
        case Spine::Location::LocationType::CoordinatePoint:
        {
          if (loc.feature.substr(0, 3) == "ADM" && config.geoObjectExists(loc.name, *areasource))
            weatherAreaVector.emplace_back(config.makePostGisArea(loc.name, *areasource));
          else
          {
            auto geoname = loc.name;
            Engine::Gis::normalize_string(geoname);
            weatherAreaVector.emplace_back(
                TextGen::WeatherArea(NFmiPoint(loc.longitude, loc.latitude),
                                     geoname,
                                     (loc.radius && loc.radius >= 5.0) ? loc.radius : 0.0));
          }
          break;
        }
        case Spine::Location::LocationType::Area:
        {
          if (config.geoObjectExists(loc.name, *areasource))
          {
            weatherAreaVector.emplace_back(config.makePostGisArea(loc.name, *areasource));
          }
          else
          {
            if (!(*areasource).empty())
              errorMessage += "Area " + loc.name + " (areasource: " + *areasource +
                              ") not found in PostGIS database!";
            else
              errorMessage += "Area " + loc.name + " not found in PostGIS database!";
            return false;
          }
          break;
        }
        case Spine::Location::LocationType::BoundingBox:
        {
          // We should never end up here because bbox parameter is converted to wkt parameter
          throw Fmi::Exception(BCP,
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
              int coordinate_string_len = (loc.name.find(')') - loc.name.find('(')) - 1;
              std::string coordinates =
                  loc.name.substr(loc.name.find('(') + 1, coordinate_string_len);
              double lon = Fmi::stod(coordinates.substr(0, coordinates.find(' ')));
              double lat = Fmi::stod(coordinates.substr(coordinates.find(' ') + 1));
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
    throw Fmi::Exception::Trace(BCP, "Location parameter parsing failed!");
  }
}

std::string get_setting_string(const std::string& key,
                               const std::string& default_value,
                               const SmartMet::Spine::HTTP::ParamMap& params,
                               std::string& modified_params)
{
  for (const auto& p : params)
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

bool Plugin::queryIsFast(const SmartMet::Spine::HTTP::Request& /*theRequest*/) const
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
std::string Plugin::query(SmartMet::Spine::Reactor& /*theReactor*/,
                          const SmartMet::Spine::HTTP::Request& theRequest,
                          SmartMet::Spine::HTTP::Response& /* theResponse */)
{
  try
  {
    std::vector<TextGen::WeatherArea> weatherAreaVector;
    SmartMet::Spine::HTTP::ParamMap queryParameters(theRequest.getParameterMap());
    std::string errorMessage;

    if (!verifyHttpRequestParameters(queryParameters, errorMessage))
      throw Fmi::Exception(BCP, errorMessage);

    SmartMet::Spine::ReadLock lock(itsConfig.itsConfigUpdateMutex);

    std::string product_name(mmap_string(queryParameters, PRODUCT_PARAM, DEFAULT_PRODUCT_NAME));
    const ProductConfig& config = itsConfig.getProductConfig(product_name);
    bool configIsModified = config.isModified(CACHE_EXPIRATION_TIME_SEC);

    // set text generator settings (stored in thread local storage)
    std::string modified_params;
    set_textgen_settings(config, queryParameters, modified_params);

    std::string languageParam = mmap_string(queryParameters, LANGUAGE_PARAM);

    if (!parse_location_parameters(
            theRequest, itsConfig, *itsGeoEngine, languageParam, weatherAreaVector, errorMessage))
    {
      throw Fmi::Exception(BCP, errorMessage);
    }

    // set locale
    const std::string loc = mmap_string(queryParameters, LOCALE_PARAM, config.locale());

    if (!loc.empty())
    {
      boost::locale::generator gen;
      std::locale::global(gen(loc));
    }

    std::string formatter_name(mmap_string(queryParameters, FORMATTER_PARAM));

    TextGenPosixTime forecasttime;
    if (!parse_forecasttime_parameter(
            mmap_string(queryParameters, FORECASTTIME_PARAM), forecasttime, errorMessage))
    {
      throw Fmi::Exception(BCP, errorMessage);
    }
    TextGenPosixTime timestamp;
    std::stringstream timestamp_cachekey_ss;
    // generate forecast at least every CACHE_EXPIRATION_TIME_SEC secods
    timestamp_cachekey_ss << (forecasttime.EpochTime() / CACHE_EXPIRATION_TIME_SEC);
    timestamp_cachekey_ss << ";" << (timestamp.EpochTime() / CACHE_EXPIRATION_TIME_SEC);

    std::string cache_key_common_part(
        mmap_string(queryParameters, PRODUCT_PARAM) + ";" + languageParam + ";" + formatter_name +
        ";" + mmap_string(queryParameters, POSTGIS_PARAM) + ";" + timestamp_cachekey_ss.str());

    const WeatherAreas& theMaskContainer = itsConfig.getProductMasks(product_name);

    bool masksExists(theMaskContainer.find(LAND_MASK_NAME) != theMaskContainer.end() &&
                     theMaskContainer.find(COAST_MASK_NAME) != theMaskContainer.end());

    TextGen::TextGenerator generator(
        masksExists ? TextGen::TextGenerator(theMaskContainer.at(LAND_MASK_NAME),
                                             theMaskContainer.at(COAST_MASK_NAME))
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
        formatter->dictionary(itsDictionary);

#ifdef MYDEBUG
        std::cout << "Generating new forecast" << std::endl;
#endif

        generator.time(forecasttime);

        const TextGen::Document document = generator.generate(area);

        {
          SmartMet::Spine::WriteLock lock(gDictionaryMutex);
          itsDictionary->changeLanguage(languageParam);
          forecast_text_area = formatter->format(document);
        }

        cache_item ci;
        ci.member = forecast_text_area;
        itsForecastTextCache.insert(cache_key, ci);
      }
      forecast_text += forecast_text_area;
    }

    return forecast_text;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
  MessageLogger::open();

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
      theResponse.setHeader("Access-Control-Allow-Origin", "*");

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
      theResponse.setHeader("Expires", expiration);
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
      Fmi::Exception exception(BCP, "Operation failed!", nullptr);
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin initialization
 */
// ----------------------------------------------------------------------

void Plugin::init()
{
  using namespace boost::placeholders;

  try
  {
    /* GeoEngine */
    auto* engine = itsReactor->getSingleton("Geonames", nullptr);
    if (engine == nullptr)
      throw Fmi::Exception(BCP, "Geonames engine unavailable");

    itsGeoEngine = reinterpret_cast<SmartMet::Engine::Geonames::Engine*>(engine);

    /* GisEngine */
    engine = itsReactor->getSingleton("Gis", nullptr);
    if (engine == nullptr)
      throw Fmi::Exception(BCP, "Gis engine unavailable");

    itsConfig.init(reinterpret_cast<Engine::Gis::Engine*>(engine));

    /* Initialize dictionary */
    const auto& dictionary_name = itsConfig.dictionary();
    if (dictionary_name == "multimysqlplusgeonames")
      itsDictionary = boost::make_shared<DatabaseDictionariesPlusGeonames>("mysql");
    else if (dictionary_name == "multipostgresqlplusgeonames")
      itsDictionary = boost::make_shared<DatabaseDictionariesPlusGeonames>("postgresql");
    else if (dictionary_name == "multifileplusgeonames")
      itsDictionary = boost::make_shared<FileDictionariesPlusGeonames>();
    else
      itsDictionary = static_cast<boost::shared_ptr<TextGen::Dictionary> >(
          (TextGen::DictionaryFactory::create(dictionary_name)));
    Settings::set("textgen::filedictionaries", itsConfig.fileDictionaries());

    const std::string dictionaryId = itsDictionary->getDictionaryId();
    if (dictionaryId == "mysql" || dictionaryId == "postgresql")
    {
      const db_connect_info& dci = itsConfig.getDatabaseConnectInfo(dictionaryId);

      // Database dictionary
      Settings::set("textgen::host", dci.host);
      Settings::set("textgen::port", Fmi::to_string(dci.port));
      Settings::set("textgen::user", dci.username);
      Settings::set("textgen::passwd", dci.password);
      Settings::set("textgen::database", dci.database);
      Settings::set("textgen::schema", dci.schema);
      Settings::set("textgen::encoding", dci.encoding);
      Settings::set("textgen::connect_timeout", Fmi::to_string(dci.connect_timeout));

#ifdef MYDEBUG
      std::cout << "Database dictionary for " << dictionary << ": " << std::endl
                << "textgen::host=" << dci.host << std::endl
                << "textgen::port=" << dci.port << std::endl
                << "textgen::user=" << dci.username << std::endl
                << "textgen::passwd=" << dci.password << std::endl
                << "textgen::database=" << dci.database << std::endl
                << "textgen::schema=" << dci.schema << std::endl
                << "textgen::encoding=" << dci.encoding << std::endl
                << "textgen::connect_timeout=" << dci.connect_timeout << std::endl
                << "textgen::filedictionaries=" << itsConfig.fileDictionaries() << std::endl
                << "textgen::frostseason=" << (config.isFrostSeason() ? "true" : "false")
                << std::endl
                << std::endl;
#endif
    }

    itsDictionary->geoinit(itsGeoEngine);

    // Read all languages at init
    for (const auto& lang : itsConfig.supportedLanguages())
      itsDictionary->init(lang);

    if (!itsReactor->addContentHandler(this,
                                       itsConfig.defaultUrl(),
                                       boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
      throw Fmi::Exception(BCP, "Failed to register textgen content handler");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
  itsConfig.shutdown();
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

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

Fmi::Cache::CacheStatistics Plugin::getCacheStats() const
{
  Fmi::Cache::CacheStatistics ret;

  ret.insert(std::make_pair("Textgen::forecast_text_cache", itsForecastTextCache.statistics()));

  return ret;
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
