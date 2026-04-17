// ======================================================================
/*!
 * \file
 * \brief Implementation of class TextGen::PoDictionariesPlusGeonames
 */
// ======================================================================

#include "PoDictionariesPlusGeonames.h"
#include <boost/algorithm/string.hpp>
#include <calculator/Settings.h>
#include <engines/geonames/Engine.h>
#include <macgyver/Exception.h>
#include <spine/Reactor.h>

using namespace std;
using namespace boost;

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
class PoDictionariesPlusGeonames::Impl
{
 public:
  Impl() = default;
  bool itsInitialized{false};
  SmartMet::Engine::Geonames::Engine* itsGeoEngine{nullptr};
  std::string itsEmptyString;

};  // class Impl

PoDictionariesPlusGeonames::PoDictionariesPlusGeonames() : itsImpl(new Impl()) {}

void PoDictionariesPlusGeonames::geoinit(void* theGeoengine)
{
  try
  {
    if (theGeoengine == nullptr)
      throw Fmi::Exception(BCP, "Geonames engine unavailable");

    itsImpl->itsGeoEngine = reinterpret_cast<SmartMet::Engine::Geonames::Engine*>(theGeoengine);
    itsImpl->itsInitialized = true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool PoDictionariesPlusGeonames::geocontains(const std::string& theKey) const
{
  std::string key(theKey);
  trim(key);
  if (key.empty())
    return false;

  try
  {
    auto locPtr = itsImpl->itsGeoEngine->nameSearch(theKey, language());

    return !locPtr->name.empty();
  }
  catch (...)
  {
    return false;
  }
}

bool PoDictionariesPlusGeonames::geocontains(const double& theLongitude,
                                             const double& theLatitude,
                                             const double& theMaxDistance) const
{
  try
  {
    auto locPtr =
        itsImpl->itsGeoEngine->lonlatSearch(theLongitude, theLatitude, language(), theMaxDistance);

    if (locPtr->geoid == 0)
      return false;

    if (locPtr->name.empty())
      return false;

    return true;
  }
  catch (...)
  {
    return false;
  }
}

std::string PoDictionariesPlusGeonames::geofind(const std::string& theKey) const
{
  try
  {
    auto locPtr = itsImpl->itsGeoEngine->nameSearch(theKey, language());
    return locPtr->name;
  }
  catch (...)
  {
    return itsImpl->itsEmptyString;
  }
}

std::string PoDictionariesPlusGeonames::geofind(double theLongitude,
                                                double theLatitude,
                                                double theMaxDistance) const
{
  try
  {
    auto locPtr =
        itsImpl->itsGeoEngine->lonlatSearch(theLongitude, theLatitude, language(), theMaxDistance);

    if (locPtr->geoid == 0)
      return itsImpl->itsEmptyString;

    return locPtr->name;
  }
  catch (...)
  {
    return itsImpl->itsEmptyString;
  }
}

}  // namespace Textgen
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
