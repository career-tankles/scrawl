/* vim: set tabstop=4 : */
#include "statistic_time.hpp"

namespace febird {

	using namespace std;

	StatisticTime::StatisticTime(const std::string& title, std::ostream& fp, bool printSameLine, boost::mutex* cout_mutex)
	{
		isClosed = true;
		open(title, fp, printSameLine, cout_mutex);
	}

	void StatisticTime::open(const std::string& title, std::ostream& fp, bool printSameLine, boost::mutex* cout_mutex)
	{
		this->cout_mutex = cout_mutex;
		if (cout_mutex) {
			boost::mutex::scoped_lock lock(*cout_mutex);
			open_imp(title, fp, printSameLine);
		} else
			open_imp(title, fp, printSameLine);
	}
	void StatisticTime::close()
	{
		if (cout_mutex) {
			boost::mutex::scoped_lock lock(*cout_mutex);
			close_imp();
		} else
			close_imp();
	}

	StatisticTime::~StatisticTime()
	{
		if (!isClosed) close();
	}

	void StatisticTime::open_imp(const std::string& title, std::ostream& fp, bool printSameLine)
	{
		if (!isClosed) close();

		this->printSameLine = printSameLine;
		m_fp = &fp;
		fp << title << " begin ...";
		if (printSameLine)
			fp.flush();
		else
			fp << std::endl;

		this->title = title;
		time1 = boost::posix_time::microsec_clock::local_time();
		isClosed = false;
	}
	void StatisticTime::close_imp()
	{
		time2 = boost::posix_time::microsec_clock::local_time();
		boost::posix_time::time_duration td = (time2 - time1);
		uint64_t mills = td.total_milliseconds();
		uint64_t sec = (int)(mills / 1000);
		*m_fp << (printSameLine ? "," : title)
			<< " completed, " << mills << " ms, h:m:s.ms=["
			<< setw(2) << setfill('0') << sec/3600 << "."
			<< setw(2) << setfill('0') << sec/60%60 << "."
			<< setw(2) << setfill('0') << sec%60 << "."
			<< setw(3) << setfill('0') << mills%1000 << "]"
			;
		if (printSameLine)
			*m_fp << endl;
		else
			m_fp->flush();

		isClosed = true;
	}

} // namespace febird

