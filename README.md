## Build & Usage

	git clone git@github.com:mylogin/sitemap.git
	cd sitemap
	make
	./sitemap setting.txt

## Dependencies

All dependencies are included in the folder 'deps'

<https://github.com/mylogin/avhtml>

<https://github.com/mylogin/CxxUrl>

<https://github.com/yhirose/cpp-httplib>

## Options

### Base options

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

If set, subdomains will be processed, otherwise only the domain from parameter **url** will be processed.

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

If set, writes to standard output information about what the program does.

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

### Logs

All log files are written in the CSV format.

Columns description:

| Column | Description |
|-|-|
| found | URL found on the page as is |
| url | Handled URL with all parts (scheme, host etc.) |
| status_code | HTTP response status code |
| parent | Page on which url was found |
| url_chain | URL chain in case of redirect |
| reason | Reason of error |
| time | HTTP response time |
| is_html | Is page content is html |
| try_cnt | Number of retries if the request fails |
| redirect_cnt | Number of redirects for a given URL |
| charset | Charset of the page |

#### log_error_reply

Urls with status_code != Succees and status_code != Redirection.

Columns: `status_code,url,parent`

Output file: *error_reply.log*

#### log_redirect

Urls that returned redirect status.

Columns: `url_chain,parent`

Output file: *redirect.log*

#### log_parse_url

Urls with bad format.

Columns: `reason,found,parent`

Output file: *parse_url.log*

#### log_ignored_url

Urls that did not pass custom filters.

Columns: `found,parent`

Output file: *ignored_url.log*

#### log_info

Verbose log.

Columns: `time,is_html,try_cnt,redirect_cnt,charset,found,url_chain,parent`

Output file: *info.log*

#### log_other

Other errors and exceptions.

Columns: `reason`

Output file: *other.log*

### Sitemap XML

#### xml_name

XML file name. File number will be added at the end of the file name as there may be many files in case of exceeding the limits. If this option is not set, XML will not be generated.

Example: `xml_name sitemap_all_main`

Output files: *sitemap_all_main1.xml, sitemap_all_main2.xml...*

#### xml_entry_limit (default: 1000000)

Limit the amount of `<url>` tags in file. If exceeded, a new file is created.

Example: `xml_entry_lim 1000000`

#### xml_filemb_limit (default: 1)

Limit the size of xml file in megabyte. If exceeded, a new file is created.

Example: `xml_filemb_lim 10`

#### xml_tag

Adds an additional tag to each `<url>` tag

Format: `xml_tag tag_name tag_value [url]`

Examples:

`xml_tag priority 0.3` default value, must be first

`xml_tag priority 0.5 ^https?:\/\/www\.sitename\.xx\/about\/` value for specific url

`xml_tag priority 0.4 ^https?:\/\/www\.sitename\.xx\/contacts\/` value for specific url