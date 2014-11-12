#include <iostream>
#include "tiny_packrat_parser.hpp"

class print_parser {
public:
	using attribute_type = tpp::unused;

	template<typename Iter>
	bool operator()(tpp::context<Iter>& context) const {
		std::cout << std::string(context.first(), context.last()) << std::endl;
		return false;
	}
};
int main() {
	std::cout << "hello world" << std::endl;

	std::string source = "hehehehello";
	auto printer = [](std::string const& word) { std::cout << ":" << word << std::endl; };

	auto result = tpp::parse(
		source.begin(), source.end(), 
		*tpp::make_terminal(tpp::lit("he")[printer])
	);
	std::cout << result << std::endl;
	auto attribute = result.attribute();
	for(auto e : attribute) {
		std::cout << e << " " << std::flush;
	}

}
