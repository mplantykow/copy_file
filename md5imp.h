#include <openssl/md5.h>
#include <iomanip>
#include <sstream>
#include <boost/iostreams/device/mapped_file.hpp>

const std::string md5_from_file(const std::string& path)
{
	unsigned char result[MD5_DIGEST_LENGTH];

	boost::iostreams::mapped_file_source src(path);
	MD5((unsigned char*)src.data(), src.size(), result);

	std::ostringstream sout;
	sout<<std::hex<<std::setfill('0');
	for (auto c: result) sout<<std::setw(2)<<(int)c;

	return sout.str();
}
