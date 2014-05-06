/* vim: set tabstop=4 : */
#ifndef __febird_statistic_time_h__
#define __febird_statistic_time_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "../stdtypes.hpp"
#include <boost/thread/mutex.hpp>
#include <iostream>

#ifndef POSIX_TIME_HPP___
# include <boost/date_time/posix_time/posix_time.hpp>
#endif

namespace febird {

class FEBIRD_DLL_EXPORT StatisticTime
{
public:
	StatisticTime(const std::string& title = "StatisticTime",
				  std::ostream& fp = std::cout,
				  bool printSameLine = false,
				  boost::mutex* cout_mutex = 0);
	~StatisticTime();

	void open(const std::string& title = "StatisticTime",
			  std::ostream& fp = std::cout,
			  bool printSameLine = false,
			  boost::mutex* cout_mutex = 0);

	void close();

	boost::posix_time::time_duration td() const { return time2 - time1; }
	uint64_t millis() const { return (time2 - time1).total_milliseconds(); }

private:
	void open_imp(const std::string& title, std::ostream& fp, bool printSameLine);
	void close_imp();	

private:
	std::string title;
	std::ostream* m_fp;

	boost::posix_time::ptime time1;
	boost::posix_time::ptime time2;
	boost::mutex* cout_mutex;
	bool isClosed;
	bool printSameLine;
};

} // namespace febird

#endif // __febird_statistic_time_h__

