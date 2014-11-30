#include <iostream>
#include <tpp/tiny_packrat_parser.hpp>
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

	std::string source = "hehello";
	std::string source2 = "morning";
	std::cout << "source: " << source << std::endl;

	auto printer0 = printer(0);
	auto he = tpp::lit("he")[printer0];
	/*
	auto expr = 
		*tpp::make_terminal(tpp::lit("he")[printer(1)]) 
			>> tpp::make_terminal(tpp::lit("xxx")[printer(2)]) 
		| *tpp::make_terminal(tpp::lit("he")[printer(11)])
			>> tpp::make_terminal(tpp::lit("llo")[printer(12)])
		| tpp::make_terminal(tpp::lit("hehehehello")[printer(31)])
		| tpp::make_terminal(tpp::lit("afternoon")[printer(41)]);
	*/
	auto expr = 
		tpp::make_terminal(tpp::lit("hehex"))
		| *tpp::make_terminal(tpp::lit("he")[printer(0)]) >> 
			(
			 (tpp::make_terminal(tpp::lit("l")[printer(1)]) >> tpp::make_terminal(tpp::lit("xx")[printer(2)]))
			 | tpp::make_terminal(tpp::lit("llo")[printer(11)])
			);
	//auto printer0 = printer(0);
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
