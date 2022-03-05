#include "sitemap.h"

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

std::vector<std::string> Utils::split(const std::string& str, char ch, bool ignore_ws) {
	std::string elem;
	std::stringstream si(str);
	std::vector<std::string> elems;
	while(std::getline(si, elem, ch)) {
		if(ignore_ws && elem == "") {
			continue;
		}
		elems.push_back(elem);
	}
	return elems;
}

std::string Utils::join(const std::vector<std::string>& v, char ch) {
	std::stringstream ss;
	for(size_t i = 0; i < v.size(); ++i) {
		if(i != 0) {
			ss << ch;
		}
		ss << v[i];
	}
	return ss.str();
}

std::string Utils::str_tolower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

bool Utils::file_exists(const std::string& str) {
   std::ifstream fs(str);
   return fs.is_open();
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
CSV_Writer& CSV_Writer::operator<<(const T& t){
	return this->add(t);
}

CSV_Writer& CSV_Writer::row() {
	if(!line_empty) {
		output << std::endl;
		line_empty = true;
	}
	return *this;
}

Log::Log(Main* _main, const std::string& _file_name, const std::string& ext, const std::vector<Field>& _fields): main(_main), file_name(_file_name), fields(_fields) {
	if(ext == "console") {
		return;
	}
	int file_num = 0;
	std::string file_name;
	if(main->rewrite_log) {
		file_name = main->log_dir + "/" + _file_name + "." + ext;
	} else {
		do {
			file_name = main->log_dir + "/" + _file_name + (file_num ? "." + std::to_string(file_num) : "") + "." + ext;
			if(file_num++ >= main->max_log_cnt) {
				throw std::runtime_error("Unable to create " + file_name);
			}
		} while(Utils::file_exists(file_name));
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

Console_Log::Console_Log(Main* main, const std::string& file_name, const std::vector<Field>& fields): Log(main, file_name, "console", fields) {}

void Console_Log::write(const std::vector<std::string>& msg) {
	std::lock_guard<std::mutex> lock(main->mutex_log);
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

CSV_Log::CSV_Log(Main* main, const std::string& file_name, const std::vector<Field>& fields): Log(main, file_name, "csv", fields), writer(file, main->csv_separator) {
	for(size_t i = 0; i < fields.size(); i++) {
		writer.add(fields_all[fields[i]]);
	}
	writer.row();
}

void CSV_Log::write(const std::vector<std::string>& msg) {
	std::lock_guard<std::mutex> lock(main->mutex_log);
	if(msg.size() != fields.size()) {
		return;
	}
	for(auto& str : msg) {
		writer.add(str);
	}
	writer.row();
}

XML_Log::XML_Log(Main* main, const std::string& file_name, const std::vector<Field>& fields): Log(main, file_name, "xml", fields), writer(file) {
	writer.write_start_doc();
}

XML_Log::~XML_Log() {
	writer.write_end_doc();
}

void XML_Log::write(const std::vector<std::string>& msg) {
	std::lock_guard<std::mutex> lock(main->mutex_log);
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

void LogWrap::init(Main* main, const std::string& file_name, const std::vector<Log::Field>& fields) {
	if(main->type_log == "console") {
		log = std::unique_ptr<Console_Log>(new Console_Log(main, file_name, fields));
	} else if(main->type_log == "xml") {
		log = std::unique_ptr<XML_Log>(new XML_Log(main, file_name, fields));
	} else if(main->type_log == "csv") {
		log = std::unique_ptr<CSV_Log>(new CSV_Log(main, file_name, fields));
	} else {
		throw std::runtime_error("Parameter 'type_log' is not valid");
	}
	enabled = true;
}

void Main::import_param(const std::string& file) {
	std::ifstream infile(file);
	if(!infile.is_open()) {
		throw std::runtime_error("Can not open setting file");
	}
	std::string line;
	std::vector<std::vector<std::string>> settings;
	while(std::getline(infile, line)) {
		std::istringstream iss(line);
		std::vector<std::string> res((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
		if(res.size() == 1) {
			if(res[0] == "subdomain") {
				param_subdomain = true;
			} else if(res[0] == "log_redirect") {
				param_log_redirect = true;
			} else if(res[0] == "log_error_reply") {
				param_log_error_reply = true;
			} else if(res[0] == "log_ignored_url") {
				param_log_ignored_url = true;
			} else if(res[0] == "log_skipped_url") {
				param_log_skipped_url = true;
			} else if(res[0] == "log_bad_html") {
				param_log_bad_html = true;
			} else if(res[0] == "log_bad_url") {
				param_log_bad_url = true;
			} else if(res[0] == "log_other") {
				param_log_other = true;
			} else if(res[0] == "log_info") {
				param_log_info = true;
			} else if(res[0] == "cert_verification") {
				cert_verification = true;
			} else if(res[0] == "link_check") {
				link_check = true;
			} else if(res[0] == "sitemap") {
				sitemap = true;
			} else if(res[0] == "rewrite_log") {
				rewrite_log = true;
			}
		} else if(res.size() == 2) {
			if(res[0] == "url") {
				param_url = res[1];
				if(!uri.ParseFromString(param_url)) {
					throw std::runtime_error("Parameter 'url' is not valid");
				}
			} else if(res[0] == "xml_name") {
				xml_name = res[1];
			} else if(res[0] == "xml_index_name") {
				xml_index_name = res[1];
			} else if(res[0] == "thread") {
				int index = std::stoi(res[1]);
				if(index >= 1 && index <= 10) {
					thread_cnt = index;
				}
			} else if(res[0] == "log_dir") {
				log_dir = res[1];
			} else if(res[0] == "sitemap_dir") {
				sitemap_dir = res[1];
			} else if(res[0] == "sleep") {
				param_sleep = std::stoi(res[1]);
			} else if(res[0] == "redirect_limit") {
				redirect_limit = std::stoi(res[1]);
			} else if(res[0] == "url_limit") {
				url_limit = std::stoi(res[1]);
			} else if(res[0] == "xml_filemb_lim") {
				xml_filemb_lim = std::stoi(res[1]);
			} else if(res[0] == "xml_entry_lim") {
				xml_entry_lim = std::stoi(res[1]);
			} else if(res[0] == "try_limit") {
				try_limit = std::stoi(res[1]);
			} else if(res[0] == "csv_separator") {
				csv_separator = res[1];
			} else if(res[0] == "ca_cert_file_path") {
				ca_cert_file_path = res[1];
			} else if(res[0] == "ca_cert_dir_path") {
				ca_cert_dir_path = res[1];
			} else if(res[0] == "type_log") {
				type_log = res[1];
			} else if(res[0] == "max_log_cnt") {
				max_log_cnt = std::stoi(res[1]);
			} else if(res[0] == "bind_interface") {
				param_interface = res[1];
			}
		} else if(res[0] == "filter" && res.size() == 4) {
			Filter f;
			if(res[1] == "regexp") {
				f.type = Filter::type_regexp;
				try {
					f.reg = std::regex(res[3], std::regex_constants::ECMAScript | std::regex_constants::icase);
				} catch(std::exception& e) {
					throw std::runtime_error(std::string("Parameter 'filter regexp' is not valid: ") + e.what());
				}
			} else if(res[1] == "get") {
				f.type = Filter::type_get;
				f.val = res[3];
			} else if(res[1] == "ext") {
				f.type = Filter::type_ext;
				f.val = res[3];
			} else {
				throw std::runtime_error("Parameter 'filter' is not valid");
			}
			if(res[2] == "include") {
				f.dir = Filter::include;
			} else if(res[2] == "exclude") {
				f.dir = Filter::exclude;
			} else if(res[2] == "skip") {
				f.dir = Filter::skip;
			} else {
				throw std::runtime_error("Parameter 'filter' is not valid");
			}
			param_filter.push_back(f);
		} else if(res[0] == "xml_tag" && res.size() == 3) {
			Xml_tag x;
			x.def = res[2];
			param_xml_tag.emplace(res[1], x);
		} else if(res[0] == "xml_tag" && res.size() == 4) {
			auto it = param_xml_tag.find(res[1]);
			if(it != param_xml_tag.end()) {
			   it->second.regexp.emplace_back(res[2], res[3]);
			}
		}
	}
	if(param_url.empty()) {
		throw std::runtime_error("Parameter 'url' is empty");
	}
	if(log_dir.empty()) {
		throw std::runtime_error("Parameter 'log_dir' is empty");
	}
	if(sitemap_dir.empty()) {
		throw std::runtime_error("Parameter 'sitemap_dir' is empty");
	}
	if(param_log_redirect) {
		if(type_log == "console") {
			log_redirect_console.init(this, "redirect", {Log::Field::url, Log::Field::parent});
		} else {
			log_redirect_file.init(this, "redirect", {Log::Field::url, Log::Field::id_parent});
		}
	}
	if(param_log_error_reply) {
		if(type_log == "console") {
			log_error_reply_console.init(this, "error_reply", {Log::Field::msg, Log::Field::url, Log::Field::parent});
		} else {
			log_error_reply_file.init(this, "error_reply", {Log::Field::msg, Log::Field::url, Log::Field::id_parent});
		}
	}
	if(param_log_ignored_url) {
		if(type_log == "console") {
			log_ignored_url_console.init(this, "ignored_url", {Log::Field::found, Log::Field::parent});
		} else {
			log_ignored_url_file.init(this, "ignored_url", {Log::Field::found, Log::Field::id_parent});
		}
	}
	if(param_log_skipped_url) {
		if(type_log == "console") {
			log_skipped_url_console.init(this, "skipped_url", {Log::Field::url, Log::Field::parent});
		} else {
			log_skipped_url_file.init(this, "skipped_url", {Log::Field::url, Log::Field::id_parent});
		}
	}
	if(param_log_bad_html) {
		if(type_log == "console") {
			log_bad_html_console.init(this, "bad_html", {Log::Field::msg, Log::Field::url});
		} else {
			log_bad_html_file.init(this, "bad_html", {Log::Field::msg, Log::Field::id});
		}
	}
	if(param_log_bad_url) {
		if(type_log == "console") {
			log_bad_url_console.init(this, "bad_url", {Log::Field::found, Log::Field::parent});
		} else {
			log_bad_url_file.init(this, "bad_url", {Log::Field::found, Log::Field::id_parent});
		}
	}
	if(param_log_other) {
		log_other.init(this, "other", {Log::Field::msg});
	}
	if(param_log_info) {
		if(type_log == "console") {
			log_info_console.init(this, "info", {Log::Field::thread, Log::Field::time, Log::Field::url, Log::Field::parent});
		} else {
			log_info_file.init(this, "info", {Log::Field::id, Log::Field::parent, Log::Field::time, Log::Field::try_cnt, Log::Field::cnt, Log::Field::is_html, Log::Field::found, Log::Field::url, Log::Field::charset, Log::Field::msg});
		}
	}
	if(sitemap) {
		std::string file_name = sitemap_dir + "/" + xml_name + "1.xml";
		sitemap_file.open(file_name, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
		if(!sitemap_file.is_open()) {
			throw std::runtime_error("Can not open " + file_name);
		}
	}
}

std::string Main::uri_normalize(const Uri::Uri& uri) {
	Uri::Uri uri_(uri);
	// remove trailing /
	auto path = uri_.GetPath();
	if(!path.empty() && path.back().empty()) {
		path.pop_back();
		uri_.SetPath(path);
	}
	// sort query
	if(uri_.HasQuery()) {
		auto split_query = Utils::split(uri_.GetQuery(), '&');
		std::sort(split_query.begin(), split_query.end());
		uri_.SetQuery(Utils::join(split_query, '&'));
	}
	// remove fragment
	uri_.ClearFragment();
	return uri_.GenerateString();
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

	std::thread t([this]() {
		std::string ch;
		while(1) {
			std::cin >> ch;
			if(ch == "q") {
				std::unique_lock<std::mutex> lk(mutex);
				std::cout << "stopping..." << std::endl;
				running = false;
				lk.unlock();
				cond.notify_all();
				break;
			}
		}
	});
	t.detach();

	std::vector<Thread> threads;
	threads.reserve(thread_cnt);
	for(int i = 0; i < thread_cnt; i++) {
		threads.emplace_back(i + 1, this);
		threads[i].start();
	}
	for(auto &thread : threads) {
		thread.join();
	}
}

bool Main::set_url(std::unique_ptr<Url_struct>& url) {
	std::unique_lock<std::mutex> lk(mutex);
	if(url_limit && url_all.size() >= url_limit) {
		if(!url_lim_reached) {
			if(log_other) {
				log_other->write({"URL limit reached"});
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
				log_skipped_url_file->write({url->resolved, std::to_string(url->parent)});
			}
			if(log_skipped_url_console) {
				log_skipped_url_console->write({url->resolved, get_resolved(url->parent)});
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
	Uri::Uri b;
	Uri::Uri d;
	if(!b.ParseFromString(url_new->base_href)) {
		if(log_bad_url_file) {
			log_bad_url_file->write({url_new->base_href, std::to_string(url_new->parent)});
		}
		if(log_bad_url_console) {
			log_bad_url_console->write({url_new->base_href, get_resolved(url_new->parent)});
		}
		return false;
	}
	if(!d.ParseFromString(found)) {
		if(log_bad_url_file) {
			log_bad_url_file->write({url_new->found, std::to_string(url_new->parent)});
		}
		if(log_bad_url_console) {
			log_bad_url_console->write({url_new->found, get_resolved(url_new->parent)});
		}
		return false;
	}
	auto r = b.Resolve(d);

	// ----- base filter
	if(r.GetScheme().find("http") != 0) {
		return false;
	}
	auto d_host = r.GetHost();
	auto b_host = uri.GetHost();
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
				res = std::regex_search(r.GenerateString(), (*it_filter).reg);
			} else if((*it_filter).type == Filter::type_get) {
				if(r.HasQuery()) {
					auto split_query = Utils::split(r.GetQuery(), '&');
					check = true;
					res = false;
					for(auto i : split_query) {
						auto query_kv = Utils::split(i, '=');
						if(query_kv[0] == (*it_filter).val) {
							res = true;
							break;
						}
					}
				}
			} else if((*it_filter).type == Filter::type_ext) {
				std::string elem;
				std::string ext;
				auto elems = r.GetPath();
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
	Uri::Uri uri_;
	uri_.SetPath(r.GetPath());
	if(r.HasQuery()) {
		uri_.SetQuery(r.GetQuery());
	}
	url_new->path = uri_.GenerateString();
	url_new->normalize = uri_normalize(r);
	url_new->resolved = r.GenerateString();
	url_new->ssl = r.GetScheme() == "https";
	url_new->host = r.GetHost();
	url_new->base_href = url_new->resolved;
	return true;
}

void Main::finished() {
	if(log_info_file) {
		for(auto it = url_all.begin(); it != url_all.end(); ++it) {
			log_info_file->write({
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
		uri.ClearQuery();
		uri.ClearFragment();
		uri.SetPath(std::vector<std::string>{""});
		auto base = uri.GenerateString();
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
		if(main->link_check) {
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
	if(main->param_log_bad_html) {
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
			if(main->log_bad_html_file) {
				main->log_bad_html_file->write({msg, std::to_string(m_url->id)});
			}
			if(main->log_bad_html_console) {
				main->log_bad_html_console->write({msg, m_url->resolved});
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
			std::lock_guard<std::mutex> lk(main->mutex);
			main->thread_work++;
		}
		while(main->get_url(this)) {
			if(suspend) {
				continue;
			}
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
			if(m_url->ssl) {
				if(main->log_error_reply_file) {
					main->log_error_reply_file->write({"HTTPS not supported", m_url->resolved, std::to_string(m_url->parent)});
				}
				if(main->log_error_reply_console) {
					main->log_error_reply_console->write({"HTTPS not supported", m_url->resolved, main->get_resolved(m_url->parent)});
				}
				continue;
			}
#endif
			std::string scheme_host(m_url->ssl ? "https" : "http");
			scheme_host += "://" + m_url->host;
			cli = std::make_shared<httplib::Client>(scheme_host);
			if(!main->param_interface.empty()) {
				cli->set_interface(main->param_interface.data());
			}
			if(m_url->ssl) {
				cli->enable_server_certificate_verification(main->cert_verification);
				if(main->cert_verification) {
					if(!main->ca_cert_file_path.empty()) {
						cli->set_ca_cert_path(main->ca_cert_file_path.c_str());
					}
					if(!main->ca_cert_dir_path.empty()) {
						cli->set_ca_cert_path(nullptr, main->ca_cert_dir_path.c_str());
					}
				}
			}
			Timer tmr;
			if(m_url->handle == url_handle_t::query_parse) {
				result = std::make_shared<httplib::Result>(cli->Get(m_url->path.c_str()));
			} else {
				result = std::make_shared<httplib::Result>(cli->Head(m_url->path.c_str()));
			}
			double time = tmr.elapsed();
			m_url->time += time;
			m_url->try_cnt++;
			if(main->log_info_console) {
				main->log_info_console->write({std::to_string(id), std::to_string(time), m_url->resolved, main->get_resolved(m_url->parent)});
			}
			http_finished();
			if(main->param_sleep) {
				std::this_thread::sleep_for(std::chrono::milliseconds(main->param_sleep));
			}
		}
	} catch(...) {
		{
			std::lock_guard<std::mutex> lk(main->mutex);
			exc_ptr = std::current_exception();
			main->running = false;
		}
		main->cond.notify_all();
	}
}

void Thread::http_finished() {
	auto& reply = *result;
	if(!reply) {
		if(m_url->try_cnt < main->try_limit) {
			main->try_again(m_url);
		} else {
			m_url->error = httplib::to_string(reply.error());
			if(main->log_error_reply_file) {
				main->log_error_reply_file->write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
			}
			if(main->log_error_reply_console) {
				main->log_error_reply_console->write({m_url->error, m_url->resolved, main->get_resolved(m_url->parent)});
			}
		}
		return;
	}
	if(m_url->ssl) {
		auto res = cli->get_openssl_verify_result();
		if(res != X509_V_OK) {
			m_url->error = std::string("Certificate verification error: ") + X509_verify_cert_error_string(res);
			if(main->log_error_reply_file) {
				main->log_error_reply_file->write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
			}
			if(main->log_error_reply_console) {
				main->log_error_reply_console->write({m_url->error, m_url->resolved, main->get_resolved(m_url->parent)});
			}
			return;
		}
	}
	if(reply->status >= 500 && reply->status < 600 && m_url->try_cnt < main->try_limit) {
		main->try_again(m_url);
		return;
	}
	if(reply->status >= 300 && reply->status < 400) {
		m_url->error = "Redirect";
		if(main->log_redirect_file) {
			main->log_redirect_file->write({m_url->resolved, std::to_string(m_url->parent)});
		}
		if(main->log_redirect_console) {
			main->log_redirect_console->write({m_url->resolved, main->get_resolved(m_url->parent)});
		}
		if(m_url->redirect_cnt > main->redirect_limit) {
			if(main->log_error_reply_file) {
				main->log_error_reply_file->write({"Redirect limit reached", m_url->resolved, std::to_string(m_url->parent)});
			}
			if(main->log_error_reply_console) {
				main->log_error_reply_console->write({"Redirect limit reached", m_url->resolved, main->get_resolved(m_url->parent)});
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
		if(main->log_error_reply_file) {
			main->log_error_reply_file->write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
		}
		if(main->log_error_reply_console) {
			main->log_error_reply_console->write({m_url->error, m_url->resolved, main->get_resolved(m_url->parent)});
		}
		return;
	}
	if(m_url->handle == url_handle_t::query) {
		return;
	}
	if(!reply->has_header("Content-Type")) {
		m_url->error = "Content-Type empty";
		if(main->log_error_reply_file) {
			main->log_error_reply_file->write({m_url->error, m_url->resolved, std::to_string(m_url->parent)});
		}
		if(main->log_error_reply_console) {
			main->log_error_reply_console->write({m_url->error, m_url->resolved, main->get_resolved(m_url->parent)});
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
	if(main->handle_url(new_url.get())) {
		main->set_url(new_url);
	} else {
		if(main->log_ignored_url_file) {
			main->log_ignored_url_file->write({new_url->found, std::to_string(new_url->parent)});
		}
		if(main->log_ignored_url_console) {
			main->log_ignored_url_console->write({new_url->found, main->get_resolved(new_url->parent)});
		}
	}
}

bool Handler::attr_charset(html::node& n, std::string& href, Thread* t) {
	if(t->m_url->charset.empty()) {
		if(Utils::str_tolower(n.get_attr("http-equiv")) == "content-type") {
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
		if(Utils::str_tolower(n.get_attr("http-equiv")) == "refresh") {
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
	auto src_all = Utils::split(href, ',');
	for(auto v : src_all) {
		auto src = Utils::split(v, ' ');
		if(!src.empty()) {
			std::unique_ptr<Url_struct> url(new Url_struct);
			url->found = src[0];
			url->handle = url_handle_t::query;
			t->set_url(url);
		}
	}
	return false;
}

int main(int argc, char *argv[]) {
	Main c;
	try {
		if(argc != 2) {
			throw std::runtime_error("Specify setting file");
		}
		Timer tmr;
		c.import_param(argv[1]);
		c.start();
		if(exc_ptr) {
			std::rethrow_exception(exc_ptr);
		}
		c.finished();
		auto elapsed = "Elapsed time: " + std::to_string(tmr.elapsed());
		if(c.log_other) {
			c.log_other->write({elapsed});
		} else {
			std::cout << elapsed << std::endl;
		}
	} catch (const std::exception& e) {
		if(c.log_other) {
			c.log_other->write({e.what()});
		} else {
			std::cout << e.what() << std::endl;
		}
	}
	return 0;
}