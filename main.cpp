#include <iostream>
#include "tiny_packrat_parser.hpp"
#include <cxxabi.h>

class print_parser {
public:
	using attribute_type = tpp::unused;

	template<typename Iter>
	bool operator()(tpp::context<Iter>& context) const {
		std::cout << std::string(context.first(), context.last()) << std::endl;
		return false;
	}
};

template<typename T> void print_type(std::string const& message) {
	int status;
	std::cout << message << "\t\t" << 
		abi::__cxa_demangle(typeid(T).name(), 0, 0, &status) << std::endl;
}
int main() {
	print_type<
		tpp::expr<tpp::tag::dereference,
			tpp::dammy_unused_attribute
		>
	>("kleene");
	print_type<
		tpp::attribute_t<
			tpp::expr<tpp::tag::shift_right,
				tpp::dammy_attribute<0>, tpp::dammy_attribute<1>
			>
		>
	>("sequence");
	print_type<std::tuple<tpp::dammy_attribute<0>, tpp::dammy_attribute<1>>>("tuple");
	
	std::cout << "hello world" << std::endl;

	std::string source = "hehehehello";
	std::cout << "source: " << source << std::endl;
	auto printer = [](std::string const& word) { std::cout << ":" << word << std::endl; };

	auto result = tpp::parse(
		source.begin(), source.end(), 
		//*tpp::make_terminal(tpp::lit("he")[printer])
		*tpp::lit("he")[printer]
	);
	std::cout << result << std::endl;
	auto attribute = result.attribute();
	for(auto e : attribute) {
		std::cout << e << " " << std::flush;
	}
	std::cout << std::endl;

}
