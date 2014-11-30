#ifndef TPP_EXPR_HPP
#define TPP_EXPR_HPP
//20141129

namespace tpp {
	//
	// expr
	//
	template<typename Tag, typename... Args>
	struct expr {
		using tag_type = Tag;
		using args_type = std::tuple<Args...>;

		// using const& but not && to avoid perfect forwarding copy constructor of death
		explicit expr(Args const&... args) : args_(args...) {}

		template<std::size_t I> auto get() const -> decltype(auto) {
			return std::get<I>(args_);
		} 

	private:
		args_type args_;
	};

	template<typename Tag, typename... Args>
	auto make_expr_impl(Args&&... args) -> decltype(auto) {
		return tpp::expr<Tag, std::decay_t<Args>...>(std::forward<Args>(args)...);
	}

	template<typename T>
	struct is_expr { static constexpr bool value = false; };

	template<typename Tag, typename... Args>
	struct is_expr<tpp::expr<Tag, Args...>> {
		static constexpr bool value = true;
	};
	
	template<typename T>
	auto make_terminal(T&& t) -> decltype(auto) {
		return tpp::make_expr_impl<tpp::tag::terminal>(std::forward<T>(t));
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
		template<typename A>
		static auto call(A&& a) -> decltype(auto) {
			return tpp::make_terminal(std::forward<A>(a));
		}
	};
	template<typename Tag, typename... Args>
	auto make_expr(Args&&... args) -> decltype(auto) {
		return tpp::make_expr_impl<Tag>(
			tpp::make_terminal_or_pass_expr<Args>::call(std::forward<Args>(args))...);
	}

	template<std::size_t I, typename Expr>
	using expr_element_t =
		std::tuple_element_t<I, typename std::remove_reference_t<Expr>::args_type>;
	
	template<typename Expr>
	auto operator*(Expr expr) -> decltype(auto) {
		return tpp::make_expr_impl<tpp::tag::dereference>(std::move(expr));
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

#endif //TPP_EXPR_HPP
