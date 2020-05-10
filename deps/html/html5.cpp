template<typename CharType> const CharType* comment_tag_string();
template<> const char* comment_tag_string<char>(){ return "<!--"; }
template<> const wchar_t* comment_tag_string<wchar_t>(){ return L"<!--"; }

template<typename CharType> const CharType* id_tag_string();
template<> const char* id_tag_string<char>(){ return "id"; }
template<> const wchar_t* id_tag_string<wchar_t>(){ return L"id"; }

template<typename CharType> const CharType* class_tag_string();
template<> const char* class_tag_string<char>(){ return "class"; }
template<> const wchar_t* class_tag_string<wchar_t>(){ return L"class"; }

template<typename CharType> const CharType* script_tag_string();
template<> const char* script_tag_string<char>(){ return "script"; }
template<> const wchar_t* script_tag_string<wchar_t>(){ return L"script"; }


template<typename CharType> const CharType* operator_string_contain();
template<> const char* operator_string_contain<char>(){ return "$="; }
template<> const wchar_t* operator_string_contain<wchar_t>(){ return L"$="; }

template<typename CharType> const CharType* operator_string_inequalityt();
template<> const char* operator_string_inequalityt<char>(){ return "!="; }
template<> const wchar_t* operator_string_inequalityt<wchar_t>(){ return L"!="; }

template<typename CharType> const CharType* operator_string_equalityt();
template<> const char* operator_string_equalityt<char>(){ return "="; }
template<> const wchar_t* operator_string_equalityt<wchar_t>(){ return L"="; }

template<typename CharType> const CharType* selector_empty_string();
template<> const char* selector_empty_string<char>(){ return "#"; }
template<> const wchar_t* selector_empty_string<wchar_t>(){ return L"#"; }

template<typename CharType> const CharType* operator_string_first();
template<> const char* operator_string_first<char>(){ return "first"; }
template<> const wchar_t* operator_string_first<wchar_t>(){ return L"first"; }

template<typename CharType> const CharType* operator_string_last();
template<> const char* operator_string_last<char>(){ return "last"; }
template<> const wchar_t* operator_string_last<wchar_t>(){ return L"last"; }

template<typename CharType> const CharType* string_eq();
template<> const char* string_eq<char>(){ return "eq"; }
template<> const wchar_t* string_eq<wchar_t>(){ return L"eq"; }

template<typename CharType> const CharType* string_qt();
template<> const char* string_qt<char>(){ return "qt"; }
template<> const wchar_t* string_qt<wchar_t>(){ return L"qt"; }

template<typename CharType> const CharType* string_lt();
template<> const char* string_lt<char>(){ return "lt"; }
template<> const wchar_t* string_lt<wchar_t>(){ return L"lt"; }

#include "html5.hpp"

template<typename CharType>
html::basic_selector<CharType>::basic_selector(const std::basic_string<CharType>& s)
	: m_select_string(s)
{
	build_matchers();
}

template<typename CharType>
html::basic_selector<CharType>::basic_selector(std::basic_string<CharType>&&s)
	: m_select_string(s)
{
	build_matchers();
}

template html::basic_selector<char>::basic_selector(std::basic_string<char>&&s);
template html::basic_selector<wchar_t>::basic_selector(std::basic_string<wchar_t>&&s);

template<typename CharType>
bool strcmp_ignore_case(const std::basic_string<CharType>& a, const std::basic_string<CharType>& b)
{
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}


template bool strcmp_ignore_case(const std::basic_string<char>& a, const std::basic_string<char>& b);
template bool strcmp_ignore_case(const std::basic_string<wchar_t>& a, const std::basic_string<wchar_t>& b);

template<typename CharType>
void html::basic_selector<CharType>::build_matchers()
{
	selector_matcher matcher;

	if (m_select_string[0] == '*')
	{
		matcher.all_match = true;
		// 所有的类型
		m_matchers.push_back(matcher);
		return;
	}
	// 从 选择字符构建匹配链
	auto str_iterator = m_select_string.begin();

	int state = 0;

	auto getc = [this, &str_iterator]() -> CharType {
		if (str_iterator != m_select_string.end())
			return *str_iterator++;
		return 0;
	};

	auto get_escape = [&getc]() -> CharType
	{
		return getc();
	};

	std::basic_string<CharType> matcher_str;

	CharType c;

	do
	{
		c = getc();
		switch(state)
		{
			case 0:
			case '#':
			case '.':
				switch(c)
				{
					case '\\':
						matcher_str += get_escape();
						break;
					case 0 : case '.' : case '#': case ' ': case '[': case ':':
						{
							if (!matcher_str.empty())
							{
								condition match_condition;
								switch(state)
								{
									case 0:
										match_condition.matching_tag_name = std::move(matcher_str);
										break;
									case '#':
										match_condition.matching_id = std::move(matcher_str);
										break;
									case '.':
										match_condition.matching_class = std::move(matcher_str);
										break;
								}
								matcher.m_conditions.push_back(match_condition);
							}
							state = c;

							if ( c == ' ')
								state = 0;

							if (state == 0)
							{
								m_matchers.push_back(std::move(matcher));
							}
						}
						break;
					default:
						matcher_str += c;
				}
			break;
			case ':':
			{
				switch (c)
				{
				case 0: case ' ': {
					state = 0;
					condition match_condition;
					int tag_type = 0;
					std::for_each(matcher_str.begin(), matcher_str.end(), [&match_condition, &tag_type](const CharType Word){
						if (Word == '(' || Word == ')')
							tag_type = 1;
						else if (0 == tag_type)
							match_condition.matching_attr_operator += Word;
						else if (1 == tag_type)
							match_condition.matching_index += Word;
					});
					matcher_str.clear();
					matcher.m_conditions.push_back(match_condition);
					m_matchers.push_back(std::move(matcher));
					break;
				}
				default:
					matcher_str += c;
					break;
				}

			}
			break;
			case '[':
			{
				switch(c)
				{
					case ']': {
						state = 0;
						condition match_condition;
						bool attr = false;
						std::for_each(matcher_str.begin(), matcher_str.end(), [&match_condition, &attr](const CharType C){
							if (C == '=' || C == '$' || C == '!')
							{
								attr = true;
								match_condition.matching_attr_operator += C;
							}
							else if ( C != '\'' )
								(attr ? match_condition.matching_attr_value : match_condition.matching_attr) += C;
						});
						matcher_str.clear();
						matcher.m_conditions.push_back(match_condition);
						break;
					}
					default:
						matcher_str += c;
				}
			}
			break;
		}
	} while(c);
}

template void html::basic_selector<char>::build_matchers();
template void html::basic_selector<wchar_t>::build_matchers();

template<typename CharType>
html::basic_dom<CharType>::basic_dom(html::basic_dom<CharType>* parent) noexcept
	: m_parent(parent)
{
}

template html::basic_dom<char>::basic_dom(html::basic_dom<char>* parent) noexcept;
template html::basic_dom<wchar_t>::basic_dom(html::basic_dom<wchar_t>* parent) noexcept;


template<typename CharType>
html::basic_dom<CharType>::basic_dom(const std::basic_string<CharType>& html_page, html::basic_dom<CharType>* parent)
	: basic_dom(parent)
{
	append_partial_html(html_page);
}

template html::basic_dom<char>::basic_dom(const std::basic_string<char>& html_page, html::basic_dom<char>* parent);
template html::basic_dom<wchar_t>::basic_dom(const std::basic_string<wchar_t>& html_page, html::basic_dom<wchar_t>* parent);

template<typename CharType>
html::basic_dom<CharType>::basic_dom(html::basic_dom<CharType>&& d)
	: attributes(std::move(d.attributes))
	, tag_name(std::move(d.tag_name))
	, content_text(std::move(d.content_text))
	, m_parent(std::move(d.m_parent))
	, children(std::move(d.children))
{
}

template html::basic_dom<char>::basic_dom(html::basic_dom<char>&& d);
template html::basic_dom<wchar_t>::basic_dom(html::basic_dom<wchar_t>&& d);

template<typename CharType>
html::basic_dom<CharType>::basic_dom(const html::basic_dom<CharType>& d)
	: attributes(d.attributes)
	, tag_name(d.tag_name)
	, content_text(d.content_text)
	, m_parent(d.m_parent)
	, children(d.children)
{
}

template html::basic_dom<char>::basic_dom(const html::basic_dom<char>& d);
template html::basic_dom<wchar_t>::basic_dom(const html::basic_dom<wchar_t>& d);

template<typename CharType>
html::basic_dom<CharType>& html::basic_dom<CharType>::operator=(const html::basic_dom<CharType>& d)
{
	attributes = d.attributes;
	tag_name = d.tag_name;
	content_text = d.content_text;
	m_parent = d.m_parent;
	children = d.children;
	return *this;
}

template html::basic_dom<char>& html::basic_dom<char>::operator=(const html::basic_dom<char>& d);
template html::basic_dom<wchar_t>& html::basic_dom<wchar_t>::operator=(const html::basic_dom<wchar_t>& d);

template<typename CharType>
html::basic_dom<CharType>& html::basic_dom<CharType>::operator=(html::basic_dom<CharType>&& d)
{
	attributes = std::move(d.attributes);
	tag_name = std::move(d.tag_name);
	content_text = std::move(d.content_text);
	m_parent = std::move(d.m_parent);
	children = std::move(d.children);
	return *this;
}

template html::basic_dom<char>& html::basic_dom<char>::operator=(html::basic_dom<char>&& d);
template html::basic_dom<wchar_t>& html::basic_dom<wchar_t>::operator=(html::basic_dom<wchar_t>&& d);

template<typename CharType>
html::detail::basic_dom_node_parser<CharType>::basic_dom_node_parser(html::basic_dom<CharType>* domer, const std::basic_string<CharType>& str)
	: m_dom(domer)
	, m_str(str)
{
	m_dom->node_parser = this;
}

template html::detail::basic_dom_node_parser<char>::basic_dom_node_parser(html::basic_dom<char>* domer, const std::basic_string<char>& str);
template html::detail::basic_dom_node_parser<wchar_t>::basic_dom_node_parser(html::basic_dom<wchar_t>* domer, const std::basic_string<wchar_t>& str);

template<typename CharType>
html::detail::basic_dom_node_parser<CharType>::basic_dom_node_parser(const basic_dom_node_parser& other)
	: m_str(other.m_str)
	, m_callback(other.m_callback)
{
	m_dom = nullptr;
}

template html::detail::basic_dom_node_parser<char>::basic_dom_node_parser(const basic_dom_node_parser& other);
template html::detail::basic_dom_node_parser<wchar_t>::basic_dom_node_parser(const basic_dom_node_parser& other);

template<typename CharType>
html::detail::basic_dom_node_parser<CharType>::basic_dom_node_parser(basic_dom_node_parser&& other)
	: m_str(other.m_str)
	, m_callback(std::move(other.m_callback))
{
	m_dom = other.m_dom;
	other.m_dom = nullptr;
}

template html::detail::basic_dom_node_parser<char>::basic_dom_node_parser(basic_dom_node_parser&& other);
template html::detail::basic_dom_node_parser<wchar_t>::basic_dom_node_parser(basic_dom_node_parser&& other);

template<typename CharType>
html::detail::basic_dom_node_parser<CharType>::~basic_dom_node_parser()
{
	if (m_dom) {
		m_dom->html_parser(&m_str);
		m_dom->node_parser = nullptr;
	}
}

template html::detail::basic_dom_node_parser<char>::~basic_dom_node_parser();
template html::detail::basic_dom_node_parser<wchar_t>::~basic_dom_node_parser();

template<typename CharType>
void html::detail::basic_dom_node_parser<CharType>::operator()(tag_stage s, std::shared_ptr<basic_dom<CharType>> nodeptr)
{
	if (m_selector == nullptr)
	{
		if (m_callback)
			m_callback(s, nodeptr);
		return;
	}

	auto macher_it = m_selector->begin();

	if ((*macher_it)(*nodeptr))
	{
		++macher_it;
	}

	if (macher_it == m_selector->end())
	{
		m_callback(s, nodeptr);
	}

	m_callback(s, nodeptr);
}

template<typename CharType>
void html::detail::basic_dom_node_parser<CharType>::set_callback_fuction(std::function<void(tag_stage, std::shared_ptr<html::basic_dom<CharType>>)>&& cb)
{
	m_callback = cb;
}

template void html::detail::basic_dom_node_parser<char>::set_callback_fuction(std::function<void(tag_stage, std::shared_ptr<html::basic_dom<char>>)>&& cb);
template void html::detail::basic_dom_node_parser<wchar_t>::set_callback_fuction(std::function<void(tag_stage, std::shared_ptr<html::basic_dom<wchar_t>>)>&& cb);

template<typename CharType>
html::detail::basic_dom_node_parser<CharType>& html::detail::basic_dom_node_parser<CharType>::operator | (const basic_selector<CharType>& selector_)
{
	m_selector = & selector_;
	return *this;
}

template html::detail::basic_dom_node_parser<char>& html::detail::basic_dom_node_parser<char>::operator | (const basic_selector<char>& selector_);
template html::detail::basic_dom_node_parser<wchar_t>& html::detail::basic_dom_node_parser<wchar_t>::operator | (const basic_selector<wchar_t>& selector_);

template<typename CharType>
html::detail::basic_dom_node_parser<CharType> html::basic_dom<CharType>::append_partial_html(const std::basic_string<CharType>& str)
{
	return detail::basic_dom_node_parser<CharType>(this, str);
}

template html::detail::basic_dom_node_parser<char> html::basic_dom<char>::append_partial_html(const std::basic_string<char>& str);
template html::detail::basic_dom_node_parser<wchar_t> html::basic_dom<wchar_t>::append_partial_html(const std::basic_string<wchar_t>& str);

template<typename CharType> template<class Handler>
void html::basic_dom<CharType>::dom_walk(std::shared_ptr<html::basic_dom<CharType>> d, Handler handler)
{
	if(handler(d))
		for (auto & c : d->children)
		{
			if (c->tag_name !=  comment_tag_string<CharType>())
				dom_walk(c, handler);
		}
}

template<typename CharType>
bool html::basic_selector<CharType>::condition::operator()(const html::basic_dom<CharType>& d) const
{
	if (!matching_tag_name.empty())
	{
		return strcmp_ignore_case(d.tag_name, matching_tag_name);
	}
	if (!matching_id.empty())
	{
		auto it = d.attributes.find(id_tag_string<CharType>());
		if ( it != d.attributes.end())
		{
			return it->second == matching_id;
		}
	}
	if (!matching_class.empty())
	{
		auto it = d.attributes.find(class_tag_string<CharType>());
		if ( it != d.attributes.end())
		{
			return it->second == matching_class;
		}
	}

	if (matching_attr_operator == operator_string_first<CharType>())
	{
		return d.index == 0;
	}

	if (matching_attr_operator == operator_string_last<CharType>())
	{
		return d.index == d.m_parent->node_count - 1;
	}

	if (matching_attr_operator == string_eq<CharType>())
	{
		return d.index == std::stoi(matching_index);
    }

    if (matching_attr_operator == string_qt<CharType>())
    {
        return d.index > std::stoi(matching_index);
    }

    if (matching_attr_operator == string_lt<CharType>())
    {
        return d.index < std::stoi(matching_index);
    }

	if (!matching_attr.empty())
	{
		auto it = d.attributes.find(matching_attr);
		if (it == d.attributes.end()) return false;

		if (matching_attr_operator == operator_string_equalityt<CharType>())
			return strcmp_ignore_case(it->second, matching_attr_value);
		else if (matching_attr_operator == operator_string_contain<CharType>())
		{
			if (matching_attr_value == selector_empty_string<CharType>()) return it->second.empty();
			else
			{
				bool find_result = it->second.find(matching_attr_value) != std::basic_string<CharType>::npos;
				return find_result;
			}
		}
		else if (matching_attr_operator == operator_string_inequalityt<CharType>())
		{
			if (matching_attr_value == selector_empty_string<CharType>()) return !it->second.empty();
			else
			{
				bool find_result = it->second.find(matching_attr_value) == std::basic_string<CharType>::npos;
				return find_result;
			}
		}
		else
		{
			// 只要存在就可以了
			return true;
		}
	}
	return false;
}

template<typename CharType>
bool html::basic_selector<CharType>::selector_matcher::operator()(const html::basic_dom<CharType>& d) const
{
	if (this->all_match)
		return true;

	for (auto& c : m_conditions)
	{
		if(c(d))
		{
			continue;
		}
		return false;
	}

	return true;
}

template<typename CharType>
html::basic_dom<CharType> html::basic_dom<CharType>::operator[](const basic_selector<CharType>& selector_) const
{
	html::basic_dom<CharType> selectee_dom;
	html::basic_dom<CharType> matched_dom(*this);
    int i = 0;

	for (auto & matcher : selector_)
    {
        selectee_dom = std::move(matched_dom);
        if(i++) {
            html::basic_dom<CharType> tmp_dom;
            for( auto & a : selectee_dom.children) {
                for( auto & b : a->children) {
                    tmp_dom.children.push_back(b);
                }
            }
            selectee_dom = std::move(tmp_dom);
        }

		for( auto & c : selectee_dom.children)
        {
            dom_walk(c, [this, &matcher, &matched_dom](std::shared_ptr<html::basic_dom<CharType>> i)
            {
                if (matcher(*i))
                {
                    matched_dom.children.push_back(i);
                    return false;
                }
                return true;
            });
        }
	}

	return matched_dom;
}

template html::basic_dom<char> html::basic_dom<char>::operator[](const basic_selector<char>& selector_) const;
template html::basic_dom<wchar_t> html::basic_dom<wchar_t>::operator[](const basic_selector<wchar_t>& selector_) const;

template<typename CharType>
static std::basic_string<CharType> basic_literal(const char* literal);

template<>
std::basic_string<char> basic_literal(const char* literal)
{
	return literal;
};

template<>
std::basic_string<wchar_t> basic_literal(const char* literal)
{
	return std::wstring(literal, literal + strlen(literal));
};


template<typename CharType>
static inline std::basic_string<CharType> get_char_set(const std::basic_string<CharType> type, const std::basic_string<CharType> & default_charset)
{
	std::match_results<const CharType*> m;
	std::basic_regex<CharType, std::regex_traits<CharType>> ex(basic_literal<CharType>("charset=([a-zA-Z0-9\\-_]+)"), std::regex_constants::ECMAScript | std::regex_constants::icase);
	if(std::regex_search(type.c_str(), m, ex)) {
		return m[1];
	} else if(std::regex_search(default_charset.c_str(), m, ex)) {
		return m[1];
	}
	return default_charset;
}

namespace html{
template<>
std::basic_string<char> basic_dom<char>::basic_charset(const std::string& default_charset) const
{
	auto charset_dom = (*this)["meta"];

	try {
		for (auto & c : charset_dom.children)
		{
			dom_walk(c, [this, &default_charset](std::shared_ptr<basic_dom<char>> i)
			{
				if (strcmp_ignore_case(i->get_attr("http-equiv"), std::string("content-type")))
				{
					auto content = i->get_attr("content");

					if (!content.empty())
					{
						auto cset = get_char_set(content, default_charset);

						throw cset;
					}
				}

				if (!i->get_attr("charset").empty())
				{
					throw i->get_attr("charset");
				}

				return true;
			});
		}
	}catch(const std::string& cset)
	{
		return cset;
	}

	return default_charset;
}
}

template<typename CharType>
std::basic_string<CharType> html::basic_dom<CharType>::to_plain_text() const
{
	std::basic_string<CharType> ret;

	if (!strcmp_ignore_case(tag_name, std::basic_string<CharType>(script_tag_string<CharType>())) && tag_name != comment_tag_string<CharType>())
	{
		ret += content_text;

		for ( auto & c : children)
		{
			ret += c->to_plain_text();
		}
	}

	return ret;
}

template std::basic_string<char> html::basic_dom<char>::to_plain_text() const;
template std::basic_string<wchar_t> html::basic_dom<wchar_t>::to_plain_text() const;

template<typename CharType>
void html::basic_dom<CharType>::to_html(std::basic_ostream<CharType>* out, int deep) const
{
	if (!tag_name.empty())
	{
		for (auto i = 0; i < deep; i++)
			*out << ' ';

		if (tag_name!= comment_tag_string<CharType>())
			*out << "<" << tag_name;
		else{
			*out << tag_name;
		}

		if (!attributes.empty())
		{
			for (auto a : attributes)
			{
				*out << ' ';
				*out << a.first << "=\"" << a.second << "\"";
			}
		}
		if (tag_name!=comment_tag_string<CharType>())
			*out << ">\n";
		else{
			*out << content_text;
			*out << "-->\n";
		}
	}else
	{
		for (auto i = 0; i < deep +1; i++)
			*out << ' ';
		*out << content_text << "\n";
	}

	for ( auto & c : children)
	{
		c->to_html(out, deep + 1);
	}

	if (!tag_name.empty())
	{
		if (tag_name!=comment_tag_string<CharType>())
		{
			for (auto i = 0; i < deep; i++)
				*out << ' ';
			*out << "</" << tag_name << ">\n";
		}
	}
}

template<typename CharType>
std::basic_string<CharType> html::basic_dom<CharType>::to_html() const
{
	std::basic_stringstream<CharType> ret;
	to_html(&ret, -1);
	return ret.str();
}

template std::basic_string<char> html::basic_dom<char>::to_html() const;
template std::basic_string<wchar_t> html::basic_dom<wchar_t>::to_html() const;

#define CASE_BLANK case ' ': case '\r': case '\n': case '\t'

template<typename CharType>
void html::basic_dom<CharType>::html_parser(const std::basic_string<CharType>* html_page_source)
{
	int pre_state = 0, state = 0;

	const std::basic_string<CharType> * _cur_str = html_page_source;
	typename std::basic_string<CharType>::const_iterator _cur_str_it;
	_cur_str_it = _cur_str->begin();

	auto getc = [&_cur_str, &_cur_str_it]() -> CharType {
		if (_cur_str_it!= _cur_str->end())
		{
			return *_cur_str_it++;
		}
		return 0;
	};

	auto get_escape = [&getc]() -> CharType
	{
		return getc();
	};

	auto get_string = [&getc, &get_escape, &pre_state, &state](CharType quote_char)
	{
		std::basic_string<CharType> ret;

		auto c = getc();

		while ( (c != quote_char) && (c!= '\n'))
		{
			if ( c == '\'' )
			{
				c += get_escape();
			}else
			{
				ret += c;
			}
			c = getc();
		}

		if (c == '\n')
		{
			pre_state = state;
			state = 0;

		}
		return ret;
	};

	std::basic_string<CharType> tag; //当前处理的 tag
	std::basic_string<CharType> content; // 当前 tag 下的内容
	std::basic_string<CharType> k,v;

	auto current_ptr = this;

	CharType c;

	std::vector<int> comment_stack;

	bool ignore_blank = false;

	do {
		c = getc();

		switch(state)
		{
			case 0: // 起始状态. content 状态
			{
				switch(c)
				{
					case '<':
					{
						if (tag.empty())
						{
							// 进入 < tag 解析状态
							pre_state = state;
							state = 1;
							if (!content.empty())
							{
								auto content_node = std::make_shared<basic_dom<CharType>>(current_ptr);
								content_node->content_text = std::move(content);
								current_ptr->children.push_back(std::move(content_node));

								(*this->node_parser)(tag_open, content_node);
								(*this->node_parser)(tag_close, content_node);
							}
						}
					}
					break;
					CASE_BLANK :
					{
						if (ignore_blank)
						{
							break;
						}else{
							ignore_blank = true;
							content += ' ';
						}
					}break;
					default:
						content += c;
						ignore_blank = false;
				}
			}
			break;
			case 1: // tag名字解析
			{
				switch (c)
				{
					CASE_BLANK :
					{
						if (tag.empty())
						{
							// empty tag name
							// 重新进入上一个 state
							// 也就是忽略 tag
							state = pre_state;
						}else
						{
							pre_state = state;
							state = 2;
							auto new_dom = std::make_shared<basic_dom<CharType>>(current_ptr);
							new_dom->tag_name = std::move(tag);
							new_dom->index = current_ptr->node_count++;
							current_ptr->children.push_back(new_dom);
							current_ptr = new_dom.get();
						}
					}
					break;
					case '>': // tag 解析完毕, 正式进入 下一个 tag
					{
						pre_state = state;
						state = 0;

						auto new_dom = std::make_shared<basic_dom<CharType>>(current_ptr);
						new_dom->tag_name = std::move(tag);
						new_dom->index = current_ptr->node_count++;
						current_ptr->children.push_back(new_dom);
						if(new_dom->tag_name[0] != '!')
							current_ptr = new_dom.get();
						if (strcmp_ignore_case(current_ptr->tag_name, std::basic_string<CharType>(script_tag_string<CharType>())))
						{
							state = 20;
						}
						(*this->node_parser)(tag_open, current_ptr->shared_from_this());
					}
					break;
					case '/':
						pre_state = state;
						state = 5;
					break;
					case '!':
						pre_state = state;
						state = 10;
					// 为 tag 赋值.
					default:
						tag += c;
				}
			}
			break;
			case 2: // tag 名字解析完毕, 进入 attribute 解析, skiping white
			{
				switch (c)
				{
					CASE_BLANK :
					break;
					case '>': // 马上就关闭 tag 了啊
					{
						// tag 解析完毕, 正式进入 下一个 tag
						pre_state = state;
						state = 0;
						(*this->node_parser)(tag_open, current_ptr->shared_from_this());
						if ( current_ptr->tag_name[0] == '!')
						{
							(*this->node_parser)(tag_close, current_ptr->shared_from_this());
							current_ptr = current_ptr->m_parent;
						}else if (strcmp_ignore_case(current_ptr->tag_name, std::basic_string<CharType>(script_tag_string<CharType>())))
						{
							state = 20;
						}

					}break;
					case '/':
					{
						// 直接关闭本 tag 了
						// 下一个必须是 '>'
						c = getc();

						if (c!= '>')
						{
							// TODO 报告错误
						}

						pre_state = state;
						state = 0;

						if (current_ptr->m_parent)
						{
							(*this->node_parser)(tag_close, current_ptr->shared_from_this());
							current_ptr = current_ptr->m_parent;
						}else
							current_ptr = this;
					}break;
					case '\"':
					case '\'':
					{
						pre_state = state;
						state = 3;
						k = get_string(c);
					}break;
					default:
						pre_state = state;
						state = 3;
						k += c;
				}
			}break;
			case 3: // tag 名字解析完毕, 进入 attribute 解析 key
			{
				switch (c)
				{
					CASE_BLANK :
					{
						// empty k=v
						state = 2;
						current_ptr->attributes[k];
						k.clear();
						v.clear();
					}
					break;
					case '=':
					{
						pre_state = state;
						state = 4;
					}break;
					case '>':
					{
						pre_state = state;
						state = 0;
						current_ptr->attributes[k];
						k.clear();
						v.clear();
						(*this->node_parser)(tag_open, current_ptr->shared_from_this());
						if ( current_ptr->tag_name[0] == '!')
						{
 							(*this->node_parser)(tag_close, current_ptr->shared_from_this());

							current_ptr = current_ptr->m_parent;

						}else if (strcmp_ignore_case(current_ptr->tag_name, std::basic_string<CharType>(script_tag_string<CharType>())))
						{
							state = 20;
						}
					}
					break;
					default:
						k += c;
				}
			}break;
			case 4: // 进入 attribute 解析 value
			{
				switch (c)
				{
					case '\"':
					case '\'':
					{
						v = get_string(c);
					}
					CASE_BLANK :
					{
						state = 2;
						current_ptr->attributes[k] = std::move(v);
						k.clear();
					}
					break;
					case '>':
					{
						pre_state = state;
						state = 0;
						current_ptr->attributes[k];
						k.clear();
						v.clear();

						(*this->node_parser)(tag_open, current_ptr->shared_from_this());

						if ( current_ptr->tag_name[0] == '!')
						{
							(*this->node_parser)(tag_close, current_ptr->shared_from_this());
							current_ptr = current_ptr->m_parent;
						}else if (strcmp_ignore_case(current_ptr->tag_name, std::basic_string<CharType>(script_tag_string<CharType>())))
						{
							state = 20;
						}
					}break;
					default:
						v += c;
				}
			}
			break;
			case 5: // 解析 </xxx>
			{
				switch(c)
				{
					case '>':
					{
						if(!tag.empty())
						{
							state = 0;
							// 来, 关闭 tag 了
							// 注意, HTML 里, tag 可以越级关闭

							// 因此需要进行回朔查找

							auto _current_ptr = current_ptr;

							while (_current_ptr && !strcmp_ignore_case(_current_ptr->tag_name, tag))
							{
								_current_ptr = _current_ptr->m_parent;
							}

							if (!_current_ptr)
							{
								// 找不到对应的 tag 要咋关闭... 忽略之
								tag.clear();

								break;
							}

							tag.clear();

							current_ptr = _current_ptr;

							// 找到了要关闭的 tag

							// 那就退出本 dom 节点
							if (current_ptr->m_parent)
							{
								(*this->node_parser)(tag_close, current_ptr->shared_from_this());
								current_ptr = current_ptr->m_parent;
							}
							else
								current_ptr = this;
						}else
						{
							state = 0;
						}
					}break;
					CASE_BLANK:
					break;
					default:
					{
						// 这个时候需要吃到  >
						tag += c;
					}
				}
				break;
			}

			case 10:
			{
				// 遇到了 <! 这种,
				switch(c)
				{
					case '-':
						tag += c;
						state = 11;
						break;
					default:
						tag += c;
						state = pre_state;
				}
			}break;
			case 11:
			{
				// 遇到了 <!- 这种,
				switch(c)
				{
					case '-':
						state = 12;
						comment_stack.push_back(pre_state);
						tag.clear();
						break;
					default:
						tag += c;
						state = pre_state;
				}

			}break;
			case 12:
			{
				// 遇到了 <!-- 这种,
				// 要做的就是一直忽略直到  -->
				switch(c)
				{
					case '<':
					{
						c = getc();
						if ( c == '!')
						{
							pre_state = state;
							state = 10;
						}else{
							content += '<';
							content += c;
						}
					}break;
					case '-':
					{
						pre_state = state;
						state = 13;
					}
					default:
						content += c;
				}
			}break;
			case 13: // 遇到 -->
			{
				switch(c)
				{
					case '-':
					{
						state = 14;
					}break;
					default:
						content += c;
						state = pre_state;
				}
			}break;
			case 14: // 遇到 -->
			{
				switch(c)
				{
					case '>':
					{
						tag.clear();
						if (pre_state == 12)
							content.pop_back();
						comment_stack.pop_back();
						state = comment_stack.empty()? 0 : 12;
						auto comment_node = std::make_shared<basic_dom<CharType>>(current_ptr);
						comment_node->tag_name = comment_tag_string<CharType>();
						comment_node->content_text = std::move(content);
						current_ptr->children.push_back(comment_node);
					}break;
					default:
						content += c;
						state = pre_state;
				}
			}break;

			case 20: // 处理 javascript
			{
				// 直到看到 </script>
				switch(c)
				{
					case '<':
					{
						pre_state = state;
						state = 21;
					}
					default:
						content += c;
				}
			}break;
			case 21:
			{
				switch(c)
				{
					case '/':
					{
						state = 22;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 22:
			{
				switch(c)
				{
					case 's':
					case 'S':
					{
						state = 23;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 23:
			{
				switch(c)
				{
					case 'c':
					case 'C':
					{
						state = 24;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 24:
			{
				switch(c)
				{
					case 'r':
					case 'R':
					{
						state = 25;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 25:
			{
				switch(c)
				{
					case 'i':
					case 'I':
					{
						state = 26;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 26:
			{
				switch(c)
				{
					case 'p':
					case 'P':
					{
						state = 27;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 27:
			{
				switch(c)
				{
					case 't':
					case 'T':
					{
						state = 28;
						content += c;
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
			case 28:
			{
				switch(c)
				{
					case '>':
					{
						state = 0;
						{
							for (int i =0 ; i < 8 ;i++)
								content.pop_back();
							current_ptr->content_text = std::move(content);
							(*this->node_parser)(tag_close, current_ptr->shared_from_this());
							current_ptr = current_ptr->m_parent;
						}
					}break;
					default:
						state = pre_state;
						content += c;
				}
			}break;
		}
	} while(c);
}

#undef CASE_BLANK

template void html::basic_dom<char>::html_parser(const std::basic_string<char>* html_page_source);
template void html::basic_dom<wchar_t>::html_parser(const std::basic_string<wchar_t>* html_page_source);
