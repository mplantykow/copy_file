#include <iostream>
#include <fstream>
#include <chrono>
#include <cerrno>
#include <filesystem>
#include <string>
#include "md5imp.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#define SUCCESS 0
#define NUM_BUFF 8
#define BUFF_SIZE 4096

class Descriptor
{
	public:
		Descriptor(char* data, int size, bool last);
		Descriptor(const Descriptor& other);
		Descriptor(Descriptor&& other);

		std::vector<char> getVector() const;
		bool getIsLast() const;

	private:
		std::vector<char> vector;
		bool isLast;
};

Descriptor::Descriptor(char* data, int size, bool last) : vector(data, data + size), isLast(last)
{
}

Descriptor::Descriptor(const Descriptor& other)
{
	this->vector = other.vector;
	this->isLast = other.isLast;
}

Descriptor::Descriptor(Descriptor&& other) : vector(std::move(other.vector)), isLast(std::move(other.isLast))
{
}

std::vector<char> Descriptor::getVector() const
{
	return this->vector;
}

bool Descriptor::getIsLast() const
{
	return this->isLast;
}

std::condition_variable queue_not_empty;
std::condition_variable queue_not_full;
std::queue<Descriptor> queue;
std::mutex queue_mtx;

void file_read(std::ifstream* in)
{
	char data[BUFF_SIZE];
	do{
		in->read(data, BUFF_SIZE);
		Descriptor desc(data, in->gcount(), !*in);
		/* unique_lock is used as the waiting thread must
		 * unlock the mutex when it is waiting */
		std::unique_lock<std::mutex> lk(queue_mtx);
		queue_not_full.wait(lk,[]{return queue.size() < NUM_BUFF;});
		queue.push(std::move(desc));
		queue_not_empty.notify_one();
		lk.unlock();
	} while(*in);
}

void file_write(std::ofstream* out)
{
	bool last = false;
	do {

		std::unique_lock<std::mutex> lk(queue_mtx);
		queue_not_empty.wait(lk,[]{return !queue.empty();});
		Descriptor desc_write = queue.front();
		queue.pop();
		queue_not_full.notify_one();
		lk.unlock();
		last = desc_write.getIsLast();
		out->write(desc_write.getVector().data(), desc_write.getVector().size());
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

	std::thread th1 (file_read, &in);
	std::thread th2 (file_write, &out);

	th1.join();
	th2.join();

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
