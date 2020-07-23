#include "sitemap.h"

std::exception_ptr exc_ptr = nullptr;

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

XML_writer::XML_writer(std::ostream& stream) : output(stream) {}

void XML_writer::write_start_doc() {
	if (!this->open_tags.empty()) {
		throw std::runtime_error("Attempt to write start document in already started document.");
	}
	this->output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	this->cur_tag_unclosed = false;
}

void XML_writer::write_end_doc() {
	while (!this->open_tags.empty()) {
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
	if (this->open_tags.empty()) {
		throw std::runtime_error("Attempt to close tag while no tags are currently opened.");
	}
	close_tag_if_open();
	this->indent_level--;
	if (!this->last_operation_was_start_el) {
		this->output << "\n" << std::string(this->indent_level, '	');
	}
	this->output << "</" << this->open_tags.top() << ">";
	this->open_tags.pop();
	this->last_operation_was_start_el = false;
}

void XML_writer::close_tag_if_open() {
	if (this->cur_tag_unclosed) {
		this->output << ">";
		this->cur_tag_unclosed = false;
	}
}

void XML_writer::write_attr(const std::string& name, const std::string& value) {
	if (!this->cur_tag_unclosed) {
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

CSV_Writer::CSV_Writer(std::string seperator) : seperator(seperator) {}

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
		this->ss << this->seperator;
	}
	line_empty = false;
	this->ss << str;
	return *this;
}

template<typename T>
CSV_Writer& CSV_Writer::operator<<(const T& t){
	return this->add(t);
}

void CSV_Writer::operator+=(CSV_Writer &csv) {
	this->ss << std::endl << csv;
}

std::string CSV_Writer::to_string() {
	return ss.str();
}

CSV_Writer& CSV_Writer::row() {
	if(!line_empty) {
		ss << std::endl;
		line_empty = true;
	}
	return *this;
}

bool CSV_Writer::write_to_file(std::string filename, bool append) {
	std::ofstream file;
	if(append) {
		file.open(filename.c_str(), std::ios::out | std::ios::app);
	} else {
		file.open(filename.c_str(), std::ios::out | std::ios::trunc);
	}
	if(!file.is_open()) {
		return false;
	}
	if(append) {
		file << std::endl;
	}
	file << this->to_string();
	file.close();
	return file.good();
}

std::ostream& operator<<(std::ostream& os, CSV_Writer& csv) {
	return os << csv.to_string();
}

void Main::import_param(const std::string& file) {
	std::ifstream infile(file);
	if(!infile.is_open()) {
		throw std::runtime_error("Can not open setting file");
	}
	std::string line;
	std::vector<std::vector<std::string>> settings;
	while (std::getline(infile, line)) {
		std::istringstream iss(line);
		std::vector<std::string> res((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
		if(res.size() == 1) {
			if(res[0] == "parse_subdomain") {
				param_subdomain = true;
			} else if(res[0] == "log_redirect") {
				log_redirect = true;
			} else if(res[0] == "log_error_reply") {
				log_error_reply = true;
			} else if(res[0] == "log_ignored_url") {
				log_ignored_url = true;
			} else if(res[0] == "log_parse_url") {
				log_parse_url = true;
			} else if(res[0] == "log_other") {
				log_other = true;
			} else if(res[0] == "log_info") {
				log_info = true;
			} else if(res[0] == "debug") {
				param_debug = true;
			} else if(res[0] == "cert_verification") {
				cert_verification = true;
			}
		} else if(res.size() == 2) {
			if(res[0] == "url") {
				param_url = res[1];
				url = param_url;
			} else if(res[0] == "xml_name") {
				xml_name = res[1];
			} else if(res[0] == "thread") {
				int index = std::stoi(res[1]);
				if(index >= 1 && index <= 10) {
					thread_cnt = index;
				}
			} else if(res[0] == "dir") {
				dir = res[1];
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
			} else if(res[0] == "cell_delim") {
				cell_delim = res[1];
			} else if(res[0] == "in_cell_delim") {
				in_cell_delim = res[1];
			} else if(res[0] == "ca_cert_file_path") {
				ca_cert_file_path = res[1];
			} else if(res[0] == "ca_cert_dir_path") {
				ca_cert_dir_path = res[1];
			}
		} else if(res[0] == "filter" && res.size() == 4) {
			Filter f;
			if(res[1] == "regexp") {
				f.type = Filter::type_regexp;
			} else if(res[1] == "get") {
				f.type = Filter::type_get;
			} else if(res[1] == "ext") {
				f.type = Filter::type_ext;
			}
			if(res[2] == "include") {
				f.dir = Filter::include;
			} else {
				f.dir = Filter::exclude;
			}
			f.val = res[3];
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
	infile.close();
	if(dir.empty()) {
		throw std::runtime_error("Parameter 'dir' is empty");
	}
	std::regex reg("http(s)?:\\/\\/(www\\.)?[\\w-]+\\.[\\w-]+.*", std::regex_constants::ECMAScript | std::regex_constants::icase);
	if(!std::regex_match(param_url, reg)) {
		throw std::runtime_error("Parameter 'url' is not valid");
	}
}

std::string Main::url_path(const Url& url) {
	Url n;
	auto path = url.path();
	if(path.empty()) {
		path = "/";
	}
	n.path(path);
	n.set_query(url.query());
	return n.str();
}

std::string Main::url_normalize(const Url& url) {
	Url d(url);
	// append trailing /
	if(d.path().empty()) {
		d.path("/");
	} else if (d.path().back() != '/') {
		auto path = d.path();
		path.append("/");
		d.path(path);
	}
	// sort query
	std::set<std::pair<std::string, std::string>> q;
	for (auto n : d.query()) {
		std::pair<std::string, std::string> p(n.key(), n.val());
		q.insert(p);
	}
	d.set_query().clear();
	for(auto it = q.begin() ; it != q.end(); ++it) {
		d.add_query(it->first, it->second);
	}
	//  remove fragment
	d.fragment("");
	return d.str();
}

void Main::start() {
	Url d(param_url);
	Url_struct url_obj;
	url_obj.found = param_url;
	url_obj.remote.push_back(d.str());
	url_obj.path = url_path(d);
	url_obj.normalize = url_normalize(d);
	url_obj.ssl = d.scheme() == "https";
	url_obj.host = d.host();
	set_url(url_obj);

	std::thread t([this]() {
		std::string ch;
		while (1) {
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

	std::vector<std::thread> threads;
	for(int i = 1; i <= thread_cnt; i++) {
		Thread worker;
		worker.main = this;
		worker.id = i;
		threads.emplace_back(&Thread::load, worker);
	}
	for (auto &thread : threads) {
		thread.join();
	}
}

bool Main::set_url(Url_struct& url) {
	std::unique_lock<std::mutex> lk(mutex);
	if(url_limit && url_all.size() >= url_limit) {
		if(!url_lim_reached) {
			log("other", "URL limit reached");
			url_lim_reached = true;
		}
		return false;
	}
	auto it = url_all.find(url.normalize);
	if(it == url_all.end()) {
		url.id = url_all.size() + 1;
		url_all[url.normalize] = url;
		url_queue.push(url);
		lk.unlock();
		cond.notify_one();
	}
	return true;
}

void Main::redirect_url(const std::string& norm, const Url_struct& url) {
	std::unique_lock<std::mutex> lk(mutex);
	auto it = url_all.find(norm);
	if(it != url_all.end()) {
		if(it->second.remote.size() > redirect_limit) {
			it->second.remote.push_back("LIMIT");
			return;
		}
		it->second.remote.push_back(url.remote.back());
		it->second.path = url.path;
		it->second.host = url.host;
		it->second.ssl = url.ssl;
		url_queue.push(it->second);
		lk.unlock();
		cond.notify_one();
	}
}

void Main::try_again(const std::string& norm) {
	std::unique_lock<std::mutex> lk(mutex);
	auto it = url_all.find(norm);
	if(it != url_all.end()) {
		it->second.try_cnt++;
		url_queue.push(it->second);
		lk.unlock();
		cond.notify_one();
	}
}

void Main::update_url(const Url_struct& url) {
	std::lock_guard<std::mutex> lk(mutex);
	auto it = url_all.find(url.normalize);
	if(it != url_all.end()) {
		it->second.is_html = url.is_html;
		it->second.charset = url.charset;
		it->second.time = url.time;
	}
}

bool Main::get_url(Thread* t) {
	std::unique_lock<std::mutex> lk(mutex);
	if (!running) {
		debug(std::to_string(t->id) + " not running " + std::to_string(thread_work) + " " + std::to_string(url_queue.size()));
		return false;
	}
	if(url_queue.size()) {
		if(t->suspend) {
			t->suspend = 0;
			thread_work++;
			debug(std::to_string(t->id) + " revive " + std::to_string(thread_work) + " " + std::to_string(url_queue.size()));
		}
		t->m_url = url_queue.front();
		url_queue.pop();
		return true;
	}
	if(!t->suspend) {
		t->suspend = 1;
		thread_work--;
		if(thread_work == 0) {
			debug(std::to_string(t->id) + " exit " + std::to_string(thread_work) + " " + std::to_string(url_queue.size()));
			running = false;
			lk.unlock();
			cond.notify_all();
			return false;
		}
	}
	debug(std::to_string(t->id) + " wait " + std::to_string(thread_work) + " " + std::to_string(url_queue.size()));
	cond.wait(lk, [this] {
		return !running || url_queue.size();
	});
	return true;
}

void Main::debug(const std::string& msg) {
	if(param_debug) {
		std::lock_guard<std::mutex> lock(mutex_log);
		std::cout << msg << std::endl;
	}
}

void Main::log(const std::string& file, const std::string& msg) {
	std::lock_guard<std::mutex> lock(mutex_log);
	if(dir.empty()) {
		std::cout << msg << std::endl;
		return;
	}
	std::ofstream ofs (dir + "/" + file + ".log", std::ofstream::out | std::ofstream::app);
	if (!ofs.is_open()) {
		std::cout << "Can not open " << dir + "/" + file + ".log" << std::endl;
		return;
	}
	ofs << msg << std::endl;
	ofs.close();
}

bool Main::handle_url(Url_struct& url_new, const std::string& found, const std::string& base_href, const std::string& parent) {
	try {

		std::string dest(found);
		dest = std::regex_replace(dest, std::regex("^\\s+|\\s+$"), std::string(""));

		// ----- adjust & base filter
		Url d(dest);
		Url b(base_href);
		
		// empty scheme
		if(d.scheme().empty()) {
			d.scheme(b.scheme());
		} else if(d.scheme().find("http") != 0) {
			return false;
		}
		// empty host
		if(d.host().empty()) {
			d.host(b.host());
		}
		// resolve path
		if (!d.path().empty() && d.path().front() != '/') {
			auto b_path = Utils::split(b.path(), '/');
			auto d_path = Utils::split(d.path(), '/');
			for (auto n : d_path) {
				b_path.push_back(n);
			}
			d.path(Utils::join(b_path, '/').insert(0, "/"));
		}

		// ----- user filter
		auto d_host = d.host();
		auto b_host = url.host();
		std::size_t i = d_host.find(b_host);
		if (i == std::string::npos || i != d_host.size() - b_host.size()) {
			return false;
		}
		if(!param_subdomain && d_host != b_host) {
			return false;
		}
		for(auto it_filter = param_filter.begin() ; it_filter != param_filter.end(); ++it_filter) {
			bool res = true;
			bool check = false;
			if((*it_filter).type == Filter::type_regexp) {
				std::regex reg((*it_filter).val, std::regex_constants::ECMAScript | std::regex_constants::icase);
				check = true;
				res = std::regex_search(d.str(), reg);
			} else if((*it_filter).type == Filter::type_get) {
				if(!d.query().empty()) {
					check = true;
					res = false;
					for (auto n : d.query()) {
						if(n.key() == (*it_filter).val) {
							res = true;
							break;
						}
					}
				}
			} else if((*it_filter).type == Filter::type_ext) {
				std::string elem;
				std::string ext;
				auto elems = Utils::split(d.path(), '/');
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
			}
		}
		url_new.parent = parent;
		url_new.found = found;
		url_new.path = url_path(d);
		url_new.normalize = url_normalize(d);
		url_new.remote.push_back(d.str());
		url_new.ssl = d.scheme() == "https";
		url_new.host = d.host();
		return true;
	}
	catch (Url::parse_error& e) {
		if(log_parse_url) {
			CSV_Writer csv(cell_delim);
			csv << e.what() << found << base_href << parent;
			log("parse_url", csv.to_string());
		}
		return false;
	}
	catch (Url::build_error& e) {
		if(log_parse_url) {
			CSV_Writer csv(cell_delim);
			csv << e.what() << found << base_href << parent;
			log("parse_url", csv.to_string());
		}
		return false;
	}
}

void Main::finished() {
	for(auto it = url_all.begin() ; it != url_all.end(); ++it) {
		if(log_redirect && it->second.remote.size() > 1) {
			CSV_Writer csv(cell_delim);
			csv.add(Utils::join(it->second.remote, in_cell_delim.at(0)));
			csv.add(it->second.parent);
			log("redirect", csv.to_string());
		}
		if(log_info) {
			CSV_Writer csv(cell_delim);
			csv.add(std::to_string(it->second.time));
			csv.add(std::to_string(it->second.is_html));
			csv.add(std::to_string(it->second.try_cnt));
			csv.add(std::to_string(it->second.remote.size()));
			csv.add(it->second.charset);
			csv.add(it->second.found);
			csv.add(Utils::join(it->second.remote, in_cell_delim.at(0)));
			csv.add(it->second.parent);
			log("info", csv.to_string());
		}
	}
	if(!xml_name.empty()) {
		int i = 1;
		int j = 0;
		std::streamoff wrap_length = 0;
		std::streamoff tag_length = 0;
		std::streamoff pos = 0;
		std::ofstream out(dir + "/" + xml_name + std::to_string(i) + ".xml", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
		std::streamoff outpos = 0;
		XML_writer writer(out);
		if(!out.is_open()) {
			throw std::runtime_error("Can not open output file");
		}
		writer.write_start_doc();
		writer.write_start_el("urlset");
		writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
		writer.write_end_el();
		writer.write_end_doc();
		writer.flush();
		wrap_length = out.tellp();
		wrap_length += 1;
		out.close();
		out.open(dir + "/" + xml_name + std::to_string(i) + ".xml", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
		writer.write_start_doc();
		writer.write_start_el("urlset");
		writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
		for(auto it = url_all.begin() ; it != url_all.end(); ++it) {
			if(!it->second.is_html) {
				continue;
			}
			int str_size = 0;
			std::vector<std::pair<std::string, std::string>> tags;
			tags.emplace_back("loc", writer.escape_str(it->second.remote.front()));
			str_size += tags.back().second.size();
			for(auto it1 = param_xml_tag.begin(); it1 != param_xml_tag.end(); ++it1) {
				tags.emplace_back(it1->first, writer.escape_str(it1->second.def));
				for(auto it2 = it1->second.regexp.begin() ; it2 != it1->second.regexp.end(); ++it2) {
					std::regex reg(it2->second, std::regex_constants::ECMAScript | std::regex_constants::icase);
					if(std::regex_search(it->second.remote.front(), reg)) {
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
				out.close();
				out.open(dir + "/" + xml_name + std::to_string(i) + ".xml", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
				writer.write_start_doc();
				writer.write_start_el("urlset");
				writer.write_attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");
			}
			if(!tag_length) {
				outpos = out.tellp();
			}
			writer.write_start_el("url");
			for(auto it1 = tags.begin(); it1 != tags.end(); ++it1) {
				writer.write_start_el(it1->first);
				writer.write_str(it1->second, false);
				writer.write_end_el();
			}
			writer.write_end_el();
			if(!tag_length) {
				tag_length = out.tellp() - outpos;
				tag_length -= str_size;
				tag_length -= 1;
			}
			pos += tag_length + str_size;
			j++;
		}
		writer.write_end_el();
		writer.write_end_doc();
		writer.flush();
		out.close();
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
			std::shared_ptr<httplib::Client> cli;
			if(m_url.ssl) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
				cli = std::make_shared<httplib::SSLClient>(m_url.host.c_str());
				httplib::SSLClient& rcli = static_cast<httplib::SSLClient &>(*cli);
				rcli.enable_server_certificate_verification(main->cert_verification);
				if(main->cert_verification) {
					if(!main->ca_cert_file_path.empty()) {
						rcli.set_ca_cert_path(main->ca_cert_file_path.c_str());
					}
					if(!main->ca_cert_dir_path.empty()) {
						rcli.set_ca_cert_path(nullptr, main->ca_cert_dir_path.c_str());
					}
				}
#else
				CSV_Writer csv(main->cell_delim);
				csv << "HTTPS not supported" << Utils::join(m_url.remote, main->in_cell_delim.at(0)) << m_url.parent;
				main->log("error_reply", csv.to_string());
				continue;
#endif
			} else {
				cli = std::make_shared<httplib::Client>(m_url.host.c_str());
			}
			Timer tmr;
			reply = cli->Get(m_url.path.c_str());
			m_url.time += tmr.elapsed();
			main->debug(std::to_string(id) + " " + std::to_string(m_url.time) + " " + m_url.remote.back());
			main->update_url(m_url);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
			if(m_url.ssl) {
				auto res = static_cast<httplib::SSLClient &>(*cli).get_openssl_verify_result();
				if(res != X509_V_OK) {
					CSV_Writer csv(main->cell_delim);
					csv << std::string("Certificate verification error: ") + X509_verify_cert_error_string(res) << Utils::join(m_url.remote, main->in_cell_delim.at(0)) << m_url.parent;
					main->log("error_reply", csv.to_string());
					continue;
				}
			}
#endif
			http_finished();
			if(main->param_sleep) {
				std::this_thread::sleep_for(std::chrono::seconds(main->param_sleep));
			}
		}
	} catch(...) {
		exc_ptr = std::current_exception();
		{
			std::lock_guard<std::mutex> lk(main->mutex);
			main->running = false;
		}
		main->cond.notify_all();
	}
}

void Thread::http_finished() {
	if (!reply) {
		if(m_url.try_cnt < main->try_limit) {
			main->try_again(m_url.normalize);
		} else if(main->log_error_reply) {
			CSV_Writer csv(main->cell_delim);
			csv << "No reply" << Utils::join(m_url.remote, main->in_cell_delim.at(0)) << m_url.parent;
			main->log("error_reply", csv.to_string());
		}
		return;
	}
	if(reply->status != 200 && !(reply->status >= 300 && reply->status < 400)) {
		if(m_url.try_cnt < main->try_limit) {
			main->try_again(m_url.normalize);
		} else if(main->log_error_reply) {
			CSV_Writer csv(main->cell_delim);
			csv << std::string("Code: ") + std::to_string(reply->status) << Utils::join(m_url.remote, main->in_cell_delim.at(0)) << m_url.parent;
			main->log("error_reply", csv.to_string());
		}
		return;
	}
	if(reply->has_header("Location") && reply->status >= 300 && reply->status < 400) {
		auto url = reply->get_header_value("Location");
		Url_struct new_url;
		if(!main->handle_url(new_url, url, m_url.remote.back(), m_url.remote.back())) {
			if(main->log_ignored_url) {
				CSV_Writer csv(main->cell_delim);
				csv << url << m_url.remote.back();
				main->log("ignored_url", csv.to_string());
			}
			return;
		}
		main->redirect_url(m_url.normalize, new_url);
		return;
	}
	if(!reply->has_header("Content-Type")) {
		return;
	}
	// check html
	auto content_type = reply->get_header_value("Content-Type");
	if(content_type.find("text/html") == std::string::npos) {
		return;
	}
	m_url.is_html = true;
	// charset from header
	auto pos = content_type.find("charset=");
	if(pos != std::string::npos) {
		m_url.charset = content_type.substr(pos + 8);
	}
	// parse
	html::dom h;
	h.append_partial_html(reply->body);
	// base href
	auto tag_base = h["base[href]"].get_children();
	std::string base_href(m_url.remote.back());
	if(!tag_base.empty()) {
		base_href = tag_base[0]->get_attr("href");
	}
	// charset from meta
	if (m_url.charset.empty()) {
		auto tag_meta = h["meta[http-equiv='Content-Type']"].get_children();
		if(!tag_meta.empty()) {
			auto content = tag_meta[0]->get_attr("content");
			if(!content.empty()) {
				auto pos = content.find("charset=");
				if(pos != std::string::npos) {
					m_url.charset = content.substr(pos + 8);
				}
			}
		}
	}
	// update
	main->update_url(m_url);
	// find links
	auto tag_a = h["a[href!='#']"];
	for( auto & a : tag_a.get_children()) {
		Url_struct new_url;
		auto href = a->get_attr("href");
		if(main->handle_url(new_url, href, base_href, m_url.remote.back())) {
			if(!main->set_url(new_url)) {
				break;
			}
		} else if(main->log_ignored_url) {
			CSV_Writer csv(main->cell_delim);
			csv << href << m_url.remote.back();
			main->log("ignored_url", csv.to_string());
		}
	}
}

int main(int argc, char *argv[]) {
	Main c;
	try {
		if(argc != 2) {
			std::cout << "Specify setting file" << std::endl;
			return 0;
		}
		c.import_param(argv[1]);
		c.start();
		if (exc_ptr) {
			std::rethrow_exception(exc_ptr);
		}
		c.finished();
	} catch (std::exception& e) {
		if(c.log_other) {
			CSV_Writer csv(c.cell_delim);
			csv.add(e.what());
			c.log("other_error", csv.to_string());
		}
	}
	return 0;
}