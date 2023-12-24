## Build & Usage
	git clone --recursive https://github.com/mylogin/sitemap.git
	cd sitemap && mkdir build && cd build
	cmake -DBOOST_ROOT=/path/to/boost ..
	cmake --build . --config Release
	# edit setting.conf
	./sitemap ../setting.conf
	# press ctrl+c to exit or wait until the program ends

## Features
* Multi-thread support.
* URL filtering with regular expressions.
* Support for many HTML elements and attributes, relative URLs.
* Configurable retries count and timeouts.
* Validating HTML markup.
* Create sitemap and sitemap index files.
* Automatically creates new sitemap files if limits is reached.
* HTTPS Support.
* Detailed information logs.

## Dependencies
* Boost URL
* Boost Program_options
* Boost String Algo
* OpenSSL library for HTTPS support (set HTTPLIB_REQUIRE_OPENSSL=OFF flag to allow build if lib not found).

## Options

### [main]

#### url (required, default: empty)
First page to parse.

#### link_check (default: off)
By default, the program scans HTML documents and recursively handles each clickable link (`<a>`, `<area>` tags). Use this option to check other tags. All tags are listed in sitemap.cpp `Tags_other`.

#### subdomain (default: off)
Subdomains will be processed, otherwise only the domain from parameter **url** will be processed.

#### thread (default: 1)
Number of concurrent requests.

#### sleep (default: 0)
Number of milliseconds to wait before next request.

#### try_limit (default: 3)
Number of retries if the request fails.

#### redirect_limit (default: 5)
Limit redirect count to avoid infinite redirects.

#### url_limit (default: 0)
Limit the amount of urls the crawler should crawl.

#### bind_interface (default: empty, values: IP address, Interface name or host name)
Use a specific network interface (is not available on Windows).  
Example: `bind_interface = eth0`

#### cert_verification (default: off)
Enables server certificate verification. If neither `ca_cert_file_path` nor `ca_cert_dir_path` is defined, the default locations will be used to load trusted CA certificates. If an error occurs during the verification process, the last error is logged to the error_reply log. Disabled by default.

#### ca_cert_file_path (default: empty)
Points to a file of CA certificates in PEM format. The file can contain several CA certificates identified by
```
 -----BEGIN CERTIFICATE-----
 ... (CA certificate in base64 encoding) ...
 -----END CERTIFICATE-----
```
sequences. Before, between, and after the certificates text is allowed which can be used e.g. for descriptions of the certificates. (Taken from OpenSSL man).

#### ca_cert_dir_path (default: empty)
Points to a directory containing CA certificates in PEM format. The files each contain one CA certificate. The files are looked up by the CA subject name hash value, which must hence be available. If more than one CA certificate with the same name hash value exist, the extension must be different (e.g. 9d66eef0.0, 9d66eef0.1 etc). The search is performed in the ordering of the extension number, regardless of other properties of the certificates. Use the c_rehash utility to create the necessary links. (Taken from OpenSSL man). If `ca_cert_file_path` is defined, this option will be ignored.

### [filters]
Format: `filter = (regexp|get|ext) (exclude|include|skip) value`

#### exclude (do not visit certain url)
`filter = regexp exclude ^https?:\/\/www\.sitename\.xx\/(articles|news)\/id\d+` - not visit url if it matches regexp  
`filter = get exclude sort` - not visit url if it has a `sort` query parameter  
`filter = ext exclude png` - not visit url if it has a file and its extension is `.png`

#### include (visit certain urls and skip the rest)
`filter = regexp include ^https?:\/\/www\.sitename\.xx\/(articles|news)\/id\d+` visit url if it matches regexp, and not vizit others  
`filter = get include sort` - visit url if it has a `sort` query parameter and not visit other urls that have query params  
`filter = ext include php` - visit url if it has a file and its extension is `.php` and not visit other urls that have a file

#### skip (do not visit url but include it in the result)
`filter = regexp skip \/news\/id\d+\/?$` - not visit url if it matches regexp but add it to result (for sites with a large number of similar urls)  
`get` and `ext` same as above.

### [sitemap]

#### enabled (default: off)
Will generage sitemap XML from clickable links.

#### dir (required when `sitemap.enabled = on`)
Directory where the sitemap and sitemap index files will be saved.

#### file_name (default: sitemap)
Sitemap file name. File number will be added at the end of the file name as there may be many files in case of exceeding the limits.  
Output files: *sitemap_all_main1.xml, sitemap_all_main2.xml...*

#### index_file_name (default: empty)
Sitemapindex file name. Use this option to create a Sitemap index file containing a list of all generated Sitemap files. Full path of each Sitemap file will consist of parameter `url` + parameter `xml_name`. If not set, Sitemapindex will not be generated.  
Output file: *sitemap_index.xml*

#### xml_filemb_lim (default: 1)
Limit the size of xml file in megabyte. If exceeded, a new file is created.

#### entry_lim (default: 1000000)
Limit the amount of `<url>` tags in file. If exceeded, a new file is created.

#### xml_tag
Adds an additional tag to each `<url>` tag.  
Format: `xml_tag = tag_name tag_value [url]`  
Examples:  
`xml_tag = priority 0.3 default` default value
`xml_tag = priority 0.5 ^https?:\/\/www\.sitename\.xx\/about\/` value for specific url  
`xml_tag = priority 0.4 ^https?:\/\/www\.sitename\.xx\/contacts\/` value for specific url

### [log]
Log files are written in CSV or XML formats, or displayed if the `type = console` is set.

#### type (default: console, values: xml, csv, console)
Log formats.  
Example: `type = console,csv,xml`

#### dir (required when `log.type` xml or csv)
Directory where the log files will be saved.

#### rewrite (default: off)
Overwrite log files instead of creating new ones.

#### max_log_cnt (default: 100)
If `rewrite = off`, sets the maximum number of log files.

#### csv_separator (default: ,)
Delimeter in CSV files.

### Log files
If an option from the list below is set, a corresponding log file will be created. The file name is the same as the parameter name without the log_ prefix.

Columns description:
| Column | Description |
|-|-|
| id | Identificator of page |
| found | URL found on the page as is |
| url | Handled URL with all parts (scheme, host etc.) |
| id_parent | ID page where the url was found |
| parent | Handled URL of the page where the URL was found |
| time | HTTP response time |
| is_html | Is page content is html |
| try_cnt | Number of retries if the request fails |
| cnt | Number of similar URLs found during the crawl |
| charset | Charset of the page |
| thread | Thread id |
| msg | Errors, exceptions and info messages |

#### log_error_reply (default: off)
Urls with status_code != Success and status_code != Redirection, try limits, certificate errors.  
Columns (csv, xml): `msg,url,id_parent`  
Columns (console): `msg,url,parent`

#### log_redirect (default: off)
Urls that returned redirect status.  
Columns (csv, xml): `url,id_parent`  
Columns (console): `url,parent`

#### log_bad_html (default: off)
HTML markup errors. Currently only checking for unclosed tags is supported.  
Columns (csv, xml): `url,id`  
Columns (console): `url,url`

#### log_bad_url (default: off)
Urls with bad format.  
Columns (csv, xml): `found,id_parent`  
Columns (console): `found,parent`

#### log_ignored_url (default: off)
Urls that did not pass custom filters.  
Columns (csv, xml): `found,id_parent`  
Columns (console): `found,parent`

#### log_skipped_url (default: off)
Urls whose content is not requested.  
Columns (csv, xml): `url,id_parent`  
Columns (console): `url,parent`

#### log_info (default: off)
Verbose log.  
Columns (csv, xml): `id,parent,time,try_cnt,cnt,is_html,found,url,charset,msg`  
Columns (console): `thread,time,url,parent`

#### log_other (default: off)
Errors, exceptions and info messages.  
Columns: `msg`