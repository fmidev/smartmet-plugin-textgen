// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::DatabaseDictionariesPlusGeonames
 */
// ======================================================================

#ifndef TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H
#define TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H

#include <memory>
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
  ~DatabaseDictionariesPlusGeonames() override = default;
  DatabaseDictionariesPlusGeonames(const std::string& theDatabaseId);
#ifdef NO_COMPILER_OPTIMIZE
  DatabaseDictionariesPlusGeonames(const DatabaseDictionariesPlusGeonames& theDict);
  DatabaseDictionariesPlusGeonames& operator=(const DatabaseDictionariesPlusGeonames& theDict);
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
};  // class DatabaseDictionariesPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  // TEXTGEN_MYSQLDICTIONARIESPLUSGEONAMES_H

// ======================================================================
