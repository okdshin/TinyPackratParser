#ifndef TPP_TINY_PACKRAT_PARSER_HPP
#define TPP_TINY_PACKRAT_PARSER_HPP
//20141112
#include <tuple>
#include <type_traits>
#include <vector>
#include <functional>

namespace tpp {
	//
	// tag
	//
	namespace tag {
		struct terminal {}; struct unary_plus {}; struct negate {};
		struct dereference {}; struct address_of {}; struct logical_not {};
		struct shift_left {}; struct modulus {}; struct minus {};
		struct bitwise_or {}; struct mem_ptr {};
	}// namespace tag

	template<typename Expr> struct tag_of { using type = typename Expr::tag_type; };
	template<typename Expr> using tag_t = typename tpp::tag_of<Expr>::type;
	template<typename Expr, typename Tag>
	struct is_tag_same {
		static const bool value = std::is_same<tpp::tag_t<Expr>, Tag>::value;
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
		/*
		template<std::size_t I> auto get() -> decltype(auto) {
			return std::get<I>(args_); } 
		*/
	private:
		args_type args_;
	};
	template<typename Tag, typename... Args>
	auto make_expr(Args const&... args) -> decltype(auto) {
		return tpp::expr<Tag, Args...>(args...);}

	template<std::size_t I, typename Expr>
	using expr_element_t = std::tuple_element_t<I, typename Expr::args_type>;

	template<typename T>
	auto make_terminal(T const& t) -> decltype(auto) {
		return tpp::make_expr<tpp::tag::terminal>(t);
	}

	template<typename T>
	auto make_kleene(T const& t) -> decltype(auto) {
		return tpp::make_expr<tpp::tag::dereference>(t);
	}

	//
	// attribute
	//
	struct unused {};
	template<typename T>
	struct is_unused {
		static const bool value = std::is_same<T, tpp::unused>::value;
	};

	template<typename Expr, typename Enable=void> struct attribute {};
	template<typename Expr> using attribute_t = typename tpp::attribute<Expr>::type;
	template<typename Expr>
	struct is_attribute_unused {
		static const bool value = tpp::is_unused<tpp::attribute_t<Expr>>::value;
	};

	// terminal
	template<typename Expr>
	struct attribute<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::terminal>::value
		>
	> {
		using type = typename tpp::expr_element_t<0, Expr>::attribute_type;
	};

	// dereference
	// *unused -> unused
	// *A -> std::vector<A>
	template<typename Expr>
	struct attribute<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::dereference>::value
		>
	> {
	private:
		using element0_attribute_type = tpp::attribute_t<tpp::expr_element_t<0, Expr>>;
	public:
		using type = std::conditional_t<tpp::is_unused<element0_attribute_type>::value,
			tpp::unused,
			std::vector<element0_attribute_type>
		>;
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
		return tpp::context<Iter>(first, last); }

	//
	// eval_result
	//
	template<typename Attribute, typename Iter>
	class eval_result {
	public:
		using attribute_type = Attribute;
		eval_result(
			bool is_success, 
			attribute_type const& attribute, 
			tpp::context<Iter> const& context
		) : is_success_(is_success), attribute_(attribute), context_(context) {}
		bool is_success() const { return is_success_; }
		attribute_type attribute() const { return attribute_; }
	private:
		bool is_success_;
		attribute_type attribute_;
		tpp::context<Iter> context_;
	};
	template<typename Attribute, typename Iter>
	auto make_eval_result(
		bool is_success, 
		Attribute attribute, 
		tpp::context<Iter> const& context
	) -> decltype(auto) {
		return tpp::eval_result<Attribute, Iter>(is_success, attribute, context);
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

	template<typename Expr, typename Iter>
	auto eval(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto)  {
		return tpp::eval_impl<Expr>::call(expr, context);
	}

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
			return expr.template get<0>()(context);
		}
	};

	// dereference
	template<typename Expr>
	struct eval_impl<
		Expr,
		std::enable_if_t<
			tpp::is_tag_same<Expr, tpp::tag::dereference>::value
			&& tpp::is_attribute_unused<Expr>::value
		>
	> {
		template<typename Iter>
		static auto call(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto) {
			while(true) {
				auto eval_result = tpp::eval(expr.template get<0>(), context);
				if(!eval_result.is_success()) {
					break;
				}
			}
			return tpp::make_eval_result(true, tpp::unused(), context);
		}
	};
	template<typename Expr>
	struct eval_impl<
		Expr,
		std::enable_if_t<
			std::is_same<tpp::tag_t<Expr>, tpp::tag::dereference>::value
			&& !tpp::is_attribute_unused<Expr>::value
		>
	> {
		template<typename Iter>
		static auto call(Expr const& expr, tpp::context<Iter>& context) -> decltype(auto) {
			std::vector<tpp::attribute_t<tpp::expr_element_t<0, Expr>>> attribute;
			while(true) {
				auto eval_result = tpp::eval(expr.template get<0>(), context);
				if(!eval_result.is_success()) {
					break;
				}
				attribute.push_back(eval_result.attribute());
			}
			return tpp::make_eval_result(true, attribute, context);
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
			}
			else {
				context.remove_last_checkpoint();
				if(action_) {
					action_(parse_result.attribute());
				}
			}
			return tpp::make_eval_result(
				parse_result.is_success(), parse_result.attribute(), context);
		}
		base_parser& operator[](action_type action) {
			action_ = action;
			return *this;
		}

	private:
		internal_parser_type internal_parser_;
		action_type action_;
	};
	class literal {
	public:
		using attribute_type = std::string;
		literal(std::string const& word) : word_(word) {}
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
	using lit = tpp::base_parser<tpp::literal>;

	template<typename Expr>
	auto operator*(Expr const& expr) -> decltype(auto) {
		return tpp::make_kleene(expr);
	}
}// namespace tpp

#endif //TPP_TINY_PACKRAT_PARSER_HPP
