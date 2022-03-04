#include <iostream>
#include <fstream>
#include <chrono>
#include <cerrno>
#include <filesystem>
#include <string>
#include "md5imp.h"
#include <queue>

#define SUCCESS 0

#define BUFF_SIZE 4096

class Descriptor
{
	public:
		char data[BUFF_SIZE];
		int size;
		bool last;
		Descriptor(char p[BUFF_SIZE], int size, bool last);
};

Descriptor::Descriptor(char p[BUFF_SIZE], int size, bool last)
{
	memcpy(this->data, p, BUFF_SIZE);
	this->size = size;
	this->last = last;
}

std::queue<Descriptor> queue;

void file_read(std::ifstream* in)
{
	do{
		char data[BUFF_SIZE];
		in->read(data, BUFF_SIZE);
		int size = in->gcount();
		bool last = !*in;
		queue.emplace(data, size, last);
	} while(*in);
}

void file_write(std::ofstream* out)
{
	bool last = false;
	do {
		bool is_empty = queue.empty();
		if (!is_empty) {
			Descriptor pwrite = queue.front();
			last = pwrite.last;
			out->write(pwrite.data, pwrite.size);
			queue.pop();
		}
	} while (!last);
}

int main(int argc, char *argv[])
{
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::duration<float> fsec;

	if (argc != 3) {
		std::cout << "usage: copy_file <src-file> <dest-file>" << std::endl;
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
		std::cout << "Could not create output file" << std::endl;
		in.close();
		return EPERM;
	}

	auto t_start = Time::now();

	file_read(&in);
	file_write(&out);

	auto t_end = Time::now();

	in.close();
	out.close();

	std::string inmd5 = md5_from_file(argv[1]);
	std::string outmd5 = md5_from_file(argv[2]);

	if (inmd5.compare(outmd5) != 0) {
		remove(argv[2]);
		return EIO;
	}

	fsec t_elapsed = t_end - t_start;
	float speed = std::filesystem::file_size(argv[1])/t_elapsed.count();
	float speedMiB = speed / (1024*1024);

	std::cout << "Average data transfer rate: " << speedMiB << "MiB/s" << std::endl;

	return SUCCESS;
}
