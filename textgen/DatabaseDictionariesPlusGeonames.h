// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::DatabaseDictionariesPlusGeonames
 */
// ======================================================================

#ifndef TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H
#define TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H

#include <boost/shared_ptr.hpp>
#include <textgen/DatabaseDictionaries.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class DatabaseDictionariesPlusGeonames : public TextGen::DatabaseDictionaries
{
 public:
  virtual ~DatabaseDictionariesPlusGeonames() = default;
  DatabaseDictionariesPlusGeonames(const std::string& theDatabaseId);
#ifdef NO_COMPILER_OPTIMIZE
  DatabaseDictionariesPlusGeonames(const DatabaseDictionariesPlusGeonames& theDict);
  DatabaseDictionariesPlusGeonames& operator=(const DatabaseDictionariesPlusGeonames& theDict);
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
};  // class DatabaseDictionariesPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  // TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H

// ======================================================================
