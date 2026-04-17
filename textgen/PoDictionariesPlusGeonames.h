// ======================================================================
/*!
 * \file
 * \brief Interface of class TextGen::PoDictionariesPlusGeonames
 */
// ======================================================================

#ifndef TEXTGEN_PODICTIONARIESPLUSGEONAMES_H
#define TEXTGEN_PODICTIONARIESPLUSGEONAMES_H

#include <textgen/PoDictionaries.h>
#include <memory>
#include <string>

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class PoDictionariesPlusGeonames : public TextGen::PoDictionaries
{
 public:
  ~PoDictionariesPlusGeonames() override = default;
  PoDictionariesPlusGeonames();

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
};  // class PoDictionariesPlusGeonames

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

#endif  // TEXTGEN_PODICTIONARIESPLUSGEONAMES_H

// ======================================================================
