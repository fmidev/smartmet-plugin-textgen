// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::FileDictionaryPlusGeonames
 */
// ======================================================================

#pragma once

#include <memory>
#include <textgen/FileDictionary.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class FileDictionaryPlusGeonames : public TextGen::FileDictionary
{
 public:
  ~FileDictionaryPlusGeonames() override = default;
  FileDictionaryPlusGeonames();
#ifdef NO_COMPILER_OPTIMIZE
  FileDictionaryPlusGeonames(const FileDictionaryPlusGeonames& theDict);
  FileDictionaryPlusGeonames& operator=(const FileDictionaryPlusGeonames& theDict);
#endif

  void geoinit(void* theGeoengine) override;
  bool geocontains(const std::string& theKey) const override;
  bool geocontains(const double& theLongitude,
                           const double& theLatitude,
                           const double& theMaxDistance) const override;
  std::string geofind(const std::string& theKey) const override;
  std::string geofind(double theLongitude, double theLatitude, double theMaxDistance) const override;

 private:
  class Impl;
  std::shared_ptr<Impl> itsImpl;
};  // class FileDictionaryPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet
