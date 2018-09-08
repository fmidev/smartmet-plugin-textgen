#pragma once

#include "Config.h"

#include <calculator/WeatherArea.h>
#include <macgyver/Cache.h>
#include <textgen/Dictionary.h>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <engines/gis/GeometryStorage.h>
#include <spine/HTTP.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>
#include <spine/Thread.h>
#include <map>
#include <string>
#include <vector>

namespace SmartMet
{
namespace Engine
{
namespace Geonames
{
class Engine;
}
namespace Gis
{
class Engine;
}
}  // namespace Engine

namespace Plugin
{
namespace Textgen
{
class PluginImpl;

class Plugin : public SmartMetPlugin, private boost::noncopyable
{
 public:
  Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig);
  virtual ~Plugin();

  const std::string& getPluginName() const;
  int getRequiredAPIVersion() const;
  bool queryIsFast(const SmartMet::Spine::HTTP::Request& theRequest) const;

 protected:
  void init();
  void shutdown();
  void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse);

 private:
  Plugin();

  std::string query(SmartMet::Spine::Reactor& theReactor,
                    const SmartMet::Spine::HTTP::Request& theRequest,
                    SmartMet::Spine::HTTP::Response& theResponse);
  bool verifyHttpRequestParameters(SmartMet::Spine::HTTP::ParamMap& queryParameters,
                                   std::string& errorMessage);

  SmartMet::Spine::Reactor* itsReactor = nullptr;
  const std::string itsModuleName;
  Config itsConfig;

  struct cache_item
  {
    std::string member;
  };
  Fmi::Cache::Cache<std::string, cache_item> itsForecastTextCache;

  typedef std::map<std::string, boost::shared_ptr<TextGen::WeatherArea> > mask_map;
  // here we store masks by product
  std::map<std::string, mask_map> itsProductMasks;
  // mutex for PostGIS database
  SmartMet::Spine::MutexType itsPostGISMutex;
  // mutex for forecast text cache
  SmartMet::Spine::MutexType itsForecastTextCacheMutex;

  SmartMet::Engine::Geonames::Engine* itsGeoEngine = nullptr;
  SmartMet::Engine::Gis::Engine* itsGisEngine = nullptr;

  // Geometries and their svg-representations are stored here
  Engine::Gis::GeometryStorage itsGeometryStorage;

};  // class Plugin

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet
