#include <iostream>
#include <fstream>
#include <chrono>
#include <cerrno>

#define SUCCESS 0

int main(int argc, char *argv[])
{
	static constexpr std::size_t buff_size{4096};
	char buffer[buff_size];

	if (argc != 3) {
		std::cout << "usage: copy_file <src-file> <dest-file>\n";
		return EINVAL;
	}

	std::ifstream in{argv[1]};
	if (!in.is_open())
	{
		std::cout << "Could not open input file" << std::endl;
		return EPERM;
	}

	//Remove file upfront to increase the performance
	remove(argv[2]);
	std::ofstream out{argv[2]};

	if (!out.is_open()) {
		std::cout << "Could not create output file\n";
		in.close();
		return EPERM;
	}

	while (in.read(buffer, buff_size)) {
		out.write(buffer, buff_size);
	}

	out.write(buffer, in.gcount());
	in.close();
	out.close();

	return SUCCESS;
}
