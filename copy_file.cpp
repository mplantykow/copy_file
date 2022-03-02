#include <iostream>
#include <fstream>
#include <chrono>
#include <cerrno>
#include <filesystem>
#include <string>
#include "md5imp.h"

#define SUCCESS 0

int main(int argc, char *argv[])
{
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::duration<float> fsec;
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

	auto t_start = Time::now();

	while (in.read(buffer, buff_size)) {
		out.write(buffer, buff_size);
	}

	out.write(buffer, in.gcount());
	in.close();
	out.close();

	std::string inmd5 = md5_from_file(argv[1]);
	std::string outmd5 = md5_from_file(argv[2]);

	if (inmd5.compare(outmd5) != 0) {
		remove(argv[2]);
		return EIO;
	}

	auto t_end = Time::now();
	fsec t_elapsed = t_end - t_start;
	float speed = std::filesystem::file_size(argv[1])/t_elapsed.count();

	std::cout << "Average data transfer rate: " << speed << "B/s\n";

	return SUCCESS;
}
