// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::FileDictionaryPlusGeonames
 */
// ======================================================================

#pragma once

#include <boost/shared_ptr.hpp>
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
  virtual ~FileDictionaryPlusGeonames() = default;
  FileDictionaryPlusGeonames();
#ifdef NO_COMPILER_OPTIMIZE
  FileDictionaryPlusGeonames(const FileDictionaryPlusGeonames& theDict);
  FileDictionaryPlusGeonames& operator=(const FileDictionaryPlusGeonames& theDict);
#endif

  virtual void geoinit(void* theReactor);
  virtual bool geocontains(const std::string& theKey) const;
  virtual bool geocontains(const double& theLongitude,
                           const double& theLatitude,
                           const double& theMaxDistance) const;
  virtual std::string geofind(const std::string& theKey) const;
  virtual std::string geofind(double theLongitude, double theLatitude, double theMaxDistance) const;

 private:
  class Impl;
  boost::shared_ptr<Impl> itsImpl;
};  // class FileDictionaryPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet
