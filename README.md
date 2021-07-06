## Build & Usage
	git clone --recursive https://github.com/mylogin/sitemap.git
	cd sitemap
	make
	# edit setting.txt
	./sitemap setting.txt
	# type `q` to quit or wait until the program ends

## Dependencies
All dependencies are placed in the 'deps' folder as submodules.  
[cpp-httplib](https://github.com/yhirose/cpp-httplib), [avhtml](https://github.com/mylogin/avhtml), [CxxUrl](https://github.com/mylogin/CxxUrl)

## HTTPS Support
HTTPS support is implemented using the OpenSSL library and is enabled by default. To use HTTPS add/remove `CPPHTTPLIB_OPENSSL_SUPPORT` macro from sitemap.h and run `make`. `libcrypto`, `libssl` (`libcrypt32`, `libcryptui` on Windows) should be available. See HTTPS options below.

## Options

### Base options

#### link_check
By default, the program scans HTML documents and recursively handles each clickable link (`<a>`, `<area>` tags). Use this option to check other tags. All tags are listed in sitemap.h (`Tags_main` and` Tags_other`).  
Example: `link_check`

#### sitemap
Will generage sitemap XML from clickable links. See [Sitemap XML options](#sitemap-xml).  
Example: `sitemap`

#### url (required)
First page to parse.  
Example: `url https://www.sitename.xx/`

#### dir (required)
Directory where the results will be saved.  
Example: `dir /var/www/sitename/sitemap`

#### thread (default: 1)
Number of concurrent requests.  
Example: `thread 3`

#### sleep (default: 0)
Number of seconds to wait before next request.  
Example: `sleep 1`

#### subdomain
Subdomains will be processed, otherwise only the domain from parameter **url** will be processed.  
Example: `subdomain`

#### redirect_limit (default: 5)
Limit redirect count to avoid infinite redirects.  
Example: `redirect_limit 3`

#### url_limit
Limit the amount of urls the crawler should crawl.  
Example: `url_limit 10000`

#### try_limit (default: 3)
Number of retries if the request fails.  
Example: `try_limit 5`

#### cell_delim (default: ,)
Delimeter in log files.  
Example: `cell_delim ;`

#### in_cell_delim (default: |)
Delimeter in cells of log files.  
Example: `in_cell_delim ~`

#### debug
Will write to standard output information about what the program does.  
Example: `debug`

### Filters
You can tell the crawler not to visit certain urls by using the filter options.  
Format: `filter (regexp|get|ext) (include|exclude) value`

Example 1:  
`filter ext include php` skip url if it has a file and its extension is not php  
`filter get exclude sort` skip url if it has a query parameter sort  
`filter regexp exclude ^https?:\/\/www\.sitename\.xx\/(articles|news)\/id\d+\/` skip url if it matches regexp

Example 2:  
`filter regexp include ^https?:\/\/www\.sitename\.xx\/news\/` skip url if it does not match regexp  
`filter ext exclude png` skip url if it has a file and its extension is png  
`filter ext exclude jpg` skip url if it has a file and its extension is jpg

### Sitemap XML

#### xml_name (default: sitemap)
Sitemap file name. File number will be added at the end of the file name as there may be many files in case of exceeding the limits.  
Example: `xml_name sitemap_all_main`  
Output files: *sitemap_all_main1.xml, sitemap_all_main2.xml...*

#### xml_index_name
Sitemapindex file name. Use this option to create a Sitemap index file containing a list of all generated Sitemap files. Full path of each Sitemap file will consist of parameter `url` + parameter `xml_name`. If not set, Sitemapindex will not be generated.  
Example: `xml_index_name sitemap_index`  
Output file: *sitemap_index.xml*

#### xml_entry_limit (default: 1000000)
Limit the amount of `<url>` tags in file. If exceeded, a new file is created.  
Example: `xml_entry_lim 1000000`

#### xml_filemb_limit (default: 1)
Limit the size of xml file in megabyte. If exceeded, a new file is created.  
Example: `xml_filemb_lim 10`

#### xml_tag
Adds an additional tag to each `<url>` tag.  
Format: `xml_tag tag_name tag_value [url]`  
Examples:  
`xml_tag priority 0.3` default value, must be first  
`xml_tag priority 0.5 ^https?:\/\/www\.sitename\.xx\/about\/` value for specific url  
`xml_tag priority 0.4 ^https?:\/\/www\.sitename\.xx\/contacts\/` value for specific url

### Logs
All log files are written in CSV format, the file name is the same as the parameter name without the log_ prefix.

Columns description:
| Column | Description |
|-|-|
| found | URL found on the page as is |
| url | Handled URL with all parts (scheme, host etc.) or URL chain in case of redirects |
| parent | Page on which url was found |
| reason | Reason of error |
| time | HTTP response time |
| is_html | Is page content is html |
| try_cnt | Number of retries if the request fails |
| redirect_cnt | Number of redirects for a given URL |
| charset | Charset of the page |

#### log_error_reply
Urls with status_code != Success and status_code != Redirection, certificate errors.  
Columns: `reason,url,parent`

#### log_redirect
Urls that returned redirect status.  
Columns: `url,parent`

#### log_parse_url
Urls with bad format.  
Columns: `found,parent`

#### log_ignored_url
Urls that did not pass custom filters.  
Columns: `found,parent`

#### log_info
Verbose log.  
Columns: `time,is_html,try_cnt,redirect_cnt,charset,found,url,parent`

#### log_other
Other errors and exceptions.
Columns: `reason`

### HTTPS options

#### cert_verification
Enables server certificate verification. If neither `ca_cert_file_path` nor `ca_cert_dir_path` is defined, the default locations will be used to load trusted CA certificates. If an error occurs during the verification process, the last error is logged to the error_reply log. Disabled by default.

#### ca_cert_file_path
Points to a file of CA certificates in PEM format. The file can contain several CA certificates identified by
```
 -----BEGIN CERTIFICATE-----
 ... (CA certificate in base64 encoding) ...
 -----END CERTIFICATE-----
```
sequences. Before, between, and after the certificates text is allowed which can be used e.g. for descriptions of the certificates. (Taken from OpenSSL man).


#### ca_cert_dir_path
Points to a directory containing CA certificates in PEM format. The files each contain one CA certificate. The files are looked up by the CA subject name hash value, which must hence be available. If more than one CA certificate with the same name hash value exist, the extension must be different (e.g. 9d66eef0.0, 9d66eef0.1 etc). The search is performed in the ordering of the extension number, regardless of other properties of the certificates. Use the c_rehash utility to create the necessary links. (Taken from OpenSSL man). If `ca_cert_file_path` is defined, this option will be ignored.