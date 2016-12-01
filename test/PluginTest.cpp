#include "Plugin.h"
#include <spine/PluginTest.h>

using namespace std;

void prelude(SmartMet::Spine::Reactor& reactor)
{
#if 1
  auto handlers = reactor.getURIMap();
  while (handlers.find("/textgen") == handlers.end())
  {
    sleep(1);
    handlers = reactor.getURIMap();
  }
#endif

  cout << endl << "Testing textforecast plugin" << endl << "============================" << endl;
}

int main()
{
  SmartMet::Spine::Options options;
  options.configfile = "cnf/reactor.conf";
  options.quiet = true;

  SmartMet::Spine::PluginTest::test(options, prelude);

  return 0;
}
