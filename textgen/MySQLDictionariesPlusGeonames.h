// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::MySQLDictionariesPlusGeonames
 */
// ======================================================================

#ifndef TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H
#define TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H

#include <boost/shared_ptr.hpp>
#include <textgen/MySQLDictionaries.h>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class MySQLDictionariesPlusGeonames : public TextGen::MySQLDictionaries
{
 public:
  virtual ~MySQLDictionariesPlusGeonames() = default;
  MySQLDictionariesPlusGeonames();
#ifdef NO_COMPILER_OPTIMIZE
  MySQLDictionariesPlusGeonames(const MySQLDictionariesPlusGeonames& theDict);
  MySQLDictionariesPlusGeonames& operator=(const MySQLDictionariesPlusGeonames& theDict);
#endif

  virtual void geoinit(void* theReactor);
  virtual bool geocontains(const std::string& theKey) const;
  virtual bool geocontains(const double& theLongitude,
                           const double& theLatitude,
                           const double& theMaxDistance) const;
  virtual const std::string& geofind(const std::string& theKey) const;
  virtual const std::string& geofind(const double& theLongitude,
                                     const double& theLatitude,
                                     const double& theMaxDistance) const;

 private:
  class Impl;
  boost::shared_ptr<Impl> itsImpl;
};  // class MySQLDictionariesPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  // TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H

// ======================================================================
