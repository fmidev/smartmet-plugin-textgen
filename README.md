# SmartMet Server

[SmartMet Server](https://github.com/fmidev/smartmet-server) is a data and product server for MetOcean data. It
provides a high capacity and high availability data and product server
for MetOcean data. The server is written in C++, since 2008 it has
been in operational use by the Finnish Meteorological Institute FMI.

# Table Of Content
### 1. General
### 2. Example
### 3. URL parameters
### 4. Configuration file
<br>

# General

This is a User's Guide for textgenplugin. Textgenplugin is an interface to automatic text generator. The actual functionality of automatic text generator is implemented in [smartmet-library-textgen](https://github.com/fmidev/smartmet-library-textgen).

# Example

Request:

`[host]textgen?language=en&product=iltaan_asti&forecasttime=201712030900&area=Uusimaa`

Response:
### Weather report for Uusimaa on Sunday 9 o'clock
#### Expected weather until Sunday evening:
Rain. Rain can be heavy in the afternoon. Temperature is round about 5 degrees. Fresh south-westerly wind.

# URL-parameters

parameter | default value | description | example | valid values
------------ | ------------- | ------------- | ------------- | -------------
product | n/a | Name of the configuration to load | n/a |   The available names are read from settings file plugins/textgen.conf. Each referes to a separate configuration file in plugins/texgen/ -directory
language | n/a | Define the generated text's language | en/fi/sv | Depends what is configured in database  
area | n/a | location/area name | area=Etelä-Savo | any valid location/area name
forcasttime | current time | time of forecast, format is YYYYMMDDHH[SS] | forecasttime=201712050900 | any valid date
formatter | defined in configuration file | output format | formatter=plainlines | plainlines, html, css
geoid | n/a | geoid of location/area | geoid=647852 | any valid geoid
language | defined in configuration file | language of output documant | language=en | fi,sv,en
lonlat | n/a | longitude and latitude separated by comma | lonlat=24.9616,60.2042 | any valid longitude/latitude pair
postgis | defined in configuration file | parameters to access PostGIS database | see below

## Mandatory parameters

In every query there must be area, lonlat or geoid parameter defined. All the other parameters must have default value defined in configuration file.

## area

area-parameter defines the area name. The name can be name of the county/city or some other geographical area, that is stored in PostGIS-database. The name must be in the same format as in PostGIS databse.

Example: `area=Uusimaa,Kymenlaakso,Helsinki,Kotka`

## forecasttime

forecasttime-parameter defines time of forecast. Default value is current time.

## formatter

formatter-parameter defines the output format

Valid values:

* html
* css
* plainlines

Example: `formatter=plainlines`

## geoid

geoid-parameter is used to fetch location info from geonames database. If the location has feature code starting with "ADM" the area definition is fetched from PostGIS-database by using the location name as a key. Forecast text is then generated for that area. If the feature code is something else (for example PPLC) the longitude-latitude coordinate is used to get forecast for the location.

Example: `geoid=647852,658226,658225`

language

language-parameter defines the language of the ouput document.

Valid values:

* fi
* sv
* en

Example: `language=en`

## lonlat

lonlat-parameter defines a coordinate point and optionally radius in kilometers. Radius is used both in defining the weather forecast area around the coordinate point and defining the area where name for the coordinate point is searched in geonames database.

Comma is used as a separator. Several longitude-latitude pairs can be given at once.

Example: `lonlat=24.9616,60.2042,25.50,61.35:10`

## postgis

postgis-parameter defines the parameters, that are needed to fetch area definitions (polygons, points) from PostGIS-database. Default PostGIS parameters are defined in configuration file, but you can override either some or all of them in HTTP request. You can define several postgis-parameters each of them can contain the following subparameters:

* host
* dbname
* schema
* table
* field
* (username)
* (password)

Even if username and password can be passed as a parameter it is not recommended that they are exposed in the query, but written in the configuration file instead.

In the following example we fetch information from tree different tables.

Example: `postgis=schema=esri,table=europe_cities_eureffin,field=name&postgis=schema=fminames,table=kunnat,field=kuntanimi&postgis=schema=fminames,table=maakunnat,field=maaknimi`

## product

Product refers to configuration file. Different products have different configuration file. product-parameter defines a identifier of the product specific configuration file. In textgenplugin main configuration file the identifiers and actual configuration files are matched together.

# Configuration file

## General

There are two kind of configuration files. Main configuration and product specific configuration files.

## Main configuration file

The main configuration file contains definitions for url-string, cache size of forecast texts and product specific configuration files.

There must be at least one configuration file defined in product_config section. Default configuration is used if no product is defined in the query.

Example:

`url             = "/textgen";`<br>
`forecast_text_cache_size    = 30;`<br><br>
`product_config:`<br>
`{`<br>
 &emsp;`default = "/home/reponen/work/brainstorm/plugins/textgenplugin/cnf/default.conf";`<br>
 &emsp;`iltaan_asti = "/home/reponen/work/brainstorm/plugins/textgenplugin/cnf/iltaan_asti.conf";`<br>
 &emsp;`ilta_ja_huominen = "/home/reponen/work/brainstorm/plugins/textgenplugin/cnf/ilta_ja_huominen.conf";`<br>
 &emsp;`yletv = "/home/reponen/work/brainstorm/plugins/textgenplugin/cnf/yletv.conf";`<br>
 &emsp;`yletv2d = "/home/reponen/work/brainstorm/plugins/textgenplugin/cnf/yletv2d.conf";`<br>
 &emsp;`yletv_long = "/home/reponen/work/brainstorm/plugins/textgenplugin/cnf/yletv_long.conf";`<br>
`};`<br>

## Product specific configuration files

The following sections are included in product specific configuration files.

### misc

Contains miscellaneous parameters:

* timeformat
* language
* locale
* dictionary
* formatter

Example:

`misc:`<br>
`{`<br>
 &emsp;`timeformat          = "iso";`<br>
 &emsp;`language            = "fi";`<br>
 &emsp;`locale              = "fi_FI.UTF-8";`<br>
 &emsp;`dictionary          = "multimysqlplusgeonames";`<br>
 &emsp;`formatter           = "html";`<br>
`};`<br>

### area

Contains area-related definitions. In this section we can define default timezone and area specific timezones. If no timezone is defined 'Europe/Helsinki' is used. Additionally we can define 'marine'- and 'mountain'-areas here. 'marine'-configuration is used when forecasts are produced for sea areas.

Example:
`area:`<br>
`{`<br>
 &emsp;`timezone:`<br>
 &emsp;`{`<br>
 &emsp; &emsp;`dublin  = "Europe/Dublin";`<br>
 &emsp; &emsp;`paris  = "Europe/Paris";`<br>
 &emsp;`};`<br>
 <br>
 &emsp;`marine:`<br>
 &emsp;`{`<br>
 &emsp; &emsp;`default = false;`<br>
 &emsp; &emsp;`B1 = true;`<br>
 &emsp; &emsp;`B2 = true;`<br>
 &emsp; &emsp;`B3 = true;`<br>
 &emsp;`};`<br>
 <br>
 &emsp;`mountain:`<br>
 &emsp;`{`<br>
 &emsp; &emsp;`default = false;`<br>
 &emsp;`};`<br>
`};`<br>

### mask

Coast- and land mask files are defined in this section. Mask file can reside either on disk file or PostGIS database.

Example:

`mask:`<br>
`{`<br>
 &emsp;`# masks fetched from svg-file`<br>
 &emsp;`coast = "/smartmet/share/textgendata/maps/merialueet/rannikko.svg:10";`<br>
 &emsp;`# masks fetched from PostGIS database`<br>
 &emsp;`land = "Finland";`<br>
`};`<br>
<br>
