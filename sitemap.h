#ifndef SITEMAP_H
#define SITEMAP_H

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <fstream>
#include <sstream>
#include <thread>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <stack>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iostream>
#include <functional>
#include <algorithm>

#include "deps/uri/include/Uri/Uri.hpp"
#include "deps/http/httplib.h"
#include "deps/parser/html.hpp"

class Timer {
public:
	Timer() : beg_(clock_::now()) {}
	void reset() {
		beg_ = clock_::now();
	}
	double elapsed() const {
		return std::chrono::duration_cast<second_>(clock_::now() - beg_).count();
	}
private:
	using clock_ = std::chrono::high_resolution_clock;
	using second_ = std::chrono::duration<double, std::ratio<1> >;
	std::chrono::time_point<clock_> beg_;
};

class Utils {
public:
	static std::vector<std::string> split(const std::string&, char, bool ignore_ws = true);
	static std::string join(const std::vector<std::string>&, char);
	static std::string str_tolower(std::string);
};

class XML_writer {
public:
	XML_writer(std::ostream& stream);
	void write_start_doc();
	void write_end_doc();
	void write_start_el(const std::string&);
	void write_end_el();
	void write_attr(const std::string&, const std::string&);
	void write_str(const std::string&, bool escape = true);
	std::string escape_str(std::string);
	void flush();
private:
	std::stack<std::string> open_tags;
	std::ostream& output;
	bool cur_tag_unclosed;
	bool last_operation_was_start_el;
	int indent_level = 0;
	void close_tag_if_open();
};

class CSV_Writer {
public:
	CSV_Writer() = default;
	CSV_Writer(std::string);
	CSV_Writer& add(std::string);
	CSV_Writer& add(const char *str);
	CSV_Writer& add(char *str);
	template<typename T> CSV_Writer& add(T str);
	template<typename T> CSV_Writer& operator<<(const T&);
	void operator+=(CSV_Writer&);
	std::string to_string();
	CSV_Writer& row();
	bool write_to_file(std::string, bool append = false);
	friend std::ostream& operator<<(std::ostream&, CSV_Writer&);
private:
	std::string seperator = ",";
	bool line_empty = true;
	std::stringstream ss;
};

struct Xml_tag {
	std::vector<std::pair<std::string, std::string>> regexp;
	std::string def;
};

struct Url_struct {
	std::string found;					// url found on page, in case of redirect first found
	std::vector<std::string> remote;	// resolved urls, all redirect chain
	std::string parent;					// taken from remote
	std::string normalize;
	std::string charset;
	std::string path;
	std::string host;
	std::string base_href;
	bool is_html = false;
	int id = 0;
	double time = 0;
	int try_cnt = 0;
	bool ssl = false;
	bool handle = false;
};

struct Filter {
	int type;
	int dir;
	std::string val;
	enum {type_regexp, type_get, type_ext};
	enum {exclude, include};
};

class Thread;

class Main {
public:
	// setting
	std::string dir;
	std::string param_url;
	std::string xml_name = "sitemap";
	std::string xml_index_name;
	std::string cell_delim = ",";
	std::string in_cell_delim = "|";
	bool log_redirect = false;
	bool log_error_reply = false;
	bool log_ignored_url = false;
	bool log_parse_url = false;
	bool log_other = false;
	bool log_info = false;
	bool param_debug = false;
	bool param_subdomain = false;
	int param_sleep = 0;
	int thread_cnt = 1;
	size_t redirect_limit = 5;
	size_t url_limit = 0;
	int xml_filemb_lim = 1;
	int xml_entry_lim = 1000000;
	int try_limit = 3;
	bool cert_verification = false;
	std::string ca_cert_file_path;
	std::string ca_cert_dir_path;
	bool link_check = false;
	bool sitemap = false;

	bool running = true;
	std::vector<Filter> param_filter;
	std::multimap<std::string, Xml_tag> param_xml_tag;
	Uri::Uri uri;
	std::condition_variable cond;
	std::mutex mutex;
	std::mutex mutex_log;
	int thread_work = 0;
	std::map<std::string, Url_struct> url_all;
	void import_param(const std::string&);
	void start();
	void finished();
	bool handle_url(Url_struct&, bool filter = true);
	bool set_url(Url_struct&);
	void redirect_url(const std::string&, const Url_struct&);
	void try_again(const std::string&);
	void update_url(const Url_struct&);
	bool get_url(Thread*);
	std::string uri_normalize(const Uri::Uri&);
	void log(const std::string&, const std::string&);
	void debug(const std::string&);
	std::queue<Url_struct> url_queue;
	bool url_lim_reached = false;
};

class Thread {
public:
	Thread(int id, Main* main) : id(id), main(main) {}
	bool suspend = false;
	int id;
	Main* main;
	Url_struct m_url;
	html::parser p;
	void load();
	void http_finished();
	bool set_url(const std::string&, bool);
	void start();
	void join();
	std::shared_ptr<httplib::Response> reply;
	std::unique_ptr<std::thread> uthread = nullptr;
};

class Handler {
public:
	static bool attr_charset(html::node&, std::string&, Thread* t = nullptr);
	static bool attr_refresh(html::node&, std::string&, Thread* t = nullptr);
	static bool attr_srcset(html::node&, std::string&, Thread* t = nullptr);
};

struct Attr {
	std::string name;
	std::function<bool(html::node&, std::string&, Thread*)> pre;
};

struct Tag {
	std::string name;
	std::vector<Attr> attr;
};

#endif // SITEMAP_H