#pragma once

#include "Config.h"

#include <macgyver/Cache.h>
#include <spine/HTTP.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>

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
  virtual ~Plugin() = default;

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

  // Mutex for forecast text cache
  SmartMet::Spine::MutexType itsForecastTextCacheMutex;

  SmartMet::Engine::Geonames::Engine* itsGeoEngine = nullptr;

  Fmi::Cache::CacheStatistics getCacheStats() const;  
};  // class Plugin

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet
