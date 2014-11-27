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

	auto printer0 = printer(0);
	auto he = tpp::lit("he")[printer0];
	auto expr = 
		*tpp::make_terminal(he) 
			>> tpp::make_terminal(tpp::lit("llo")[printer(1)]) 
		| tpp::make_terminal(tpp::lit("morning")[printer(2)])
		| tpp::make_terminal(tpp::lit("afternoon")[printer(2)]);
	//auto printer0 = printer(0);
	printer0("aaaa");

	/*
	auto l = tpp::lit("he")[printer0];
	auto l2 = tpp::lit("llo")[printer0];
	auto expr = *(tpp::make_terminal(l) | tpp::make_terminal(l2));
	*/
	auto result = tpp::parse(
		source.begin(), source.end(), expr
	);
	std::cout << result << std::endl;
	/*
	auto result2 = tpp::parse(
		source2.begin(), source2.end(), expr
	);
	std::cout << result2 << std::endl;
	*/
	//auto attribute = result.attribute();
	/*
	for(auto e : attribute) {
		std::cout << e << " " << std::flush;
	}
	*/
	//std::cout << std::endl;

}
