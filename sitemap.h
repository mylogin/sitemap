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
	static bool file_exists(const std::string&);
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
	bool cur_tag_unclosed = false;
	bool last_operation_was_start_el = false;
	int indent_level = 0;
	void close_tag_if_open();
};

class CSV_Writer {
public:
	CSV_Writer(std::ostream& stream, std::string);
	CSV_Writer& add(std::string);
	CSV_Writer& add(const char *str);
	CSV_Writer& add(char *str);
	template<typename T> CSV_Writer& add(T str);
	template<typename T> CSV_Writer& operator<<(const T&);
	CSV_Writer& row();
private:
	std::ostream& output;
	std::string seperator = ",";
	bool line_empty = true;
};

class Main;

class Log {
public:
	enum Field: int {id, found, url, parent, time, is_html, try_cnt, charset, error, thread};
	virtual void write(const std::vector<std::string>&) {}
	Log(Main*, const std::string&, const std::string&, const std::vector<Field>&);
	~Log();
protected:
	Main* main;
	std::ofstream file;
	std::string file_name;
	const std::vector<Field> fields;
	const std::vector<std::string> fields_all{"id", "found", "url", "parent", "time", "is_html", "try_cnt", "charset", "error", "thread"};
};

class Console_Log: public Log {
public:
	Console_Log(Main*, const std::string&, const std::vector<Field>&);
	void write(const std::vector<std::string>&);
};

class CSV_Log: public Log {
public:
	CSV_Log(Main*, const std::string&, const std::vector<Field>&);
	void write(const std::vector<std::string>&);
private:
	CSV_Writer writer;
};

class XML_Log: public Log {
public:
	XML_Log(Main*, const std::string&, const std::vector<Field>&);
	~XML_Log();
	void write(const std::vector<std::string>&);
private:
	XML_writer writer;
};

class LogWrap {
public:
	void init(Main*, const std::string&, const std::vector<Log::Field>&);
	operator bool() const {
		return enabled;
	}
	Log* operator->() const {
		return log.get();
	}
private:
	std::unique_ptr<Log> log;
	bool enabled = false;
};

struct Xml_tag {
	std::vector<std::pair<std::string, std::string>> regexp;
	std::string def;
};

enum class url_handle_t {query, query_parse, none};

struct Url_struct {
	std::string found;
	std::string resolved;
	std::string normalize;
	std::string charset;
	std::string path;
	std::string host;
	std::string base_href;
	bool is_html = false;
	int id = 0;
	int parent = 0;
	double time = 0;
	int try_cnt = 0;
	bool ssl = false;
	size_t redirect_cnt = 0;
	url_handle_t handle = url_handle_t::query;
	std::string error;
};

struct Filter {
	int type;
	int dir;
	std::string val;
	std::regex reg;
	enum {type_regexp, type_get, type_ext};
	enum {exclude, include, skip};
};

class Thread;

class Main {
public:
	// setting
	std::string dir;
	std::string param_url;
	std::string xml_name = "sitemap";
	std::string xml_index_name;
	std::string csv_separator = ",";
	std::string type_log = "csv";
	bool param_log_redirect = false;
	bool param_log_error_reply = false;
	bool param_log_ignored_url = false;
	bool param_log_skipped_url = false;
	bool param_log_bad_url = false;
	bool param_log_other = false;
	bool param_log_info = false;
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
	std::vector<Filter> param_filter;
	int max_log_cnt = 100;
	bool rewrite_log = false;

	bool running = true;
	std::multimap<std::string, Xml_tag> param_xml_tag;
	Uri::Uri uri;
	std::condition_variable cond;
	std::mutex mutex;
	std::mutex mutex_log;
	int thread_work = 0;
	std::map<std::string, std::unique_ptr<Url_struct>> url_all;
	std::queue<Url_struct*> url_queue;
	bool url_lim_reached = false;
	LogWrap log_redirect;
	LogWrap log_error_reply;
	LogWrap log_ignored_url;
	LogWrap log_skipped_url;
	LogWrap log_bad_url;
	LogWrap log_other;
	LogWrap log_info_console;
	LogWrap log_info_file;
	void import_param(const std::string&);
	void start();
	void finished();
	bool handle_url(Url_struct*, bool filter = true);
	bool set_url(std::unique_ptr<Url_struct>&);
	void try_again(Url_struct*);
	bool get_url(Thread*);
	std::string uri_normalize(const Uri::Uri&);
};

class Thread {
public:
	Thread(int id, Main* main) : id(id), main(main) {}
	bool suspend = false;
	int id;
	Main* main;
	Url_struct* m_url;
	html::parser p;
	void load();
	void http_finished();
	void set_url(std::unique_ptr<Url_struct>&);
	void start();
	void join();
	std::shared_ptr<httplib::Client> cli;
	std::shared_ptr<httplib::Result> result;
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