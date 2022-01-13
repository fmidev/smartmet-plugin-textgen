#pragma once

#include "Config.h"

#include <macgyver/Cache.h>
#include <spine/HTTP.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>
#include <textgen/DictionaryFactory.h>

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
  ~Plugin() override = default;

  const std::string& getPluginName() const override;
  int getRequiredAPIVersion() const override;
  bool queryIsFast(const SmartMet::Spine::HTTP::Request& theRequest) const override;

 protected:
  void init() override;
  void shutdown() override;
  void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse) override;

 private:
  Plugin() = delete;

  std::string query(SmartMet::Spine::Reactor& theReactor,
                    const SmartMet::Spine::HTTP::Request& theRequest,
                    SmartMet::Spine::HTTP::Response& theResponse);
  bool verifyHttpRequestParameters(SmartMet::Spine::HTTP::ParamMap& queryParameters,
                                   std::string& errorMessage);

  SmartMet::Spine::Reactor* itsReactor = nullptr;
  const std::string itsModuleName;
  Config itsConfig;
  boost::shared_ptr<TextGen::Dictionary> itsDictionary;

  struct cache_item
  {
    std::string member;
  };
  Fmi::Cache::Cache<std::string, cache_item> itsForecastTextCache;

  SmartMet::Engine::Geonames::Engine* itsGeoEngine = nullptr;

  Fmi::Cache::CacheStatistics getCacheStats() const override;
};  // class Plugin

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet
