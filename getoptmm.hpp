/*
getoptmm

Copyright (c) 2015 iorate

This software is released under the MIT License.
http://opensource.org/licenses/mit-license.php
*/

#ifndef GETOPTMM_HPP
#define GETOPTMM_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace getoptmm {

constexpr struct no_arg_t {} no_arg = {};
constexpr struct optional_arg_t {} optional_arg = {};
constexpr struct required_arg_t {} required_arg = {};

enum class arg_type
{
    none,
    optional,
    required
};

enum class match_type
{
    none,
    exact,
    partial
};

template <class String>
class basic_option
{
public:
    using char_type = typename String::value_type;
    using string_type = String;

    template <class NoargHandler>
    basic_option(
        std::initializer_list<char_type> short_names,
        std::initializer_list<string_type> long_names,
        no_arg_t,
        NoargHandler &&handler,
        string_type const &description)
      : m_short_names(short_names),
        m_long_names(long_names),
        m_arg_type(arg_type::none),
        m_handler(
            [h = std::forward<NoargHandler>(handler)](string_type const *)
            {
                h();
            }),
        m_arg_name(),
        m_description(description)
    {}

    template <class OptionalargHandler>
    basic_option(
        std::initializer_list<char_type> short_names,
        std::initializer_list<string_type> long_names,
        optional_arg_t,
        OptionalargHandler &&handler,
        string_type const &arg_name,
        string_type const &description)
      : m_short_names(short_names),
        m_long_names(long_names),
        m_arg_type(arg_type::optional),
        m_handler(
            [h = std::forward<OptionalargHandler>(handler)](string_type const *a)
            {
                if (a) { h(*a); }
                else { h(); }
            }),
        m_arg_name(arg_name),
        m_description(description)
    {}

    template <class RequiredargHandler>
    basic_option(
        std::initializer_list<char_type> short_names,
        std::initializer_list<string_type> long_names,
        required_arg_t,
        RequiredargHandler &&handler,
        string_type const &arg_name,
        string_type const &description)
      : m_short_names(short_names),
        m_long_names(long_names),
        m_arg_type(arg_type::required),
        m_handler(
            [h = std::forward<RequiredargHandler>(handler)](string_type const *a)
            {
                assert(a);
                h(*a);
            }),
        m_arg_name(arg_name),
        m_description(description)
    {}

    match_type match(char_type c) const
    {
        for (auto name : m_short_names) {
            if (name == c) { return match_type::exact; }
        }
        return match_type::none;
    }

    match_type match(string_type const &s) const
    {
        auto ret = match_type::none;
        for (auto const &name : m_long_names) {
            if (name == s) { return match_type::exact; }
            if (s.size() < name.size() &&
                std::mismatch(s.begin(), s.end(), name.begin()).first == s.end()) {
                ret = match_type::partial;
            }
        }
        return ret;
    }

    arg_type get_arg_type() const noexcept
    {
        return m_arg_type;
    }

    void execute()
    {
        assert(m_arg_type != arg_type::required);
        m_handler(nullptr);
    }

    void execute(string_type const &arg)
    {
        assert(m_arg_type != arg_type::none);
        m_handler(&arg);
    }

    std::array<string_type, 3> get_help() const
    {
        using ostringstream = std::basic_ostringstream<
            char_type,
            typename string_type::traits_type>;
        std::array<string_type, 3> ret;
        {
            ostringstream oss;
            auto first = true;
            for (auto c : m_short_names) {
                if (first) { first = false; }
                else { oss << ','; }
                oss << '-' << c;
                if (m_arg_type == arg_type::optional) {
                    oss << '[' << m_arg_name << ']';
                }
                else if (m_arg_type == arg_type::required) {
                    oss << ' ' << m_arg_name;
                }
            }
            ret[0] = oss.str();
        }
        {
            ostringstream oss;
            auto first = true;
            for (auto const &s : m_long_names) {
                if (first) { first = false; }
                else { oss << ','; }
                oss << "--" << s;
                if (m_arg_type == arg_type::optional) {
                    oss << "[=" << m_arg_name << ']';
                } else if (m_arg_type == arg_type::required) {
                    oss << '=' << m_arg_name;
                }
            }
            ret[1] = oss.str();
        }
        ret[2] = m_description;
        return ret;
    }

private:
    std::vector<char_type> m_short_names;
    std::vector<string_type> m_long_names;
    arg_type m_arg_type;
    std::function<void (string_type const *)> m_handler;
    string_type m_arg_name;
    string_type m_description;
};

using option = basic_option<std::string>;
using woption = basic_option<std::wstring>;

template <class String>
class basic_parse_error
  : public std::runtime_error
{
public:
    using string_type = String;

    explicit basic_parse_error(string_type const &message)
      : std::runtime_error("parse error"),
        m_message(message)
    {}

    string_type get_message() const
    {
        return m_message;
    }

private:
    string_type m_message;
};

enum class parse_flag
{
    none,
    posixly_correct
};

template <class String>
class basic_parser
{
public:
    using char_type = typename String::value_type;
    using string_type = String;
    using option_type = basic_option<String>;
    using error = basic_parse_error<String>;

    template <class Iterator, class NonOptionHandler>
    basic_parser(
        Iterator first, Iterator last,
        NonOptionHandler &&non_option_handler,
        parse_flag flag = parse_flag::none)
      : basic_parser(
          std::move(first), std::move(last),
          std::forward<NonOptionHandler>(non_option_handler),
          [](string_type const &a)
          {
              std::basic_stringstream<char_type, typename string_type::traits_type> err;
              err << "unrecognized option: " << a;
              throw error(err.str());
          },
          flag)
    {}

    template <
        class Iterator, class NonOptionHandler, class UnrecOptionHandler,
        std::result_of_t<UnrecOptionHandler(string_type const &)> * = nullptr>
    basic_parser(
        Iterator first, Iterator last,
        NonOptionHandler &&non_option_handler,
        UnrecOptionHandler &&unrec_option_handler,
        parse_flag flag = parse_flag::none)
      : m_options(std::move(first), std::move(last)),
        m_non_option_handler(std::forward<NonOptionHandler>(non_option_handler)),
        m_unrec_option_handler(std::forward<UnrecOptionHandler>(unrec_option_handler)),
        m_flag(flag)
    {}

    void run(int argc, char_type **argv)
    {
        run(argv + 1, argv + argc);
    }

    template <class Iterator>
    void run(Iterator first, Iterator last)
    {
        auto _ = [](char const *from)
        {
            std::basic_stringstream<char_type, typename string_type::traits_type> ss;
            ss << from;
            return ss.str();
        };
        for (auto it = first; it != last; ++it) {
            string_type arg = *it;
            if (arg == _("--")) {
                // the rest are non-option
                for (++it; it != last; ++it) {
                    m_non_option_handler(*it);
                }
                break;
            }
            using regex = std::basic_regex<char_type>;
            std::match_results<typename string_type::const_iterator> match;
            if (std::regex_match(arg, match, regex(_("--([^=]*)(?:=(.*))?")))) {
                // long option
                auto const name = match.str(1);
                auto find_match = [&](auto optit, match_type n) {
                    return std::find_if(
                        optit, m_options.end(),
                        [&](auto const &opt) { return opt.match(name) == n; });
                };
                auto optit = find_match(m_options.begin(), match_type::exact);
                if (optit == m_options.end()) {
                    optit = find_match(m_options.begin(), match_type::partial);
                    if (optit == m_options.end()) {
                        m_unrec_option_handler(arg);
                        continue;
                    }
                    if (find_match(std::next(optit), match_type::partial) != m_options.end()) {
                        throw error(_("ambiguous option: --") + name);
                    }
                }
                if (find_match(std::next(optit), match_type::exact) != m_options.end()) {
                    throw error(_("ambiguous option: --") + name);
                }
                
                auto const n = optit->get_arg_type();
                if (n == arg_type::none) {
                    if (match[2].matched) {
                        throw error(_("argument not allowed: --") + name);
                    }
                    optit->execute();
                } else if (n == arg_type::optional) {
                    if (match[2].matched) {
                        optit->execute(match[2]);
                    } else {
                        optit->execute();
                    }
                } else {
                    if (match[2].matched) {
                        optit->execute(match[2]);
                    } else {
                        if (++it == last) {
                            throw error(_("argument required: --") + name);
                        }
                        optit->execute(*it);
                    }
                }
            } else if (std::regex_match(arg, match, regex(_("-(.+)")))) {
                // short option
                for (auto cit = match[1].first, clast = match[1].second; cit != clast; ++cit) {
                    auto name = *cit;
                    auto find_match = [&](auto optit) {
                        return std::find_if(
                            optit, m_options.end(),
                            [&](auto const &opt) { return opt.match(name) == match_type::exact; });
                    };
                    auto optit = find_match(m_options.begin());
                    if (optit == m_options.end()) { 
                        m_unrec_option_handler(_("-") + string_type(cit, clast));
                        continue;
                    }
                    if (find_match(std::next(optit)) != m_options.end()) {
                        throw error(_("ambiguous option: -") + name);
                    }
                    auto const n = optit->get_arg_type();
                    if (n == arg_type::none) {
                        optit->execute();
                    } else if (n == arg_type::optional) {
                        if (++cit == clast) { optit->execute(); }
                        else { optit->execute({cit, clast}); }
                        break;
                    } else {
                        if (++cit == clast) {
                            if (++it == last) {
                                throw error(_("argument required: -") + name);
                            }
                            optit->execute(*it);
                        } else {
                            optit->execute({cit, clast});
                        }
                        break;
                    }
                };
            } else if (m_flag == parse_flag::posixly_correct) {
                // non-option (the rest are treated as so)
                for (; it != last; ++it) {
                    m_non_option_handler(*it);
                }
                break;
            } else {
                // non-option
                m_non_option_handler(*it);
            }
        }
    }

    string_type get_help(string_type const &header) const
    {
        std::vector<std::array<string_type, 3>> helps;
        helps.reserve(m_options.size());
        auto col0 = 0;
        auto col1 = 0;
        for (auto const &opt: m_options) {
            auto help = opt.get_help();
            col0 = std::max<int>(col0, help[0].length());
            col1 = std::max<int>(col1, help[1].length());
            helps.push_back(std::move(help));
        }
        ++col0;
        ++col1;

        using stringstream = std::basic_stringstream<
            char_type,
            typename string_type::traits_type>;
        stringstream ss;
        ss << header << '\n';
        ss << std::left;
        auto fstopt = true;
        for (auto const &help: helps) {
            if (fstopt) { fstopt = false; }
            else { ss << '\n'; }

            ss << std::setw(col0) << help[0];
            ss << std::setw(col1) << help[1];
            stringstream desc(help[2]);

            string_type ln;
            auto fstln = true;
            while (std::getline(desc, ln)) {
                if (fstln) { fstln = false; }
                else { ss << '\n' << std::setw(col0 + col1) << ""; }
                ss << ln;
            }
        }
        return ss.str();
    }

private:
    std::vector<option_type> m_options;
    std::function<void (string_type const &)> m_non_option_handler;
    std::function<void (string_type const &)> m_unrec_option_handler;
    parse_flag m_flag;
};

using parser = basic_parser<std::string>;
using wparser = basic_parser<std::wstring>;

namespace detail {

    template <class String, class T>
    struct from_string_t
    {
        T operator()(String const &s) const
        {
            using stringstream = std::basic_stringstream<
                typename String::value_type,
                typename String::traits_type>;
            using error = basic_parse_error<String>;

            stringstream ss(s);
            T t;
            if (!(ss >> t && (ss.peek(), ss.eof()))) {
                stringstream err;
                err << "invalid value: " << s;
                throw error(err.str());
            }
            return t;
        }
    };

    template <class String>
    struct from_string_t<String, String>
    {
        String operator()(String const &s) const { return s; }
    };

    template <class T, class String>
    inline T from_string(String const &s)
    {
        return from_string_t<String, T>()(s);
    }

    struct ignore_t
    {
        void operator()() const {}

        template <class Arg>
        void operator()(Arg const &) const {}
    };

    template <class T, class U>
    struct assign_const_t
    {
        T &t;
        U u;

        void operator()() const { t = u; }

        template <class Arg>
        void operator()(Arg const &) const { t = u; }
    };

    template <class T, class U>
    struct push_back_const_t
    {
        T &t;
        U u;

        void operator()() const { t.push_back(u); }

        template <class Arg>
        void operator()(Arg const &) const { t.push_back(u); }
    };

    template <class T, class U>
    struct assign_or_t
    {
        T &t;
        U u;

        void operator()() const { t = u; }

        template <class Arg>
        void operator()(Arg const &arg) const { t = from_string<T>(arg); }
    };

    template <class T, class U>
    struct push_back_or_t
    {
        T &t;
        U u;

        void operator()() const { t.push_back(u); }

        template <class Arg>
        void operator()(Arg const &arg) const
        {
            t.push_back(from_string<typename T::value_type>(arg));
        }
    };

} // namespace detail

constexpr detail::ignore_t ignore = {};

template <class T, class U>
inline auto assign_const(T &t, U &&u)
{
    return detail::assign_const_t<T, std::decay_t<U>>{t, std::forward<U>(u)};
}

template <class T>
inline auto assign_true(T &t) { return detail::assign_const_t<T, bool>{t, true}; }

template <class T>
inline auto assign_false(T &t) { return detail::assign_const_t<T, bool>{t, false}; }

template <class T, class U>
inline auto push_back_const(T &t, U &&u)
{
    return detail::push_back_const_t<T, std::decay_t<U>>{t, std::forward<U>(u)};
}

template <class T, class U>
inline auto assign_or(T &t, U &&u)
{
    return detail::assign_or_t<T, std::decay_t<U>>{t, std::forward<U>(u)};
}

template <class T, class U>
inline auto push_back_or(T &t, U &&u)
{
    return detail::push_back_or_t<T, std::decay_t<U>>{t, std::forward<U>(u)};
}

template <class T>
inline auto assign(T &t)
{
    return [&t](auto const &arg)
    {
        t = detail::from_string<T>(arg);
    };
}

template <class T>
inline auto push_back(T &t)
{
    return [&t](auto const &arg)
    {
        t.push_back(detail::from_string<typename T::value_type>(arg));
    };
}

} // namespace getoptmm

#endif
