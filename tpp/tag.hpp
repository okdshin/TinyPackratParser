#ifndef TPP_TAG_HPP
#define TPP_TAG_HPP
//20141129

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

	template<typename Expr> struct tag_of {
		using type = typename std::remove_reference_t<Expr>::tag_type;
	};
	template<typename Expr> using tag_t = typename tpp::tag_of<Expr>::type;
	template<typename Expr, typename Tag>
	struct is_tag_same {
		static constexpr bool value = std::is_same<tpp::tag_t<Expr>, Tag>::value;
	};
	
}// namespace tpp

#endif //TPP_TAG_HPP
