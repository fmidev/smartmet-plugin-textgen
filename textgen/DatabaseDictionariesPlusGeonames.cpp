// ======================================================================
/*!
 * \file
 * \brief Implementation of class TextGen::DatabaseDictionariesPlusGeonames
 */
// ======================================================================
/*!
 * \class TextGen::DatabaseDictionariesPlusGeonames
 *
 * \brief Provides dictionary services
 *
 * The responsibility of the DatabaseDictionariesPlusGeonames class is to provide natural
 * language text for the given keyword. Inserting new keyword-text pairs.
 *
 * The dictionary has an initialization method, which fetches the specified
 * language from the DatabasePlusGeonames server.
 *
 * Sample usage:
 * \code
 * using namespace TextGen;
 *
 * DatabaseDictionariesPlusGeonames finnish;
 * finnish.init("fi");
 *
 * cout << finnish.find("good morning") << endl;
 * cout << finnish.find("good night") << endl;
 *
 * if(finnish.contains("good night"))
 *    cout << finnish.find("good night") << endl;
 * \endcode
 *
 * Note that find throws if the given keyword does not exist.
 *
 * The database address, table name, user name and password
 * are all specified externally in fmi.conf used by newbase
 * NFmiSettings class.
 *
 * The dictionary can be initialized multiple times. Each init
 * erases the language initialized earlier.
 */
// ----------------------------------------------------------------------

#include "DatabaseDictionariesPlusGeonames.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <calculator/Settings.h>
#include <calculator/TextGenError.h>
#include <engines/geonames/Engine.h>
#include <macgyver/Exception.h>
#include <mysql++/mysql++.h>
#include <spine/Reactor.h>
#include <cassert>
#include <map>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace boost;

namespace SmartMet
{
namespace Plugin
{
namespace Textgen
{
// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple
 */
// ----------------------------------------------------------------------

class DatabaseDictionariesPlusGeonames::Impl
{
 public:
  Impl() : itsInitialized(false), itsGeoEngine(nullptr) {}
  bool itsInitialized;
  SmartMet::Engine::Geonames::Engine* itsGeoEngine;
  std::string itsEmptyString;

};  // class Impl

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 *
 * The constructor does nothing special.
 */
// ----------------------------------------------------------------------

DatabaseDictionariesPlusGeonames::DatabaseDictionariesPlusGeonames(const std::string& theDatabaseId)
    : DatabaseDictionaries(theDatabaseId), itsImpl(new Impl())
{
}

void DatabaseDictionariesPlusGeonames::geoinit(void* theGeoengine)
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

bool DatabaseDictionariesPlusGeonames::geocontains(const std::string& theKey) const
{
  std::string key(theKey);
  trim(key);
  if (key.empty())
    return false;

  try
  {
    auto locPtr = itsImpl->itsGeoEngine->nameSearch(theKey, language());

    if (locPtr->name.empty())
      return false;

    return true;
  }
  catch (...)
  {
    return false;
  }
}

bool DatabaseDictionariesPlusGeonames::geocontains(const double& theLongitude,
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

std::string DatabaseDictionariesPlusGeonames::geofind(const std::string& theKey) const
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

std::string DatabaseDictionariesPlusGeonames::geofind(double theLongitude,
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
