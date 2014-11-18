#ifndef TPP_TINY_PACKRAT_PARSER_HPP
#define TPP_TINY_PACKRAT_PARSER_HPP
//20141112
#include <cassert>
#include <tuple>
#include <type_traits>
#include <vector>
#include <functional>
#include<experimental/optional>

namespace tpp {
	//
	// tag
	//
	namespace tag {
		struct terminal {}; struct unary_plus {}; struct negate {};
		struct dereference {}; struct address_of {}; struct logical_not {};
		struct shift_right {}; struct modulus {}; struct minus {};
		struct bitwise_or {}; struct mem_ptr {};
	}// namespace tag

	template<typename Expr> struct tag_of { using type = typename Expr::tag_type; };
	template<typename Expr> using tag_t = typename tpp::tag_of<Expr>::type;
	template<typename Expr, typename Tag>
	struct is_tag_same {
		static constexpr bool value = std::is_same<tpp::tag_t<Expr>, Tag>::value;
	};
	
	//
	// expr
	//
	template<typename Tag, typename... Args>
	struct expr {
		using tag_type = Tag;
		using args_type = std::tuple<Args...>;

		explicit expr(Args const&... args) : args_(args...) {}

		template<std::size_t I> auto get() const -> decltype(auto) {
			return std::get<I>(args_); } 

	private:
		args_type args_;
	};

	template<typename Tag, typename... Args>
	auto make_expr_impl(Args const&... args) -> decltype(auto) {
		return tpp::expr<Tag, Args...>(args...);
	}

	template<typename T> struct is_expr { static constexpr bool value = false; };
	template<typename Tag, typename... Args> struct is_expr<tpp::expr<Tag, Args...>> {
		static constexpr bool value = true;
	};
	
	template<typename T>
	auto make_terminal(T const& t) -> decltype(auto) {
		return tpp::make_expr_impl<tpp::tag::terminal>(t);
	}

	template<typename TerminalSourceOrExpr, typename=void>
	struct make_terminal_or_pass_expr;

	template<typename Arg>
	struct make_terminal_or_pass_expr<
		Arg, 
		std::enable_if_t<tpp::is_expr<Arg>::value>
	> {
		static auto call(Arg const& arg) -> decltype(auto) { return arg; }
	};
	template<typename Arg>
	struct make_terminal_or_pass_expr<
		Arg, 
		std::enable_if_t<!tpp::is_expr<Arg>::value>
	> {
		static auto call(Arg const& arg) -> decltype(auto) {
			return tpp::make_terminal(arg);
		}
	};
	template<typename Tag, typename... Args>
	auto make_expr(Args const&... args) -> decltype(auto) {
		return tpp::make_expr_impl<Tag>(tpp::make_terminal_or_pass_expr<Args>::call(args)...);
	}

	template<std::size_t I, typename Expr>
	using expr_element_t = std::tuple_element_t<I, typename Expr::args_type>;

	//
	// attribute
	//
	struct unused {};

	template<typename T>
	using optional = std::experimental::optional<T>;

	template<typename A, typename B>
	class variant {
	public:
		variant(A const& a) : a_(a), b_() {}
		variant(B const& b) : a_(), b_(b) {}

		operator A() const { return *a_; }
		operator B() const { return *b_; }
	private:	
		tpp::optional<A> a_;
		tpp::optional<B> b_;
	};
	
	template<typename Expr, typename Enable=void> struct attribute;
	template<typename Expr> using attribute_t = typename tpp::attribute<Expr>::type;

	namespace traits {
		// default implementation
		template<typename Arg0>
		struct terminal_attribute {
			using type = typename Arg0::attribute_type;
		};
	}// namespace traits
	template<typename Arg0>
	using terminal_attribute_t = typename tpp::traits::terminal_attribute<Arg0>::type;
	
	// terminal
	template<typename Expr>
	struct attribute<
		Expr,//=Terminal
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::terminal>::value
		>
	> {
		using type = tpp::terminal_attribute_t<tpp::expr_element_t<0, Expr>>;
	};
	
	template<std::size_t I, typename Expr>
	using attribute_of_element_t = attribute_t<tpp::expr_element_t<I, Expr>>;
	
	template<typename Expr, template<typename> class AttributeOf>
	using attribute_of_unary_expr =
		AttributeOf<
			tpp::attribute_of_element_t<0, Expr>
		>;
	template<typename Expr, template<typename> class AttributeOf>
	using attribute_of_unary_expr_t = 
		typename tpp::attribute_of_unary_expr<Expr, AttributeOf>::type;

	template<typename Expr, template<typename, typename> class AttributeOf>
	using attribute_of_binary_expr = 
		AttributeOf<
			tpp::attribute_of_element_t<0, Expr>,
			tpp::attribute_of_element_t<1, Expr>
		>;
	template<typename Expr, template<typename, typename> class AttributeOf>
	using attribute_of_binary_expr_t = 
		typename tpp::attribute_of_binary_expr<Expr, AttributeOf>::type;

	// kleene
	/*
	a: A --> *a: vector<A>
	a: Unused --> *a: Unused
	*/
	template<typename A>
	struct attribute_of_kleene {
		using type = std::vector<A>;
		static void push_back(type& t, A const& a) {
			t.push_back(a);
		}
	};
	template<>
	struct attribute_of_kleene<tpp::unused> {
		using type = tpp::unused;
		static void push_back(type, tpp::unused) {
			//do nothing
		}
	};

	template<typename Expr>
	struct attribute<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::dereference>::value
		>
	> {
		using type = tpp::attribute_of_unary_expr_t<Expr, tpp::attribute_of_kleene>;
		static void push_back(
			type& attribute, 
			tpp::attribute_of_element_t<0, Expr> const& a
		) {
			tpp::attribute_of_unary_expr<Expr, tpp::attribute_of_kleene>::
				push_back(attribute, a);
		}
	};

	// alternative
	/*
	a: A, b: B --> (a | b): variant<A, B>
	a: A, b: Unused --> (a | b): optional<A>
	a: A, b: B, c: Unused --> (a | b | c): optional<variant<A, B> >
	a: Unused, b: B --> (a | b): optional<B>
	a: Unused, b: Unused --> (a | b): Unused
	a: A, b: A --> (a | b): A
	*/
	template<typename A, typename B>
	struct attribute_of_alternative {
		using type = tpp::variant<A, B>;
		static auto construct(A const& a) -> type {
			return type(a);
		}
		static auto construct(B const& b) -> type {
			return type(b);
		}
	};
	template<typename A>
	struct attribute_of_alternative<A, tpp::unused> {
		using type = tpp::optional<A>;
		static auto construct(A const& a) -> type {
			return type(a);
		}
	};
	template<typename A>
	struct attribute_of_alternative<tpp::unused, A> {
		using type = tpp::optional<A>;
		static auto construct(A const& a) -> type {
			return type(a);
		}
	};
	template<>
	struct attribute_of_alternative<tpp::unused, tpp::unused> {
		using type = tpp::unused;
		static auto construct(tpp::unused) -> type {
			return tpp::unused();
		}
	};
	template<typename A>
	struct attribute_of_alternative<A, A> {
		using type = A;
		static auto construct(A const& a) -> type {
			return a;
		}
	};
	
	template<typename Expr>
	struct attribute<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::bitwise_or>::value
		>
	> {
		using type = tpp::attribute_of_binary_expr_t<Expr, tpp::attribute_of_alternative>;
		template<typename T>
		static auto construct(T const& t) {
			return tpp::attribute_of_binary_expr<Expr, tpp::attribute_of_alternative>
				::construct(t);
		}
	};

	// sequence
	/*
	a: A, b: B --> (a >> b): tuple<A, B>
	a: A, b: Unused --> (a >> b): A
	a: Unused, b: B --> (a >> b): B
	a: Unused, b: Unused --> (a >> b): Unused

	a: A, b: A --> (a >> b): vector<A>
	a: vector<A>, b: A --> (a >> b): vector<A>
	a: A, b: vector<A> --> (a >> b): vector<A>
	a: vector<A>, b: vector<A> --> (a >> b): vector<A>
	*/
	template<typename A, typename B>
	struct attribute_of_sequence {
		using type = std::tuple<A, B>;
		static auto construct(A const& a, B const& b) -> type {
			return std::make_tuple(a, b);
		}
	};
	template<typename A>
	struct attribute_of_sequence<A, tpp::unused> {
		using type = A;
		static auto construct(A const& a, tpp::unused) -> type {
			return a;
		}
	};
	template<typename A>
	struct attribute_of_sequence<tpp::unused, A> {
		using type = A;
		static auto construct(tpp::unused, A const& b) -> type {
			return b;
		}
	};
	template<>
	struct attribute_of_sequence<tpp::unused, tpp::unused> {
		using type = tpp::unused;
		static auto construct(tpp::unused, tpp::unused) -> type {
			return tpp::unused();
		}
	};

	template<typename A>
	struct attribute_of_sequence<A, A> {
		using type = std::vector<A>;
		static auto construct(A const& a, A const& b) -> type {
			type attribute;
			attribute.push_back(a);
			attribute.push_back(b);
			return attribute;
		}
	};
	template<typename A>
	struct attribute_of_sequence<std::vector<A>, A> {
		using type = std::vector<A>;
		static auto construct(std::vector<A> a, A const& b) -> type {
			a.push_back(b);
			return a;
		}
	};
	template<typename A>
	struct attribute_of_sequence<A, std::vector<A>> {
		using type = std::vector<A>;
		static auto construct(A const& a, std::vector<A> b) -> type {
			b.insert(b.begin(), a);
			return b;
		}
	};
	template<typename A>
	struct attribute_of_sequence<std::vector<A>, std::vector<A>> {
		using type = std::vector<A>;
		static auto construct(std::vector<A> a, std::vector<A> const& b) -> type {
			std::copy(b.begin(), b.end(), std::back_inserter(a));
			return a;
		}
	};

	template<typename Expr>
	struct attribute<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::shift_right>::value
		>
	> {
		using type = tpp::attribute_of_binary_expr_t<Expr, tpp::attribute_of_sequence>;
		static auto construct(
			tpp::attribute_of_element_t<0, Expr> const& a, 
			tpp::attribute_of_element_t<1, Expr> const& b
		) -> type {
			return tpp::attribute_of_binary_expr<Expr, tpp::attribute_of_sequence>
				::construct(a, b);
		}
	};
	
	//
	// context
	//
	template<typename Iter>
	class context;
	template<typename Iter>
	bool is_empty(tpp::context<Iter> const& context) {
		return context.current() == context.last(); }
	template<typename Iter>
	class context {
	public:
		context(Iter first, Iter last) : 
			first_(first), last_(last), current_(first) {}
		Iter first() const { return first_; }
		Iter last() const { return last_; }
		Iter current() const { return current_; }

		void make_checkpoint() { checkpoint_stack_.push_back(current_); }
		void return_to_last_checkpoint() {
			current_ = checkpoint_stack_.back();
			checkpoint_stack_.pop_back();
		}
		void remove_last_checkpoint() {
			checkpoint_stack_.pop_back();
		}
		bool match(typename std::iterator_traits<Iter>::value_type value) {
			bool is_success = !is_empty(*this) && *current_ == value;
			if(is_success) { ++current_; }
			return is_success;
		}
	private:
		Iter first_, last_;
		Iter current_;
		std::vector<Iter> checkpoint_stack_;
	};

	template<typename Iter>
	auto make_context(Iter first, Iter last) -> decltype(auto) { 
		return tpp::context<Iter>(first, last);
	}

	//
	// eval_result
	//
	template<typename Attribute, typename Iter>
	class eval_result {
	public:
		using attribute_type = Attribute;

		eval_result(
			tpp::context<Iter>& context,
			attribute_type const& attribute
		) : context_(context), attribute_opt_(attribute) {}

		eval_result(tpp::context<Iter>& context) : 
			context_(context), attribute_opt_() {}

		bool is_success() const { return static_cast<bool>(attribute_opt_); }
		attribute_type attribute() const { assert(is_success()); return *attribute_opt_; }
	private:
		tpp::context<Iter>& context_;
		tpp::optional<attribute_type> attribute_opt_;
	};
	template<typename Attribute, typename Iter>
	auto make_eval_result(
		tpp::context<Iter>& context,
		Attribute const& attribute
	) -> decltype(auto) {
		return tpp::eval_result<Attribute, Iter>(context, attribute);
	}
	template<typename Attribute, typename Iter>
	auto make_false_eval_result(
		tpp::context<Iter>& context
	) -> decltype(auto) {
		return tpp::eval_result<Attribute, Iter>(context);
	}
	template<typename Attribute, typename Iter>
	auto operator<<(std::ostream& os, 
			tpp::eval_result<Attribute, Iter> const& eval_result) -> std::ostream& {
		os << "is_success:" << eval_result.is_success() << std::endl;
		return os;
	}

	//
	// eval
	//
	template<typename Expr, typename=void>
	struct eval_impl;

	template<typename Arg, typename Iter>
	auto eval(
		Arg const& arg,
		tpp::context<Iter>& context
	) -> decltype(auto)  {
		auto expr = tpp::make_terminal_or_pass_expr<Arg>::call(arg);
		return tpp::eval_impl<decltype(expr)>::call(expr, context);
	}

	template<typename Expr>
	struct is_attribute_unused {
		static constexpr bool value =
			std::is_same<tpp::attribute_t<Expr>, tpp::unused>::value;
	};

	// terminal
	template<typename Expr>
	struct eval_impl<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::terminal>::value
		>
	> {
		template<typename Iter>
		static auto call(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto) {
			return expr.template get<0>()(context); //TODO make traits
		}
	};

	// dereference
	template<typename Expr>
	struct eval_impl<
		Expr,
		std::enable_if_t<
			std::is_same<tpp::tag_t<Expr>, tpp::tag::dereference>::value
		>
	> {
		template<typename Iter>
		static auto call(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto) {
			tpp::attribute_t<Expr> attribute;
			while(true) {
				auto eval_result = tpp::eval(expr.template get<0>(), context);
				if(!eval_result.is_success()) {
					break;
				}
				tpp::attribute<Expr>::push_back(attribute, eval_result.attribute());
			}
			return tpp::make_eval_result(context, attribute);
		}
	};

	// bitwise_or
	template<typename Expr>
	struct eval_impl<
		Expr,
		std::enable_if_t<
			std::is_same<tpp::tag_t<Expr>, tpp::tag::bitwise_or>::value
		>
	> {
		template<typename Iter>
		static auto call(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto) {
			auto result0 = tpp::eval(expr.template get<0>(), context);
			if(result0.is_success()) {
				return tpp::make_eval_result(context,
					tpp::attribute_t<Expr>(result0.attribute()));
			}
			else {
				auto result1 = tpp::eval(expr.template get<1>(), context);
				if(result1.is_success()) {
					return tpp::make_eval_result(context,
						tpp::attribute_t<Expr>(result1.attribute()));
				}
			}
			return tpp::make_false_eval_result<tpp::attribute_t<Expr>>(context);
		}
	};
	
	// shift_right
	template<typename Expr>
	struct eval_impl<
		Expr,
		std::enable_if_t<
			std::is_same<tpp::tag_t<Expr>, tpp::tag::shift_right>::value
		>
	> {
		template<typename Iter>
		static auto call(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto) {
			auto result0 = tpp::eval(expr.template get<0>(), context);
			if(result0.is_success()) {
				auto result1 = tpp::eval(expr.template get<1>(), context);
				if(result1.is_success()) {
					return tpp::make_eval_result(
						context,
						tpp::attribute<Expr>::construct(
							result0.attribute(), result1.attribute())
					);
				}
			}
			return tpp::make_false_eval_result<tpp::attribute_t<Expr>>(context);
		}
	};

	template<typename Iter, typename Expr>
	auto parse(Iter first, Iter last, Expr const& expr) -> decltype(auto) {
		auto context = tpp::make_context(first, last);
		return tpp::eval(expr, context);
	}

	//
	// parser
	//
	template<typename Attribute>
	class parser_result {
	public:
		using attribute_type = Attribute;
		parser_result(bool is_success, attribute_type const& attribute) : 
			is_success_(is_success), attribute_(attribute) {}
		bool is_success() const { return is_success_; }
		attribute_type attribute() const { return attribute_; }
	private:
		bool is_success_;
		attribute_type attribute_;
	};
	template<typename Attribute>
	auto make_parser_result(
			bool is_success, Attribute const& attribute) -> decltype(auto)  {
		return tpp::parser_result<Attribute>(is_success, attribute);
	}
	template<typename InternalParser>
	class base_parser {
	public:
		using internal_parser_type = InternalParser;
		using attribute_type = typename InternalParser::attribute_type;
		using action_type = std::function<void (attribute_type)>;

		template<typename... Args>
		base_parser(Args const&... args) :
			internal_parser_(args...), action_() {}
		template<typename Iter>
		auto operator()(tpp::context<Iter>& context) const {
			context.make_checkpoint();
			auto parse_result = internal_parser_.parse(context);
			if(!parse_result.is_success()) {
				context.return_to_last_checkpoint();
				return tpp::make_false_eval_result<attribute_type>(context);
			}
			else {
				context.remove_last_checkpoint();
				if(action_) {
					action_(parse_result.attribute());
				}
				return tpp::make_eval_result(context, parse_result.attribute());
			}
		}
		base_parser& operator[](action_type action) {
			action_ = action;
			return *this;
		}

	private:
		internal_parser_type internal_parser_;
		action_type action_;
	};
	class literal_parser {
	public:
		using attribute_type = std::string;
		literal_parser(std::string const& word) : word_(word) {}
		template<typename Iter>
		auto parse(tpp::context<Iter>& context) const {
			for(auto c : word_) {
				if(!context.match(c)) {
					return tpp::make_parser_result(false, attribute_type());
				}
			}
			return tpp::make_parser_result(true, word_);
		}

	private:
		std::string word_;
	};
	using lit = tpp::base_parser<tpp::literal_parser>;

	template<typename Expr>
	auto operator*(Expr const& expr) -> decltype(auto) {
		return tpp::make_expr_impl<tpp::tag::dereference>(expr);
	}
	template<typename Expr0, typename Expr1>
	auto operator|(Expr0 const& expr0, Expr1 const& expr1) -> decltype(auto) {
		return tpp::make_expr_impl<tpp::tag::bitwise_or>(expr0, expr1);
	}
	template<typename Expr0, typename Expr1>
	auto operator>>(Expr0 const& expr0, Expr1 const& expr1) -> decltype(auto) {
		return tpp::make_expr_impl<tpp::tag::shift_right>(expr0, expr1);
	}
}// namespace tpp

#endif //TPP_TINY_PACKRAT_PARSER_HPP
