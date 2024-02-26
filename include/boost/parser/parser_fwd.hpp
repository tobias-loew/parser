// Copyright (C) 2020 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PARSER_PARSER_FWD_HPP
#define BOOST_PARSER_PARSER_FWD_HPP

#include <boost/parser/config.hpp>
#include <boost/parser/error_handling_fwd.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <variant>


namespace boost::parser::detail { namespace text {
    struct null_sentinel_t;
}}

namespace boost { namespace parser {

    /** A sentinel type that compares equal to a pointer to a character value
        type, iff the pointer is null. */
    using null_sentinel_t = boost::parser::detail::text::null_sentinel_t;

    /** A variable template that indicates that type `T` is an optional-like
        type. */
    template<typename T>
    constexpr bool enable_optional = false;

    /** A variable template that indicates that type `T` is an variant-like
        type. */
    template<typename T>
    constexpr bool enable_variant = false;

#ifndef BOOST_PARSER_DOXYGEN
    template<typename T>
    constexpr bool enable_optional<std::optional<T>> = true;
    template<typename... Ts>
    constexpr bool enable_variant<std::variant<Ts...>> = true;
#endif

    /** A type trait that evaluates to the attribute type for parser `Parser`
        used to parse range `R`, as if by calling `parse(r, parser)`, using
        some `R r` and `Parser parser`.  Note that this implies that pointers
        to null-terminated strings are supported types for `R`.  The result is
        not wrapped in a `std::optional` like the result of a call to
        `parse()` would be.  If `Parser` produces no attribute, the result is
        the no-attribute sentinel type `none`. */
    template<typename R, typename Parser>
    struct attribute;

    /** An alias for `typename attribute<R, Parser>::type`. */
    template<typename R, typename Parser>
    using attribute_t = typename attribute<R, Parser>::type;

    namespace detail {
        template<typename T>
        constexpr bool is_optional_v = enable_optional<T>;

        struct nope;

        enum class flags : unsigned int {
            gen_attrs = 1 << 0,
            use_skip = 1 << 1,
            trace = 1 << 2,
            in_apply_parser = 1 << 3
        };

        struct any_copyable
        {
            template<
                typename T,
                typename Enable = std::enable_if_t<!std::is_reference_v<T>>>
            any_copyable(T && v) :
                impl_(new holder<std::decay_t<T>>(std::move(v)))
            {}
            template<typename T>
            any_copyable(T const & v) : impl_(new holder<T>(v))
            {}

            any_copyable() = default;
            any_copyable(any_copyable const & other)
            {
                if (other.impl_)
                    impl_ = other.impl_->clone();
            }
            any_copyable & operator=(any_copyable const & other)
            {
                any_copyable temp(other);
                swap(temp);
                return *this;
            }
            any_copyable(any_copyable &&) = default;
            any_copyable & operator=(any_copyable &&) = default;

            bool empty() const { return impl_.get() == nullptr; }

            template<typename T>
            T & cast() const
            {
                BOOST_PARSER_DEBUG_ASSERT(impl_);
                BOOST_PARSER_DEBUG_ASSERT(dynamic_cast<holder<T> *>(impl_.get()));
                return static_cast<holder<T> *>(impl_.get())->value_;
            }

            void swap(any_copyable & other) { std::swap(impl_, other.impl_); }

        private:
            struct holder_base
            {
                virtual ~holder_base() {}
                virtual std::unique_ptr<holder_base> clone() const = 0;
            };
            template<typename T>
            struct holder : holder_base
            {
                holder(T && v) : value_(std::move(v)) {}
                holder(T const & v) : value_(v) {}
                virtual ~holder() {}
                virtual std::unique_ptr<holder_base> clone() const
                {
                    return std::unique_ptr<holder_base>(new holder<T>{value_});
                }
                T value_;
            };

            std::unique_ptr<holder_base> impl_;
        };

        using symbol_table_tries_t =
            std::map<void *, any_copyable, std::less<void *>>;

        template<typename Iter, typename Sentinel, typename ErrorHandler>
        inline auto make_context(
            Iter first,
            Sentinel last,
            bool & success,
            int & indent,
            ErrorHandler const & error_handler,
            nope &,
            symbol_table_tries_t & symbol_table_tries) noexcept;

        struct skip_skipper;

        struct char_subrange
        {
            char32_t lo_;
            char32_t hi_;
        };

        template<typename Tag>
        struct char_subranges
        {};

        struct hex_digit_subranges
        {};
        struct control_subranges
        {};

        template<typename Tag>
        struct char_set
        {};

        struct punct_chars
        {};
        struct lower_case_chars
        {};
        struct upper_case_chars
        {};
    }

    /** Repeats the application of another parser `p` of type `Parser`,
        optionally applying another parser `d` of type `DelimiterParser` in
        between each pair of applications of `p`.  The parse succeeds if `p`
        succeeds at least the minumum number of times, and `d` succeeds each
        time it is applied.  The attribute produced is a sequence of the type
        of attribute produced by `Parser`. */
    template<
        typename Parser,
        typename DelimiterParser = detail::nope,
        typename MinType = int64_t,
        typename MaxType = int64_t>
    struct repeat_parser;

    /** Repeats the application of another parser `p` of type `Parser`, `[0,
        Inf)` times.  The parse always succeeds.  The attribute produced is a
        sequence of the type of attribute produced by `Parser`. */
    template<typename Parser>
    struct zero_plus_parser;

    /** Repeats the application of another parser `p` of type `Parser`, `[1,
        Inf)` times.  The parse succeeds iff `p` succeeds at least once.  The
        attribute produced is a sequence of the type of attribute produced by
        `Parser`. */
    template<typename Parser>
    struct one_plus_parser;

    /** Repeats the application of another parser `p` of type `Parser`, `[1,
        Inf)` times, applying a parser `d` of type `DelimiterParser` in
        between each pair of applications of `p`.  The parse succeeds iff `p`
        succeeds at least once, and `d` succeeds each time it is applied.  The
        attribute produced is a sequence of the type of attribute produced by
        `Parser`. */
    template<typename Parser, typename DelimiterParser>
    struct delimited_seq_parser;

    /** Repeats the application of another parser of type `Parser`, `[0, 1]`
        times.  The parse always succeeds.  The attribute produced is a
        `std::optional<T>`, where `T` is the type of attribute produced by
        `Parser`. */
    template<typename Parser>
    struct opt_parser;

    /** Applies each parser in `ParserTuple`, in order, stopping after the
        application of the first one that succeeds.  The parse succeeds iff
        one of the sub-parsers succeeds.  The attribute produced is a
        `std::variant` over the types of attribute produced by the parsers in
        `ParserTuple`. */
    template<typename ParserTuple>
    struct or_parser;

    /** Applies each parser in `ParserTuple`, in order.  The parse succeeds
        iff all of the sub-parsers succeed.  The attribute produced is a
        `std::tuple` over the types of attribute produced by the parsers in
        `ParserTuple`.  The BacktrackingTuple template parameter is a
        `parser::tuple` of `std::bool_constant` values.  The `i`th such value
        indicates whether backtracking is allowed if the `i`th parser
        fails. */
    template<
        typename ParserTuple,
        typename BacktrackingTuple,
        typename CombiningGroups>
    struct seq_parser;

    /** Applies the given parser `p` of type `Parser` and an invocable `a` of
        type `Action`.  `Action` shall model `semantic_action`, and `a` will
        only be invoked if `p` succeeds.  The parse succeeds iff `p` succeeds.
        Produces no attribute. */
    template<typename Parser, typename Action>
    struct action_parser;

    /** Applies the given parser `p` of type `Parser`.  This parser produces
        no attribute, and suppresses the production of any attributes that
        would otherwise be produced by `p`.  The parse succeeds iff `p`
        succeeds. */
    template<typename Parser>
    struct omit_parser;

    /** Applies the given parser `p` of type `Parser`; regardless of the
        attribute produced by `Parser`, this parser's attribute is equivalent
        to `_where(ctx)` within a semantic action on `p`.  The parse succeeds
        iff `p` succeeds. */
    template<typename Parser>
    struct raw_parser;

#if defined(BOOST_PARSER_DOXYGEN) || defined(__cpp_lib_concepts)
    /** Applies the given parser `p` of type `Parser`.  Regardless of the
        attribute produced by `Parser`, this parser's attribute is equivalent
        to `std::basic_string_view<char_type>` within a semantic action on
        `p`, where `char_type` is the type of character in the sequence being
        parsed.  If the parsed range is transcoded, `char_type` will be the
        type being transcoded from.  If the underlying range of `char_type` is
        non-contiguous, code using `string_view_parser` is ill-formed.  The
        parse succeeds iff `p` succeeds.  This parser is only available in
        C++20 and later. */
    template<typename Parser>
    struct string_view_parser;
#endif

    /** Applies the given parser `p` of type `Parser`, disabling the current
        skipper in use, if any.  The parse succeeds iff `p` succeeds.  The
        attribute produced is the type of attribute produced by `Parser`. */
    template<typename Parser>
    struct lexeme_parser;

    /** Applies the given parser `p` of type `Parser`, enabling
        case-insensitive matching, based on Unicode case folding.  The parse
        succeeds iff `p` succeeds.  The attribute produced is the type of
        attribute produced by `Parser`. */
    template<typename Parser>
    struct no_case_parser;

    /** Applies the given parser `p` of type `Parser`, using a parser of type
        `SkipParser` as the skipper.  The parse succeeds iff `p` succeeds.
        The attribute produced is the type of attribute produced by
        `Parser`. */
    template<typename Parser, typename SkipParser = detail::nope>
    struct skip_parser;

    /** Applies the given parser `p` of type `Parser`, producing no attributes
        and consuming no input.  The parse succeeds iff `p`'s success is
        unequal to `FailOnMatch`. */
    template<typename Parser, bool FailOnMatch>
    struct expect_parser;

    /** Matches one of a set S of possible inputs, each of which is associated
        with an attribute value of type `T`, forming a symbol table.  New
        elements and their associated attributes may be added to or removed
        from S dynamically, during parsing; any such changes are reverted at
        the end of parsing.  The parse succeeds iff an element of S is
        matched.  \see `symbols` */
    template<typename T>
    struct symbol_parser;

    /** Applies another parser `p`, associated with this parser via `TagType`.
        The attribute produced is `Attribute`.  Both a default-constructed
        object of type `LocalState`, and a default-constructed object of type
        `ParamsTuple`, are added to the parse context before the associated
        parser is applied.  The parse succeeds iff `p` succeeds.  If
        `CanUseCallbacks` is `true`, and if this parser is used within a call
        to `callback_parse()`, the attribute is produced via callback;
        otherwise, the attribute is produced as normal (as a return value, or
        as an out-param).  The rule may be constructed with user-friendly
        diagnostic text that will appear if the top-level parse is executed
        with `trace_mode == boost::parser::trace::on`. */
    template<
        bool CanUseCallbacks,
        typename TagType,
        typename Attribute,
        typename LocalState,
        typename ParamsTuple>
    struct rule_parser;

    /** Matches anything, and consumes no input.  If `Predicate` is anything
        other than `detail::nope` (which it is by default), and `pred_(ctx)`
        evaluates to false, where `ctx` is the parser context, the parse
        fails. */
    template<typename Predicate>
    struct eps_parser;

    /** Matches only the end of input.  Produces no attribute. */
    struct eoi_parser;

    /** Matches anything, consumes no input, and produces an attribute of type
        `RESOLVE(Attribute)`. */
    template<typename Attribute>
    struct attr_parser;

    /** A tag type that can be passed as the first parameter to `char_()` when
        the second parameter is a sorted, random access sequence that can be
        matched using a binary search.*/
    struct sorted_t
    {};

    inline constexpr sorted_t sorted;

    /** Matches a single code point.  If `AttributeType` is not `void`,
        `AttributeType` is the attribute type produced; otherwise, the
        attribute type is the decayed type of the matched code point.  The
        parse fails only if the parser is constructed with a specific set of
        expected code point values that does not include the matched code
        point. */
    template<typename Expected, typename AttributeType = void>
    struct char_parser;

    /** Matches a single code point that is equal to one of the code points
        associated with tag type `Tag`.  This is used to create sets of
        characters for matching Unicode character classes like punctuation or
        lower case.  Attribute type is the attribute type of the character
        being matched. */
    template<typename Tag>
    struct char_set_parser;

    /** Matches a single code point that falls into one of the subranges of
        code points associated with tag type `Tag`.  This is used to create
        sets of characters for matching Unicode character classes like hex
        digits or control characters.  Attribute type is the attribute type of
        the character being matched. */
    template<typename Tag>
    struct char_subrange_parser;

    /** Matches a single decimal digit code point, using the Unicode character
        class Hex_Digit.  Attribute type is the attribute type of the
        character being matched. */
    struct digit_parser;

    /** Maches a particular string, delimited by an iterator sentinel pair;
        produces no attribute. */
    template<typename StrIter, typename StrSentinel>
    struct string_parser;

    /** Maches an end-of-line (`NewlinesOnly == true`), whitespace
        (`NewlinesOnly == false`), or (`NoNewlines == true`) blank (whitespace
        but not newline) code point, based on the Unicode definitions of each
        (also matches the two code points `"\r\n"`).  Produces no
        attribute. */
    template<bool NewlinesOnly, bool NoNewlines>
    struct ws_parser;

    /** Maches the strings "true" and "false", producing an attribute of
        `true` or `false`, respectively, and fails on any other input. */
    struct bool_parser;

    /** Matches an unsigned number of radix `Radix`, of at least `MinDigits`
        and at most `MaxDigits`, producing an attribute of type `T`.  Fails on
        any other input.  The parse will also fail if `Expected` is anything
        but `detail::nope` (which it is by default), and the produced
        attribute is not equal to `expected_`.  `Radix` must be in `[2,
        36]`. */
    template<
        typename T,
        int Radix = 10,
        int MinDigits = 1,
        int MaxDigits = -1,
        typename Expected = detail::nope>
    struct uint_parser;

    /** Matches a signed number of radix `Radix`, of at least `MinDigits` and
        at most `MaxDigits`, producing an attribute of type `T`.  Fails on any
        other input.  The parse will also fail if `Expected` is anything but
        `detail::nope` (which it is by default), and the produced
        attribute is not equal to `expected_`.  `Radix` must be one of `2`,
        `8`, `10`, or `16`. */
    template<
        typename T,
        int Radix = 10,
        int MinDigits = 1,
        int MaxDigits = -1,
        typename Expected = detail::nope>
    struct int_parser;

    /** Matches a floating point number, producing an attribute of type
        `T`. */
    template<typename T>
    struct float_parser;

    /** Applies at most one of the parsers in `OrParser`.  If `switch_value_`
        matches one or more of the values in the parsers in `OrParser`, the
        first such parser is applied, and the success or failure and attribute
        of the parse are those of the applied parser.  Otherwise, the parse
        fails. */
    template<typename SwitchValue, typename OrParser = detail::nope>
    struct switch_parser;

    /** A wrapper for parsers that provides the operations that must be
        supported by all parsers (e.g. `operator>>()`).  `GlobalState` is an
        optional state object that can be accessed within semantic actions via
        a call to `_globals()`.  This global state object is ignored for all
        but the topmost parser; the topmost global state object is available
        in the semantic actions of all nested parsers.  `ErrorHandler` is the
        type of the error handler to be used on parse failure.  This handler
        is ignored on all but the topmost parser; the topmost parser's error
        handler is used for all errors encountered during parsing. */
    template<
        typename Parser,
        typename GlobalState = detail::nope,
        typename ErrorHandler = default_error_handler>
    struct parser_interface;

    using no_attribute = detail::nope;
    using no_local_state = detail::nope;
    using no_params = detail::nope;

    /** A type used to declare named parsing rules.  The `TagType` template
        parameter is used to associate a particular `rule` with the
        `rule_parser` used during parsing. */
    template<
        typename TagType,
        typename Attribute = no_attribute,
        typename LocalState = no_local_state,
        typename ParamsTuple = no_params>
    struct rule;

    /** A type used to declare named parsing rules that support reporting of
        attributes via callback.  The `TagType` template parameter is used to
        associate a particular `rule` with the `rule_parser` used during
        parsing. */
    template<
        typename TagType,
        typename Attribute = no_attribute,
        typename LocalState = no_local_state,
        typename ParamsTuple = no_params>
    struct callback_rule;

    /** Returns a reference to the attribute(s) (i.e. return value) of the
        bottommost parser; multiple attributes will be stored within a
        `parser::tuple`.  You may write to this value in a semantic action to
        control what attribute value(s) the associated parser produces.
        Returns `none` if the bottommost parser does produce an attribute. */
    template<typename Context>
    decltype(auto) _val(Context const & context);

    /** Returns a reference to the attribute or attributes already produced by
        the bottommost parser; multiple attributes will be stored within a
        `parser::tuple`.  Returns `none` if the bottommost parser does produce
        an attribute. */
    template<typename Context>
    decltype(auto) _attr(Context const & context);

    /** Returns a `subrange` that describes the matched range of the
        bottommost parser. */
    template<typename Context>
    decltype(auto) _where(Context const & context);

    /** Returns an iterator to the beginning of the entire sequence being
        parsed.  The effect of calling this within a semantic action
        associated with a skip-parser is undefined */
    template<typename Context>
    decltype(auto) _begin(Context const & context);

    /** Returns an iterator to the end of the entire sequence being parsed. */
    template<typename Context>
    decltype(auto) _end(Context const & context);

    /** Returns a reference to a `bool` that represents the success or failure
        of the bottommost parser.  You can assign `false` to this within a
        semantic action to force a parser to fail its parse. */
    template<typename Context>
    decltype(auto) _pass(Context const & context);

    /** Returns a reference to one or more local values that the bottommost
        rule is declared to have; multiple values will be stored within a
        `parser::tuple`.  Returns `none` if there is no bottommost rule, or if
        that rule has no locals. */
    template<typename Context>
    decltype(auto) _locals(Context const & context);

    /** Returns a reference to one or more parameters passed to the bottommost
        rule `r`, by using `r` as `r.with(param0, param1, ... paramN)`;
        multiple values will be stored within a `parser::tuple`.  Returns
        `none` if there is no bottommost rule, or if that rule was not given
        any parameters. */
    template<typename Context>
    decltype(auto) _params(Context const & context);

    /** Returns a reference to the globals object associated with the
        top-level parser.  Returns `none` if there is no associated globals
        object. */
    template<typename Context>
    decltype(auto) _globals(Context const & context);

    /** Returns a reference to the error handler object associated with the
        top-level parser.  Returns `none` if there is no associated error
        handler. */
    template<typename Context>
    decltype(auto) _error_handler(Context const & context);

    /** Report that the error described in `message` occurred at `location`,
        using the context's error handler. */
    template<typename Iter, typename Context>
    void _report_error(
        Context const & context, std::string_view message, Iter location);

    /** Report that the error described in `message` occurred at
        `_where(context).begin()`, using the context's error handler. */
    template<typename Context>
    void _report_error(Context const & context, std::string_view message);

    /** Report that the warning described in `message` occurred at `location`,
        using the context's error handler. */
    template<typename Iter, typename Context>
    void _report_warning(
        Context const & context, std::string_view message, Iter location);

    /** Report that the warning described in `message` occurred at
        `_where(context).begin()`, using the context's error handler. */
    template<typename Context>
    void _report_warning(Context const & context, std::string_view message);

}}

#endif
