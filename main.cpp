#include <iostream>
#include "tiny_packrat_parser.hpp"
#include <cxxabi.h>

class printer {
public:
	using attribute_type = tpp::unused;
	explicit printer(std::size_t id) : id_(id) {}
	template<typename Attribute>
	void operator()(Attribute const& attribute) const {
		std::cout << id_ << ":" << attribute << std::endl;
	}

private:
	std::size_t id_;
};

int main() {
	std::cout << "hello world" << std::endl;

	std::string source = "hehehehello";
	std::string source2 = "morning";
	std::cout << "source: " << source << std::endl;

	auto expr = 
		*tpp::make_terminal(tpp::lit("he")[printer(0)]) 
			>> tpp::make_terminal(tpp::lit("llo")[printer(1)]) 
		| tpp::make_terminal(tpp::lit("morning")[printer(2)])
		| tpp::make_terminal(tpp::lit("afternoon")[printer(2)]);
	auto result = tpp::parse(
		source.begin(), source.end(), expr
	);
	auto result2 = tpp::parse(
		source2.begin(), source2.end(), expr
	);
	std::cout << result << std::endl;
	//auto attribute = result.attribute();
	/*
	for(auto e : attribute) {
		std::cout << e << " " << std::flush;
	}
	*/
	//std::cout << std::endl;

}
