/* vim: set tabstop=4 : */
#ifndef __febird_pipeline_hpp__
#define __febird_pipeline_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
# pragma warning(push)
# pragma warning(disable: 4018)
# pragma warning(disable: 4267)
#endif

#include "config.hpp"
#include <stddef.h>
#include <vector>
#include <string>
#include <boost/thread.hpp>
#include <boost/function.hpp>

namespace febird {

class FEBIRD_DLL_EXPORT PipelineTask
{
public:
	virtual ~PipelineTask();
};

class FEBIRD_DLL_EXPORT PipelineQueueItem
{
public:
	uintptr_t plserial;
	PipelineTask* task;

	PipelineQueueItem(uintptr_t plserial, PipelineTask* task)
		: plserial(plserial)
		, task(task)
	{}

	PipelineQueueItem()
		: plserial(0)
		, task(0)
	{}
};

class FEBIRD_DLL_EXPORT PipelineProcessor;

class FEBIRD_DLL_EXPORT PipelineStep
{
	friend class PipelineProcessor;

public:
//	typedef concurrent_queue<std::deque<PipelineQueueItem> > queue_t;
	class queue_t;

protected:
	queue_t* m_out_queue;

	PipelineStep *m_prev, *m_next;
	PipelineProcessor* m_owner;

	struct FEBIRD_DLL_EXPORT ThreadData
	{
		std::string m_err_text;
		volatile int m_run; // int maybe faster than bool

		ThreadData() : m_run(false) {}
	};
	std::vector<boost::thread*> m_threads;
	std::vector<ThreadData> m_threadsData;
	enum {
		ple_none,
		ple_generate,
		ple_keep
	} m_pl_enum;
	uintptr_t m_plserial;

	void run_wrapper(int threadno);

	void run_step_first(int threadno);
	void run_step_last(int threadno);
	void run_step_mid(int threadno);

	void run_serial_step_slow(int threadno, void (PipelineStep::*fdo)(PipelineQueueItem&));
	void run_serial_step_fast(int threadno, void (PipelineStep::*fdo)(PipelineQueueItem&));
	void serial_step_do_mid(PipelineQueueItem& item);
	void serial_step_do_last(PipelineQueueItem& item);

	bool isPrevRunning();
	bool isRunning();
	void start(int queue_size);
	void wait();
	void stop();

protected:
	virtual void process(int threadno, PipelineQueueItem* task) = 0;

	virtual void setup(int threadno);
	virtual void clean(int threadno);

	virtual void run(int threadno);
	virtual void onException(int threadno, const std::exception& exp);

public:
	std::string m_step_name;

	//! @param thread_count 0 indicate keepSerial, -1 indicate generate serial
	explicit PipelineStep(int thread_count);

	virtual ~PipelineStep();

	int step_ordinal() const;
	const std::string& err(int threadno) const;

	// helper functions:
	std::string msg_leading(int threadno) const;
	boost::mutex* getMutex() const;
	int getInQueueSize()  const;
	int getOutQueueSize() const;
	void setOutQueueSize(int size);
};

class FEBIRD_DLL_EXPORT FunPipelineStep : public PipelineStep
{
	boost::function3<void, PipelineStep*, int, PipelineQueueItem*> m_process; // take(this, threadno, task)
	boost::function2<void, PipelineStep*, int> m_setup; // take(this, threadno)
	boost::function2<void, PipelineStep*, int> m_clean; // take(this, threadno)

	void process(int threadno, PipelineQueueItem* task);
	void setup(int threadno);
	void clean(int threadno);

	void default_setup(int threadno);
	void default_clean(int threadno);
	static void static_default_setup(PipelineStep* self, int threadno);
	static void static_default_clean(PipelineStep* self, int threadno);

public:
	FunPipelineStep(int thread_count,
					const boost::function3<void, PipelineStep*, int, PipelineQueueItem*>& fprocess,
					const boost::function2<void, PipelineStep*, int>& fsetup,
					const boost::function2<void, PipelineStep*, int>& fclean);
	FunPipelineStep(int thread_count,
					const boost::function3<void, PipelineStep*, int, PipelineQueueItem*>& fprocess,
					const std::string& step_name = "");
};

class FEBIRD_DLL_EXPORT PipelineProcessor
{
	friend class PipelineStep;

	PipelineStep *m_head;
	int m_queue_size;
	int m_queue_timeout;
	boost::function1<void, PipelineTask*> m_destroyTask;
	boost::mutex* m_mutex;
	volatile int m_run; // int maybe faster than bool
	bool m_is_mutex_owner;
	bool m_keepSerial;

protected:
	static void defaultDestroyTask(PipelineTask* task);
	virtual void destroyTask(PipelineTask* task);

	void add_step(PipelineStep* step);
	void clear();

public:
	bool m_silent; // set to true to depress status messages

	PipelineProcessor();

	virtual ~PipelineProcessor();

	int isRunning() const { return m_run; }

	void setQueueSize(int queue_size) { m_queue_size = queue_size; }
	int  getQueueSize() const { return m_queue_size; }
	void setQueueTimeout(int queue_timeout) { m_queue_timeout = queue_timeout; }
	int  getQueueTimeout() const { return m_queue_timeout; }
	void setDestroyTask(const boost::function1<void, PipelineTask*>& fdestory) { m_destroyTask = fdestory; }
	void setMutex(boost::mutex* pmutex);
	boost::mutex* getMutex() { return m_mutex; }

	std::string queueInfo();

	int step_ordinal(const PipelineStep* step) const;
	int total_steps() const;

	PipelineProcessor& operator| (PipelineStep* step) { this->add_step(step); return *this; }
	PipelineProcessor& operator>>(PipelineStep* step) { this->add_step(step); return *this; }

	void start();
	void compile(); // input feed from external, not first step
	void compile(int input_feed_queue_size /* default = m_queue_size */);

	void send(PipelineTask* task);

	void stop() { m_run = false; }
	void wait();
	int getInQueueSize(int step_no) const;
};

#define PPL_STEP_EX_0(pObject, Class, MemFun, thread_count) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _setup), pObject, _1, _2)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _clean), pObject, _1, _2)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define PPL_STEP_0(pObject, Class, MemFun, thread_count) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3)\
		, BOOST_STRINGIZE(Class::MemFun)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define PPL_STEP_EX_1(pObject, Class, MemFun, thread_count, arg1) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3, arg1)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _setup), pObject, _1, _2)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _clean), pObject, _1, _2)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define PPL_STEP_1(pObject, Class, MemFun, thread_count, arg1) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3, arg1)\
		, BOOST_STRINGIZE(Class::MemFun)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define PPL_STEP_EX_2(pObject, Class, MemFun, thread_count, arg1, arg2) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3, arg1, arg2)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _setup), pObject, _1, _2)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _clean), pObject, _1, _2)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define PPL_STEP_2(pObject, Class, MemFun, thread_count, arg1, arg2) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3, arg1, arg2)\
		, BOOST_STRINGIZE(Class::MemFun)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define PPL_STEP_EX_3(pObject, Class, MemFun, thread_count, arg1, arg2, arg3) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3, arg1, arg2, arg3)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _setup), pObject, _1, _2)\
		, boost::bind(&Class::BOOST_JOIN(MemFun, _clean), pObject, _1, _2)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define PPL_STEP_3(pObject, Class, MemFun, thread_count, arg1, arg2, arg3) \
	new febird::FunPipelineStep(thread_count\
		, boost::bind(&Class::MemFun, pObject, _1, _2, _3, arg1, arg2, arg3)\
		, BOOST_STRINGIZE(Class::MemFun)\
		)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace febird

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma warning(pop)
#endif

#endif // __febird_pipeline_hpp__

