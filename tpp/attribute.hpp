#ifndef TPP_ATTRIBUTE_HPP
#define TPP_ATTRIBUTE_HPP
//20141129
#include <tpp/tag.hpp>
#include <tpp/expr.hpp>

namespace tpp {
	//
	// attribute
	//
	struct unused {};

	template<typename T>
	using optional = std::experimental::optional<T>;
	using std::experimental::nullopt;

	template<typename A, typename B>
	class variant {
	public:
		explicit variant(A const& a) : a_(a), b_() {}
		explicit variant(B const& b) : a_(), b_(b) {}

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
			using type = typename std::remove_reference_t<Arg0>::attribute_type;
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
}// namespace tpp

#endif //TPP_ATTRIBUTE_HPP
