#ifndef TPP_TINY_PACKRAT_PARSER_HPP
#define TPP_TINY_PACKRAT_PARSER_HPP
//20141112
#include <cassert>
#include <utility>
#include <type_traits>
#include <tuple>
#include <map>
#include <vector>
#include <functional>
#include<experimental/optional>
#include <tpp/attribute.hpp>
#include <tpp/tag.hpp>

namespace tpp {
	template<typename Attribute, typename Iter>
	class eval_result;

	//
	// context
	//
	template<typename Iter>
	class context {
	public:
		context(Iter first, Iter last) : 
			first_(first),
			last_(last), 
			current_(first_) {}

		Iter first() const { return first_; }
		Iter last() const { return last_; }
		Iter current() const { return current_; }

		void make_checkpoint() { checkpoint_stack_.push_back(current_); }
		void return_to_last_checkpoint() {
			current_ = checkpoint_stack_.back();
			checkpoint_stack_.pop_back();
		}
		bool match(typename std::iterator_traits<Iter>::value_type const& value) {
			bool is_success = (current_!=last_&& *current_==value);
			if(is_success) { ++current_; }
			return is_success;
		}
		bool is_speculating() const {
			return !checkpoint_stack_.empty();
		}
		template<typename Iterator>
		friend std::ostream& operator<<(
			std::ostream& os, tpp::context<Iterator> const& context);

	private:
		Iter first_, last_;
		Iter current_;
		std::vector<Iter> checkpoint_stack_;
	};
	template<typename Iterator>
	std::ostream& operator<<(std::ostream& os, tpp::context<Iterator> const& context) {
		os << std::string(context.first_, context.current_) << "@" 
			<< std::string(context.current_, context.last_) << "\n";
		if(context.is_speculating()) {
			os << std::string(context.first_, context.checkpoint_stack_.back()) 
				<< "?" << std::flush;
		}
		else {
			os << "!" << std::flush;
			
		}
		return os;
	}

	template<typename Iter>
	auto make_context(Iter const& first, Iter const& last) -> decltype(auto) { 
		return tpp::context<Iter>(first, last);
	}

	//
	// eval_result
	//
	template<typename Attribute, typename Iter>
	class eval_result {
	public:
		using attribute_type = std::remove_reference_t<Attribute>;

		template<typename Attr>
		eval_result(tpp::context<Iter>& context, Attr&& attr) : 
			context_(context), attribute_opt_(std::forward<Attr>(attr)) {}

		explicit eval_result(tpp::context<Iter>& context) : 
			eval_result(context, tpp::nullopt) {}

		bool is_success() const { return static_cast<bool>(attribute_opt_); }
		attribute_type attribute() const { assert(is_success()); return *attribute_opt_; }
	private:
		tpp::context<Iter>& context_;
		tpp::optional<attribute_type> attribute_opt_;
	};
	template<typename Attribute, typename Iter>
	auto make_eval_result(
		tpp::context<Iter>& context,
		Attribute&& attribute
	) -> decltype(auto) {
		return tpp::eval_result<Attribute, Iter>(
			context, 
			std::forward<Attribute>(attribute)
		);
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
		return tpp::eval_impl<std::remove_cv_t<decltype(expr)>>::call(expr, context);
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
			context.make_checkpoint();
			std::cout << "enter alter: "<< context << std::endl;
			std::cout << "alter: try left" << std::endl;
			auto result0 = tpp::eval(expr.template get<0>(), context);
			if(result0.is_success()) {
				std::cout << "alter: choice left" << std::endl;
				context.return_to_last_checkpoint();
				auto result0 = tpp::eval(expr.template get<0>(), context);
				return tpp::make_eval_result(context,
					tpp::attribute_t<Expr>(result0.attribute()));
			}
			else {
				context.return_to_last_checkpoint();
				context.make_checkpoint();
				std::cout << "alter: try right" << std::endl;
				auto result1 = tpp::eval(expr.template get<1>(), context);
				if(result1.is_success()) {
					std::cout << "alter: choice right" << std::endl;
					context.return_to_last_checkpoint();
					auto result1 = tpp::eval(expr.template get<1>(), context);
					return tpp::make_eval_result(context,
						tpp::attribute_t<Expr>(result1.attribute()));
				}
				else {
					context.return_to_last_checkpoint();
					return tpp::make_false_eval_result<tpp::attribute_t<Expr>>(context);
				}
			}
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
							result0.attribute(), result1.attribute()
						)
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

		// avoid perfect forwarding copy constructor of death
		template<typename... Args>
		explicit base_parser(Args const&... args) :
			internal_parser_(args...), action_() {}

		template<typename Iter>
		auto operator()(tpp::context<Iter>& context) const {
			auto parse_result = internal_parser_.parse(context);
			if(!parse_result.is_success()) {
				return tpp::make_false_eval_result<attribute_type>(context);
			}
			else {
				if(!context.is_speculating() && action_) {
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

		explicit literal_parser(std::string word) : word_(std::move(word)) {}

		template<typename Iter>
		auto parse(tpp::context<Iter>& context) const -> decltype(auto) {
			std::cout << context << std::endl;
			std::cout << "lit: " << word_ << " ";
			for(auto c : word_) {
				if(!context.match(c)) {
					std::cout << "false" << std::endl;
					return tpp::make_parser_result(false, attribute_type());
				}
			}
			std::cout << "true" << std::endl;
			return tpp::make_parser_result(true, word_);
		}

	private:
		std::string word_;
	};
	using lit = tpp::base_parser<tpp::literal_parser>;

}// namespace tpp

#endif //TPP_TINY_PACKRAT_PARSER_HPP
