// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::FileDictionariesPlusGeonames
 */
// ======================================================================

#ifndef TEXTGEN_FILEDICTIONARIESPLUSGEONAMES_H
#define TEXTGEN_FILEDICTIONARIESPLUSGEONAMES_H

#include <textgen/FileDictionaries.h>
#include <memory>
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
  ~FileDictionariesPlusGeonames() override = default;
  FileDictionariesPlusGeonames();
#ifdef NO_COMPILER_OPTIMIZE
  FileDictionariesPlusGeonames(const FileDictionariesPlusGeonames& theDict);
  FileDictionariesPlusGeonames& operator=(const FileDictionariesPlusGeonames& theDict);
#endif

  void geoinit(void* theGeoengine) override;
  bool geocontains(const std::string& theKey) const override;
  bool geocontains(const double& theLongitude,
                   const double& theLatitude,
                   const double& theMaxDistance) const override;
  std::string geofind(const std::string& theKey) const override;
  std::string geofind(double theLongitude,
                      double theLatitude,
                      double theMaxDistance) const override;

 private:
  class Impl;
  std::shared_ptr<Impl> itsImpl;
};  // class FileDictionariesPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  // TEXTGEN_FILEDICTIONARIESPLUSGEONAMES_H

// ======================================================================
