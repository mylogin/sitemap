#include "sitemap.h"

Main main_obj;

std::exception_ptr exc_ptr = nullptr;

std::vector<std::string> Tags_main{"a", "area"};

std::vector<Tag> Tags_other{
	{"meta", {{"content", Handler::attr_charset}}},
	{"meta", {{"content", Handler::attr_refresh}}},
	{"a", {{"ping"}}},
	{"area", {{"ping"}}},
	{"audio", {{"src"}}},
	{"body", {{"background"}}},
	{"frame", {{"src"}, {"longdesc"}}},
	{"iframe", {{"src"}, {"longdesc"}}},
	{"img", {{"src"}, {"srcset", Handler::attr_srcset}, {"longdesc"}}},
	{"input", {{"src"}, {"formaction"}}},
	{"source", {{"src"}, {"srcset", Handler::attr_srcset}}},
	{"table", {{"background"}}},
	{"tbody", {{"background"}}},
	{"td", {{"background"}}},
	{"tfoot", {{"background"}}},
	{"th", {{"background"}}},
	{"thead", {{"background"}}},
	{"tr", {{"background"}}},
	{"track", {{"src"}}},
	{"video", {{"poster"}, {"src"}}},
	{"button", {{"formaction"}}},
	{"form", {{"action"}}},
	{"link", {{"href"}}},
	{"script", {{"src"}}},
	{"blockquote", {{"cite"}}},
	{"del", {{"cite"}}},
	{"head", {{"profile"}}},
	{"html", {{"manifest"}}},
	{"ins", {{"cite"}}},
	{"q", {{"cite"}}},
	{"*", {{"itemtype"}}}
};

std::string Timer::elapsed_str(int p) const {
	std::stringstream ret;
	const auto diff = clock_::now() - beg_;
	const auto h = std::chrono::duration_cast<hours_>(diff);
	const auto m = std::chrono::duration_cast<minutes_>(diff - h);
	const second_ s = diff - h - m;
	ret << std::fixed << std::setprecision(p);
	if(h != hours_::zero()) {
		ret << h.count() << " h, ";
	}
	if(m != minutes_::zero()) {
		ret << m.count() << " min, ";
	}
	ret << s.count() << " s";
	return ret.str();
}

XML_writer::XML_writer(std::ostream& stream) : output(stream) {}

void XML_writer::write_start_doc() {
	if(!this->open_tags.empty()) {
		throw std::runtime_error("Attempt to write start document in already started document.");
	}
	this->output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
}

void XML_writer::write_end_doc() {
	while(!this->open_tags.empty()) {
		this->write_end_el();
	}
}

void XML_writer::write_start_el(const std::string& name) {
	close_tag_if_open();
	this->output << "\n" << std::string(this->indent_level++, '	') << "<" << name;
	this->open_tags.push(name);
	this->cur_tag_unclosed = true;
	this->last_operation_was_start_el = true;
}

void XML_writer::write_end_el() {
	if(this->open_tags.empty()) {
		throw std::runtime_error("Attempt to close tag while no tags are currently opened.");
	}
	close_tag_if_open();
	this->indent_level--;
	if(!this->last_operation_was_start_el) {
		this->output << "\n" << std::string(this->indent_level, '	');
	}
	this->output << "</" << this->open_tags.top() << ">";
	this->open_tags.pop();
	this->last_operation_was_start_el = false;
}

void XML_writer::close_tag_if_open() {
	if(this->cur_tag_unclosed) {
		this->output << ">";
		this->cur_tag_unclosed = false;
	}
}

void XML_writer::write_attr(const std::string& name, const std::string& value) {
	if(!this->cur_tag_unclosed) {
		throw std::runtime_error("Attempt to write attribute while not in open tag.");
	}
	this->output << " " << name << "=\"" << escape_str(value) << "\"";
}

void XML_writer::write_str(const std::string& text, bool escape) {
	close_tag_if_open();
	this->output << (escape ? escape_str(text) : text);
}

void XML_writer::flush() {
	this->output.flush();
}

std::string XML_writer::escape_str(std::string text) {
	text = std::regex_replace(text, std::regex("&"), "&amp;");
	text = std::regex_replace(text, std::regex("\'"), "&apos;");
	text = std::regex_replace(text, std::regex("<"), "&lt;");
	text = std::regex_replace(text, std::regex(">"), "&gt;");
	text = std::regex_replace(text, std::regex("\""), "&quot;");
	return text;
}

CSV_Writer::CSV_Writer(std::ostream& stream, std::string seperator) : output(stream), seperator(seperator) {}

CSV_Writer& CSV_Writer::add(const char *str){
	return this->add(std::string(str));
}

CSV_Writer& CSV_Writer::add(char *str){
	return this->add(std::string(str));
}

CSV_Writer& CSV_Writer::add(std::string str) {
	size_t position = str.find("\"",0);
	bool found_quotation = position != std::string::npos;
	while(position != std::string::npos) {
		str.insert(position,"\"");
		position = str.find("\"",position + 2);
	}
	if(found_quotation) {
		str = "\"" + str + "\"";
	} else if(str.find(this->seperator) != std::string::npos) {
		str = "\"" + str + "\"";
	}
	return this->add<std::string>(str);
}

template<typename T>
CSV_Writer& CSV_Writer::add(T str) {
	if(!line_empty) {
		this->output << this->seperator;
	}
	line_empty = false;
	this->output << str;
	return *this;
}

template<typename T>
CSV_Writer& CSV_Writer::operator<<(const T& t) {
	return this->add(t);
}

CSV_Writer& CSV_Writer::row() {
	if(!line_empty) {
		output << std::endl;
		line_empty = true;
	}
	return *this;
}

Log::Log(const std::string& _file_name, const std::string& ext, const std::vector<Field>& _fields): file_name(_file_name), fields(_fields) {
	if(ext == "console") {
		return;
	}
	int file_num = 0;
	std::string file_name;
	if(main_obj.rewrite_log) {
		file_name = main_obj.log_dir + "/" + _file_name + "." + ext;
	} else {
		do {
			file_name = main_obj.log_dir + "/" + _file_name + (file_num ? "." + std::to_string(file_num) : "") + "." + ext;
			if(file_num++ >= main_obj.max_log_cnt) {
				throw std::runtime_error("Unable to create " + file_name);
			}
		} while(utils::file_exists(file_name));
	}
	file.open(file_name, std::ios::out | std::ios::trunc);
	if(!file.is_open()) {
		throw std::runtime_error("Can not open " + file_name);
	}
}

Log::~Log() {
	if(file.is_open()) {
		file.close();
	}
}

Console_Log::Console_Log(const std::string& file_name, const std::vector<Field>& fields): Log(file_name, "console", fields) {}

void Console_Log::write(const std::vector<std::string>& msg) {
	std::lock_guard<std::mutex> lock(main_obj.mutex_log);
	if(msg.size() != fields.size()) {
		return;
	}
	std::cout << "[" << file_name << "] ";
	for(size_t i = 0; i < msg.size(); i++) {
		if(i) {
			std::cout << ", ";
		}
		std::cout << fields_all[fields[i]] << ": " << msg[i];
	}
	std::cout << std::endl;
}

CSV_Log::CSV_Log(const std::string& file_name, const std::vector<Field>& fields): Log(file_name, "csv", fields), writer(file, main_obj.csv_separator) {
	for(size_t i = 0; i < fields.size(); i++) {
		writer.add(fields_all[fields[i]]);
	}
	writer.row();
}

void CSV_Log::write(const std::vector<std::string>& msg) {
	std::lock_guard<std::mutex> lock(main_obj.mutex_log);
	if(msg.size() != fields.size()) {
		return;
	}
	for(auto& str : msg) {
		writer.add(str);
	}
	writer.row();
}

XML_Log::XML_Log(const std::string& file_name, const std::vector<Field>& fields): Log(file_name, "xml", fields), writer(file) {
	writer.write_start_doc();
}

XML_Log::~XML_Log() {
	writer.write_end_doc();
}

void XML_Log::write(const std::vector<std::string>& msg) {
	std::lock_guard<std::mutex> lock(main_obj.mutex_log);
	if(msg.size() != fields.size()) {
		return;
	}
	writer.write_start_el("row");
	for(size_t i = 0; i < msg.size(); i++) {
		writer.write_start_el(fields_all[fields[i]]);
		writer.write_str(msg[i]);
		writer.write_end_el();
	}
	writer.write_end_el();
}

void LogWrap::init(const std::set<std::string>& types, const std::string& file_name, const std::vector<Log::Field>& fields) {
	for(auto& type : types) {
		if(type == "console") {
			logs.push_back(std::unique_ptr<Log>(new Console_Log(file_name, fields)));
		} else if(type == "xml") {
			logs.push_back(std::unique_ptr<Log>(new XML_Log(file_name, fields)));
		} else if(type == "csv") {
			logs.push_back(std::unique_ptr<Log>(new CSV_Log(file_name, fields)));
		} else {
			throw std::runtime_error("Parameter 'type_log' is not valid");
		}
	}
	enabled = true;
}

void LogWrap::write(const std::vector<std::string>& msg) const {
	for(const auto& log : logs) {
		log->write(msg);
	}
}

void Main::import_param(const std::string& file) {

	namespace po = boost::program_options;

	std::ifstream infile(file);
	if(!infile.is_open()) {
		throw std::runtime_error("Can not open setting file");
	}

	po::variables_map options;
	po::options_description desc;

	desc.add_options()
		("main.subdomain", po::value<bool>(&param_subdomain))
		("main.link_check", po::value<bool>(&link_check))
		("main.thread", po::value<int>(&thread_cnt))
		("main.url", po::value<std::string>(&param_url))
		("main.sleep", po::value<int>(&param_sleep))
		("main.try_limit", po::value<int>(&try_limit))
		("main.url_limit", po::value<size_t>(&url_limit))
		("main.redirect_limit", po::value<size_t>(&redirect_limit))
		("main.cert_verification", po::value<bool>(&cert_verification))
		("main.ca_cert_file_path", po::value<std::string>(&ca_cert_file_path))
		("main.ca_cert_dir_path", po::value<std::string>(&ca_cert_dir_path))
		("main.bind_interface", po::value<std::string>(&param_interface))
		("filters.filter", po::value<std::vector<std::string>>())
		("sitemap.enabled", po::value<bool>(&sitemap))
		("sitemap.dir", po::value<std::string>(&sitemap_dir))
		("sitemap.file_name", po::value<std::string>(&xml_name))
		("sitemap.index_file_name", po::value<std::string>(&xml_index_name))
		("sitemap.filemb_lim", po::value<int>(&xml_filemb_lim))
		("sitemap.entry_lim", po::value<int>(&xml_entry_lim))
		("sitemap.xml_tag", po::value<std::vector<std::string>>())
		("log.type", po::value<std::string>())
		("log.dir", po::value<std::string>(&log_dir))
		("log.rewrite", po::value<bool>(&rewrite_log))
		("log.max_log_cnt", po::value<int>(&max_log_cnt))
		("log.csv_separator", po::value<std::string>(&csv_separator))
		("log.log_redirect", po::value<bool>(&param_log_redirect))
		("log.log_bad_html", po::value<bool>(&param_log_bad_html))
		("log.log_bad_url", po::value<bool>(&param_log_bad_url))
		("log.log_error_reply", po::value<bool>(&param_log_error_reply))
		("log.log_ignored_url", po::value<bool>(&param_log_ignored_url))
		("log.log_info", po::value<bool>(&param_log_info))
		("log.log_other", po::value<bool>(&param_log_other))
		("log.log_skipped_url", po::value<bool>(&param_log_skipped_url))
	;
	po::store(po::parse_config_file(infile, desc), options);
	po::notify(options);

	if(param_url.empty()) {
		throw std::runtime_error("Parameter 'url' is empty");
	}
	boost::system::result<boost::urls::url> r = boost::urls::parse_uri_reference(param_url);
	if(!r) {
		throw std::runtime_error("Parameter 'url' is not valid");
	}
	uri = r.value();

	if(thread_cnt >= 1 && thread_cnt <= 10) {
		thread_cnt = 1;
	}

	if(options.count("filters.filter")) {
		const auto& filters = options["filters.filter"].as<std::vector<std::string>>();
		for(const auto& filter : filters) {
			str_vec v;
			boost::split(v, filter, boost::is_any_of(" "));
			if(v.size() != 3) {
				throw std::runtime_error("Parameter 'filter' (" + filter + ") is not valid");
			}
			Filter f;
			if(v[0] == "regexp") {
				f.type = Filter::type_regexp;
				try {
					f.reg = std::regex(v[2], std::regex_constants::ECMAScript | std::regex_constants::icase);
				} catch(std::exception& e) {
					throw std::runtime_error(std::string("Parameter 'filter' (" + filter + ") is not valid: ") + e.what());
				}
			} else if(v[0] == "get") {
				f.type = Filter::type_get;
				f.val = v[2];
			} else if(v[0] == "ext") {
				f.type = Filter::type_ext;
				f.val = v[2];
			} else {
				throw std::runtime_error("Parameter 'filter' (" + filter + ") is not valid");
			}
			if(v[1] == "include") {
				f.dir = Filter::include;
			} else if(v[1] == "exclude") {
				f.dir = Filter::exclude;
			} else if(v[1] == "skip") {
				f.dir = Filter::skip;
			} else {
				throw std::runtime_error("Parameter 'filter' (" + filter + ") is not valid");
			}
			param_filter.push_back(std::move(f));
		}
	}

	if(options.count("sitemap.xml_tag")) {
		const auto& tags = options["sitemap.xml_tag"].as<std::vector<std::string>>();
		for(const auto& tag : tags) {
			str_vec v;
			boost::split(v, tag, boost::is_any_of(" "));
			if(v.size() != 3) {
				throw std::runtime_error("Parameter 'xml_tag' (" + tag + ") is not valid");
			}
			param_xml_tag[v[0]];
			if(v[2] == "default") {
				param_xml_tag[v[0]].def = v[2];
			} else {
				param_xml_tag[v[0]].regexp.emplace_back(v[1], v[2]);
			}
		}
	}

	if(options.count("log.type")) {
		str_vec vec;
		boost::split(vec, options["log.type"].as<std::string>(), boost::is_any_of(","));
		std::set<std::string> param_type_log(vec.begin(), vec.end());
		type_log = std::move(param_type_log);
		if(type_log.find("csv") != type_log.end() || type_log.find("xml") != type_log.end()) {
			if(log_dir.empty()) {
				throw std::runtime_error("Parameter 'log.dir' is empty");
			}
		}
	}
	auto it = type_log.find("console");
	if(it != type_log.end()) {
		type_log.erase(it);
		if(param_log_redirect) {
			log_redirect_console.init({"console"}, "redirect", {Log::Field::url, Log::Field::parent});
		}
		if(param_log_error_reply) {
			log_error_reply_console.init({"console"}, "error_reply", {Log::Field::msg, Log::Field::url, Log::Field::parent});
		}
		if(param_log_ignored_url) {
			log_ignored_url_console.init({"console"}, "ignored_url", {Log::Field::found, Log::Field::parent});
		}
		if(param_log_skipped_url) {
			log_skipped_url_console.init({"console"}, "skipped_url", {Log::Field::url, Log::Field::parent});
		}
		if(param_log_bad_html) {
			log_bad_html_console.init({"console"}, "bad_html", {Log::Field::msg, Log::Field::url});
		}
		if(param_log_bad_url) {
			log_bad_url_console.init({"console"}, "bad_url", {Log::Field::found, Log::Field::parent});
		}
		if(param_log_info) {
			log_info_console.init({"console"}, "info", {Log::Field::thread, Log::Field::time, Log::Field::url, Log::Field::parent});
		}
	}
	if(param_log_redirect) {
		log_redirect_file.init(type_log, "redirect", {Log::Field::url, Log::Field::id_parent});
	}
	if(param_log_error_reply) {
		log_error_reply_file.init(type_log, "error_reply", {Log::Field::msg, Log::Field::url, Log::Field::id_parent});
	}
	if(param_log_ignored_url) {
		log_ignored_url_file.init(type_log, "ignored_url", {Log::Field::found, Log::Field::id_parent});
	}
	if(param_log_skipped_url) {
		log_skipped_url_file.init(type_log, "skipped_url", {Log::Field::url, Log::Field::id_parent});
	}
	if(param_log_bad_html) {
		log_bad_html_file.init(type_log, "bad_html", {Log::Field::msg, Log::Field::id});
	}
	if(param_log_bad_url) {
		log_bad_url_file.init(type_log, "bad_url", {Log::Field::found, Log::Field::id_parent});
	}
	if(param_log_info) {
		log_info_file.init(type_log, "info", {Log::Field::id, Log::Field::parent, Log::Field::time, Log::Field::try_cnt, Log::Field::cnt, Log::Field::is_html, Log::Field::found, Log::Field::url, Log::Field::charset, Log::Field::msg});
	}
	if(param_log_other) {
		log_other.init(type_log, "other", {Log::Field::msg});
	}
	if(sitemap) {
		if(sitemap_dir.empty()) {
			throw std::runtime_error("Parameter 'sitemap.dir' is empty");
		}
		std::string file_name = sitemap_dir + "/" + xml_name + "1.xml";
		sitemap_file.open(file_name, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
		if(!sitemap_file.is_open()) {
			throw std::runtime_error("Can not open " + file_name);
		}
	}
}

std::string Main::uri_normalize(const boost::url& uri) {
	boost::url u = uri;
	u.normalize();
	u.remove_fragment();
	if(u.port() == "80" || u.port().empty()) {
		u.remove_port();
	}
	if (u.has_authority() && u.encoded_path().empty()) {
		u.set_path_absolute(true);
	}
	if(u.has_query()) {
		str_vec split_query;
		boost::split(split_query, u.query(), boost::is_any_of("&"));
		std::sort(split_query.begin(), split_query.end());
		u.set_query(boost::join(split_query, "&"));
	}
	return u.buffer();
}

bool Main::exit_handler() {
	std::unique_lock<std::mutex> lk(mutex);
	std::cout << "Stopping..." << std::endl;
	running = false;
	lk.unlock();
	cond.notify_all();
	return true;
}

void Main::start() {
	std::unique_ptr<Url_struct> url(new Url_struct);
	url->found = param_url;
	url->base_href = param_url;
	url->handle = url_handle_t::query_parse;
	if(!handle_url(url.get(), false)) {
		throw std::runtime_error("Parameter 'url' is not valid");
	}
	set_url(url);

	if(!sys::handle_exit()) {
		std::cout << "Could not set exit handler" << std::endl;
	}

	std::vector<Thread> threads;
	threads.reserve(thread_cnt);
	for(int i = 0; i < thread_cnt; i++) {
		threads.emplace_back(i + 1);
		threads[i].start();
	}
	for(auto& thread : threads) {
		thread.join();
	}

}

bool Main::set_url(std::unique_ptr<Url_struct>& url) {
	std::unique_lock<std::mutex> lk(mutex);
	if(url_limit && url_all.size() >= url_limit) {
		if(!url_lim_reached) {
			if(log_other) {
				log_other.write({"URL limit reached"});
			}
			url_lim_reached = true;
		}
		return false;
	}
	auto it = url_unique.find(url->normalize);
	if(it == url_unique.end()) {
		url_unique[url->normalize] = url_all.size();
		url->id = url_all.size() + 1;
		if(url->handle == url_handle_t::none) {
			if(log_skipped_url_file) {
				log_skipped_url_file.write({url->resolved, std::to_string(url->parent)});
			}
			if(log_skipped_url_console) {
				log_skipped_url_console.write({url->resolved, get_resolved(url->parent)});
			}
			url_all.push_back(std::move(url));
		} else {
			url_queue.push(url.get());
			url_all.push_back(std::move(url));
			lk.unlock();
			cond.notify_one();
		}
		return true;
	} else {
		url_all[it->second]->cnt++;
	}
	return false;
}

void Main::try_again(Url_struct* url) {
	std::unique_lock<std::mutex> lk(mutex);
	url_queue.push(url);
	lk.unlock();
	cond.notify_one();
}

bool Main::get_url(Thread* t) {
	std::unique_lock<std::mutex> lk(mutex);
	if(!running) {
		if(!t->suspend) {
			t->suspend = true;
			thread_work--;
		}
		return false;
	}
	if(url_queue.size()) {
		if(t->suspend) {
			t->suspend = false;
			thread_work++;
		}
		t->m_url = url_queue.front();
		url_queue.pop();
		return true;
	}
	if(!t->suspend) {
		t->suspend = true;
		thread_work--;
		if(thread_work == 0) {
			running = false;
			lk.unlock();
			cond.notify_all();
			return false;
		}
	}
	cond.wait(lk, [this] {
		return !running || url_queue.size();
	});
	return true;
}

std::string Main::get_resolved(int i) {
	if(i <= 0) {
		return "";
	}
	std::unique_lock<std::mutex> lk(mutex);
	return url_all[i - 1]->resolved;
}

bool Main::handle_url(Url_struct* url_new, bool filter) {

	std::string found = std::regex_replace(url_new->found, std::regex(R"(^\s+|\s+$)"), std::string(""));

	// ----- resolve
	boost::system::result<boost::url> rb = boost::urls::parse_uri_reference(url_new->base_href);
	boost::system::result<boost::url> rd = boost::urls::parse_uri_reference(found);
	if(!rb) {
		if(log_bad_url_file) {
			log_bad_url_file.write({url_new->base_href, std::to_string(url_new->parent)});
		}
		if(log_bad_url_console) {
			log_bad_url_console.write({url_new->base_href, get_resolved(url_new->parent)});
		}
	}
	if(!rd) {
		if(log_bad_url_file) {
			log_bad_url_file.write({url_new->found, std::to_string(url_new->parent)});
		}
		if(log_bad_url_console) {
			log_bad_url_console.write({url_new->found, get_resolved(url_new->parent)});
		}
		return false;
	}
	if(!rb || !rd) {
		return false;
	}
	boost::url b = rb.value();
	boost::url d = rd.value();
	b.resolve(d);

	// ----- base filter
	if(!b.scheme().starts_with("http")) {
		return false;
	}
	auto d_host = d.host();
	auto b_host = uri.host();
	if(d_host.empty()) {
		return false;
	}
	std::size_t i = d_host.find(b_host);
	if(i == std::string::npos || i != d_host.size() - b_host.size()) {
		return false;
	}

	// ----- user filter
	if(filter) {
		if(!param_subdomain && d_host != b_host) {
			return false;
		}
		for(auto it_filter = param_filter.begin() ; it_filter != param_filter.end(); ++it_filter) {
			bool res = true;
			bool check = false;
			if((*it_filter).type == Filter::type_regexp) {
				check = true;
				res = std::regex_search(b.buffer().data(), (*it_filter).reg);
			} else if((*it_filter).type == Filter::type_get) {
				if(b.has_query()) {
					str_vec split_query;
					boost::split(split_query, b.query(), boost::is_any_of("&"));
					check = true;
					res = false;
					for(auto i : split_query) {
						str_vec query_kv;
						boost::split(query_kv, i, boost::is_any_of("="));
						if(query_kv[0] == (*it_filter).val) {
							res = true;
							break;
						}
					}
				}
			} else if((*it_filter).type == Filter::type_ext) {
				std::string elem;
				std::string ext;
				auto elems = b.segments();
				if(!elems.empty()) {
					auto pos = elems.back().find_last_of('.');
					if(pos != std::string::npos) {
						ext = elems.back().substr(pos + 1);
					}
				}
				if(!ext.empty()) {
					check = true;
					res = ext == (*it_filter).val;
				}
			}
			if(check) {
				if((*it_filter).dir == Filter::exclude && res) {
					return false;
				}
				if((*it_filter).dir == Filter::include && !res) {
					return false;
				}
				if((*it_filter).dir == Filter::skip && res) {
					if(url_new->handle == url_handle_t::query_parse) {
						url_new->handle = url_handle_t::none;
					}
				}
			}
		}
	}
	boost::url uri_;
	uri_.set_path(b.path());
	if(b.has_query()) {
		uri_.set_query(b.query());
	}
	auto path = uri_.buffer();
	if(uri_.empty()) {
		path = "/";
	}
	url_new->path = path;
	url_new->normalize = uri_normalize(b);
	url_new->resolved = b.buffer();
	url_new->ssl = b.scheme() == "https";
	url_new->host = b.host();
	url_new->base_href = url_new->resolved;
	return true;
}

void Main::finished() {
	if(log_info_file) {
		for(auto it = url_all.begin(); it != url_all.end(); ++it) {
			log_info_file.write({
				std::to_string((*it)->id),
				std::to_string((*it)->parent),
				std::to_string((*it)->time),
				std::to_string((*it)->try_cnt),
				std::to_string((*it)->cnt),
				std::to_string((*it)->is_html),
				(*it)->found,
				(*it)->resolved,
				(*it)->charset,
				(*it)->error
			});
		}
	}
	if(sitemap) {
		auto base = uri.scheme().data() + std::string("://") + uri.authority().data();
		int i = 1;
		int j = 0;
		std::streamoff wrap_length = 0;
		std::streamoff tag_length = 0;
		std::streamoff pos = 0;
		std::streamoff outpos = 0;
		XML_writer writer(sitemap_file);
		writer.write_start_doc();
		writer.write_start_el("urlset");
		writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
		writer.write_end_el();
		writer.write_end_doc();
		writer.flush();
		wrap_length = sitemap_file.tellp();
		wrap_length += 1;
		sitemap_file.close();
		std::string file_name(sitemap_dir + "/" + xml_name + std::to_string(i) + ".xml");
		sitemap_file.open(file_name, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
		if(!sitemap_file.is_open()) {
			throw std::runtime_error("Can not open " + file_name);
		}
		writer.write_start_doc();
		writer.write_start_el("urlset");
		writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
		for(auto it = url_all.begin(); it != url_all.end(); ++it) {
			if((*it)->handle == url_handle_t::query) {
				continue;
			}
			if((*it)->handle == url_handle_t::query_parse && !(*it)->is_html) {
				continue;
			}
			int str_size = 0;
			std::vector<std::pair<std::string, std::string>> tags;
			tags.emplace_back("loc", writer.escape_str((*it)->resolved));
			str_size += tags.back().second.size();
			for(auto it1 = param_xml_tag.begin(); it1 != param_xml_tag.end(); ++it1) {
				tags.emplace_back(it1->first, writer.escape_str(it1->second.def));
				for(auto it2 = it1->second.regexp.begin() ; it2 != it1->second.regexp.end(); ++it2) {
					std::regex reg(it2->second, std::regex_constants::ECMAScript | std::regex_constants::icase);
					if(std::regex_search((*it)->resolved, reg)) {
						tags.back().second = writer.escape_str(it2->first);
					}
				}
				str_size += tags.back().second.size();
			}
			if(j >= xml_entry_lim || (pos + wrap_length + tag_length + str_size > xml_filemb_lim * 1024 * 1024)) {
				pos = 0;
				j = 0;
				i++;
				writer.write_end_el();
				writer.write_end_doc();
				writer.flush();
				sitemap_file.close();
				file_name = sitemap_dir + "/" + xml_name + std::to_string(i) + ".xml";
				sitemap_file.open(file_name, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
				if(!sitemap_file.is_open()) {
					throw std::runtime_error("Can not open " + file_name);
				}
				writer.write_start_doc();
				writer.write_start_el("urlset");
				writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
			}
			if(!tag_length) {
				outpos = sitemap_file.tellp();
			}
			writer.write_start_el("url");
			for(auto it1 = tags.begin(); it1 != tags.end(); ++it1) {
				writer.write_start_el(it1->first);
				writer.write_str(it1->second, false);
				writer.write_end_el();
			}
			writer.write_end_el();
			if(!tag_length) {
				tag_length = sitemap_file.tellp() - outpos;
				tag_length -= str_size;
				tag_length -= 1;
			}
			pos += tag_length + str_size;
			j++;
		}
		writer.write_end_el();
		writer.write_end_doc();
		writer.flush();
		sitemap_file.close();
		if(!xml_index_name.empty()) {
			file_name = sitemap_dir + "/" + xml_index_name + ".xml";
			sitemap_file.open(file_name, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
			if(!sitemap_file.is_open()) {
				throw std::runtime_error("Can not open " + file_name);
			}
			writer.write_start_doc();
			writer.write_start_el("sitemapindex");
			writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
			writer.write_start_el("sitemap");
			for(j = 1; j <= i; j++) {
				writer.write_start_el("loc");
				writer.write_str(base + xml_name + std::to_string(j) + ".xml");
				writer.write_end_el();
			}
			writer.write_end_el();
			writer.write_end_el();
			writer.write_end_doc();
			writer.flush();
			sitemap_file.close();
		}
	}
}

void Thread::start() {
	p.set_callback([this](html::node& n) {
		if(n.type_node != html::node_t::tag || n.type_tag != html::tag_t::open) {
			return;
		}
		if(n.tag_name == "base") {
			auto href = n.get_attr("href");
			if(!href.empty()) {
				m_url->base_href = href;
			}
			return;
		}
		if(std::find(Tags_main.begin(), Tags_main.end(), n.tag_name) != Tags_main.end()) {
			auto href = n.get_attr("href");
			if(!href.empty()) {
				std::unique_ptr<Url_struct> url(new Url_struct);
				url->found = href;
				url->handle = url_handle_t::query_parse;
				set_url(url);
			}
			return;
		}
		if(main_obj.link_check) {
			for(auto& tag : Tags_other) {
				if(n.tag_name == tag.name) {
					for(auto& attr : tag.attr) {
						auto href = n.get_attr(attr.name);
						if(href.empty()) {
							continue;
						}
						if(attr.pre && !attr.pre(n, href, this)) {
							continue;
						}
						std::unique_ptr<Url_struct> url(new Url_struct);
						url->found = href;
						url->handle = url_handle_t::query;
						set_url(url);
					}
					return;
				}
			}
		}
	});
	if(main_obj.param_log_bad_html) {
		p.set_callback([this](html::err_t e, html::node& n) {
			std::string msg;
			if(e == html::err_t::tag_not_closed) {
				html::node* current = &n;
				while(current->get_parent()) {
					msg.insert(0, " " + current->tag_name);
					current = current->get_parent();
				}
				msg.insert(0, "Unclosed tag:");
			}
			if(main_obj.log_bad_html_file) {
				main_obj.log_bad_html_file.write({msg, std::to_string(m_url->id)});
			}
			if(main_obj.log_bad_html_console) {
				main_obj.log_bad_html_console.write({msg, m_url->resolved});
			}
		});
	}
	uthread.reset(new std::thread(&Thread::load, this));
}

void Thread::join() {
	if(uthread != nullptr) {
		uthread->join();
		uthread = nullptr;
	}
}

void Thread::load() {
	try {
		{
			std::lock_guard<std::mutex> lk(main_obj.mutex);
			main_obj.thread_work++;
		}
		while(main_obj.get_url(this)) {
			if(suspend) {
				continue;
			}
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
			if(m_url->ssl) {
				if(main_obj.log_error_reply_file) {
					main_obj.log_error_reply_file.write({"HTTPS not supported", m_url->resolved, std::to_string(m_url->parent)});
				}
				if(main_obj.log_error_reply_console) {
					main_obj.log_error_reply_console.write({"HTTPS not supported", m_url->resolved, main_obj.get_resolved(m_url->parent)});
				}
				continue;
			}
#endif
			std::string scheme_host(m_url->ssl ? "https" : "http");
			scheme_host += "://" + m_url->host;
			cli = std::make_shared<httplib::Client>(scheme_host);
			if(!main_obj.param_interface.empty()) {
				cli->set_interface(main_obj.param_interface.data());
			}
			if(m_url->ssl) {
				cli->enable_server_certificate_verification(main_obj.cert_verification);
				if(main_obj.cert_verification) {
					if(!main_obj.ca_cert_file_path.empty()) {
						cli->set_ca_cert_path(main_obj.ca_cert_file_path.c_str());
					}
					if(!main_obj.ca_cert_dir_path.empty()) {
						cli->set_ca_cert_path(nullptr, main_obj.ca_cert_dir_path.c_str());
					}
				}
			}
			Timer tmr;
			if(m_url->handle == url_handle_t::query_parse) {
				result = std::make_shared<httplib::Result>(cli->Get(m_url->path.c_str()));
			} else {
				result = std::make_shared<httplib::Result>(cli->Head(m_url->path.c_str()));
			}
			double time = tmr.seconds();
			m_url->time += time;
			m_url->try_cnt++;
			if(main_obj.log_info_console) {
				main_obj.log_info_console.write({std::to_string(id), std::to_string(time), m_url->resolved, main_obj.get_resolved(m_url->parent)});
			}
			http_finished();
			if(main_obj.param_sleep) {
				std::this_thread::sleep_for(std::chrono::milliseconds(main_obj.param_sleep));
			}
		}
	} catch(...) {
		{
			std::lock_guard<std::mutex> lk(main_obj.mutex);
			exc_ptr = std::current_exception();
			main_obj.running = false;
		}
		main_obj.cond.notify_all();
	}
}

void Thread::http_finished() {
	auto& reply = *result;
	if(!reply) {
		if(m_url->try_cnt < main_obj.try_limit) {
			main_obj.try_again(m_url);
		} else {
			m_url->error = httplib::to_string(reply.error());
			if(main_obj.log_error_reply_file) {
				main_obj.log_error_reply_file.write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
			}
			if(main_obj.log_error_reply_console) {
				main_obj.log_error_reply_console.write({m_url->error, m_url->resolved, main_obj.get_resolved(m_url->parent)});
			}
		}
		return;
	}
	if(m_url->ssl) {
		auto res = cli->get_openssl_verify_result();
		if(res != X509_V_OK) {
			m_url->error = std::string("Certificate verification error: ") + X509_verify_cert_error_string(res);
			if(main_obj.log_error_reply_file) {
				main_obj.log_error_reply_file.write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
			}
			if(main_obj.log_error_reply_console) {
				main_obj.log_error_reply_console.write({m_url->error, m_url->resolved, main_obj.get_resolved(m_url->parent)});
			}
			return;
		}
	}
	if(reply->status >= 500 && reply->status < 600 && m_url->try_cnt < main_obj.try_limit) {
		main_obj.try_again(m_url);
		return;
	}
	if(reply->status >= 300 && reply->status < 400) {
		m_url->error = "Redirect";
		if(main_obj.log_redirect_file) {
			main_obj.log_redirect_file.write({m_url->resolved, std::to_string(m_url->parent)});
		}
		if(main_obj.log_redirect_console) {
			main_obj.log_redirect_console.write({m_url->resolved, main_obj.get_resolved(m_url->parent)});
		}
		if(m_url->redirect_cnt > main_obj.redirect_limit) {
			if(main_obj.log_error_reply_file) {
				main_obj.log_error_reply_file.write({"Redirect limit reached", m_url->resolved, std::to_string(m_url->parent)});
			}
			if(main_obj.log_error_reply_console) {
				main_obj.log_error_reply_console.write({"Redirect limit reached", m_url->resolved, main_obj.get_resolved(m_url->parent)});
			}
			return;
		}
		if(reply->has_header("Location")) {
			std::unique_ptr<Url_struct> url(new Url_struct);
			url->found = reply->get_header_value("Location");
			url->handle = url_handle_t::query_parse;
			url->redirect_cnt = m_url->redirect_cnt + 1;
			set_url(url);
		}
		return;
	}
	if(reply->status != 200) {
		m_url->error = "Code:" + std::to_string(reply->status);
		if(main_obj.log_error_reply_file) {
			main_obj.log_error_reply_file.write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
		}
		if(main_obj.log_error_reply_console) {
			main_obj.log_error_reply_console.write({m_url->error, m_url->resolved, main_obj.get_resolved(m_url->parent)});
		}
		return;
	}
	if(m_url->handle == url_handle_t::query) {
		return;
	}
	if(!reply->has_header("Content-Type")) {
		m_url->error = "Content-Type empty";
		if(main_obj.log_error_reply_file) {
			main_obj.log_error_reply_file.write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
		}
		if(main_obj.log_error_reply_console) {
			main_obj.log_error_reply_console.write({m_url->error, m_url->resolved, main_obj.get_resolved(m_url->parent)});
		}
		return;
	}
	// check html
	auto content_type = reply->get_header_value("Content-Type");
	if(content_type.find("text/html") == std::string::npos) {
		return;
	}
	m_url->is_html = true;
	// charset from header
	auto pos = content_type.find("charset=");
	if(pos != std::string::npos) {
		m_url->charset = content_type.substr(pos + 8);
	}
	p.parse(reply->body);
}

void Thread::set_url(std::unique_ptr<Url_struct>& new_url) {
	new_url->parent = m_url->id;
	new_url->base_href = m_url->base_href;
	if(main_obj.handle_url(new_url.get())) {
		main_obj.set_url(new_url);
	} else {
		if(main_obj.log_ignored_url_file) {
			main_obj.log_ignored_url_file.write({new_url->found, std::to_string(new_url->parent)});
		}
		if(main_obj.log_ignored_url_console) {
			main_obj.log_ignored_url_console.write({new_url->found, main_obj.get_resolved(new_url->parent)});
		}
	}
}

bool Handler::attr_charset(html::node& n, std::string& href, Thread* t) {
	if(t->m_url->charset.empty()) {
		if(boost::to_lower_copy(n.get_attr("http-equiv")) == "content-type") {
			auto pos = href.find("charset=");
			if(pos != std::string::npos) {
				t->m_url->charset = href.substr(pos + 8);
			}
		}
	}
	return false;
}

bool Handler::attr_refresh(html::node& n, std::string& href, Thread* t) {
	if(t->m_url->charset.empty()) {
		if(boost::to_lower_copy(n.get_attr("http-equiv")) == "refresh") {
			std::regex e("[\\d\\s]+;\\s*url\\s*=\\s*(.+)", std::regex_constants::ECMAScript | std::regex_constants::icase);
			std::smatch m;
			if(std::regex_match(href, m, e)) {
				std::unique_ptr<Url_struct> url(new Url_struct);
				url->found = m[1];
				url->handle = url_handle_t::query_parse;
				t->set_url(url);
			}
		}
	}
	return false;
}

bool Handler::attr_srcset(html::node& n, std::string& href, Thread* t) {
	str_vec src_all;
	boost::split(src_all, href, boost::is_any_of(","));
	for(auto v : src_all) {
		str_vec src;
		boost::split(src, v, boost::is_any_of(" "));
		if(!src.empty()) {
			std::unique_ptr<Url_struct> url(new Url_struct);
			url->found = src[0];
			url->handle = url_handle_t::query;
			t->set_url(url);
		}
	}
	return false;
}

namespace sys {

#ifdef WINDOWS_PLATFORM
BOOL WINAPI ctrl_handler(DWORD ctrl_type) {
	switch(ctrl_type) {
		case CTRL_C_EVENT:
			return main_obj.exit_handler() ? TRUE : FALSE;
		break;
		default:
			return FALSE;
	}
}

bool handle_exit() {
	return SetConsoleCtrlHandler(ctrl_handler, TRUE) != 0;
}
#elif defined(LINUX_PLATFORM) || defined(MACOS_PLATFORM)
void sig_handler(int sig) {
	switch(sig) {
		case SIGINT:
			main_obj.exit_handler();
		break;
		default:
			return;
	}
}

bool handle_exit() {
	struct sigaction action;
	action.sa_handler = sig_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if(sigaction(SIGINT, &action, nullptr) == 0) {
		return true;
	}
	return false;
}
#else
bool handle_exit() {
	return false;
}
#endif

}

namespace utils {

bool file_exists(const std::string& str) {
   std::ifstream fs(str);
   return fs.is_open();
}

}

int main(int argc, char *argv[]) {
	try {
		if(argc != 2) {
			throw std::runtime_error("Specify setting file");
		}
		Timer tmr;
		main_obj.import_param(argv[1]);
		main_obj.start();
		if(exc_ptr) {
			std::rethrow_exception(exc_ptr);
		}
		main_obj.finished();
		auto elapsed_str = "Elapsed time: " + tmr.elapsed_str();
		std::cout << elapsed_str << std::endl;
		if(main_obj.log_other) {
			main_obj.log_other.write({elapsed_str});
		}
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		if(main_obj.log_other) {
			main_obj.log_other.write({e.what()});
		}
	}
	return 0;
}