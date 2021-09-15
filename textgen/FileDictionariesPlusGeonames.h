// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::FileDictionariesPlusGeonames
 */
// ======================================================================

#ifndef TEXTGEN_FILEDICTIONARIESPLUSGEONAMES_H
#define TEXTGEN_FILEDICTIONARIESPLUSGEONAMES_H

#include <boost/shared_ptr.hpp>
#include <textgen/FileDictionaries.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class FileDictionariesPlusGeonames : public TextGen::FileDictionaries
{
 public:
  virtual ~FileDictionariesPlusGeonames() = default;
  FileDictionariesPlusGeonames();
#ifdef NO_COMPILER_OPTIMIZE
  FileDictionariesPlusGeonames(const FileDictionariesPlusGeonames& theDict);
  FileDictionariesPlusGeonames& operator=(const FileDictionariesPlusGeonames& theDict);
#endif

  virtual void geoinit(void* theGeoengine);
  virtual bool geocontains(const std::string& theKey) const;
  virtual bool geocontains(const double& theLongitude,
                           const double& theLatitude,
                           const double& theMaxDistance) const;
  virtual std::string geofind(const std::string& theKey) const;
  virtual std::string geofind(double theLongitude, double theLatitude, double theMaxDistance) const;

 private:
  class Impl;
  boost::shared_ptr<Impl> itsImpl;
};  // class FileDictionariesPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  // TEXTGEN_FILEDICTIONARIESPLUSGEONAMES_H

// ======================================================================
