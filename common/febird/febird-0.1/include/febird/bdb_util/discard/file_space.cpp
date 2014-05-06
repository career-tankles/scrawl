#include "file_space.hpp"
#include "../io/DataIO.hpp"
#include "../io/FileStream.hpp"
#include "../io/load_save_file.hpp"

namespace febird {

template<class PositionT, class SizeT>
class SpaceManager_Impl
{
	typedef std::map<PositionT, SizeT> point_map_t;
	typedef std::set<std::pair<SizeT, PositionT> > space_tab_t;

	SizeT m_cell_size;
//	const SizeT m_cell_size;
	point_map_t m_free_ptr;
	space_tab_t	m_spac_tab;

	boost::function1<std::pair<PositionT,SizeT>, SizeT> m_make_new_space;

	DATA_IO_LOAD_SAVE_V(SpaceManager_Impl, 1,
		& m_cell_size
		& m_free_ptr
		& m_spac_tab)

	PositionT alloc_new(SizeT min_size, SizeT* max_size);
	std::pair<PositionT,SizeT> make_new_space_aux(SizeT new_size);

	std::pair<PositionT, SizeT> make_pos_size(PositionT pos, SizeT size)
	{
		assert(pos < PositionT(1024*1024*1024));
		assert(size < 1024*1024*1024);
		return std::make_pair(pos, size);
	}

	std::pair<SizeT, PositionT> make_size_pos(SizeT size, PositionT pos)
	{
		assert(size < 1024*1024*1024);
		assert(pos < PositionT(1024*1024*1024));
		return std::make_pair(size, pos);
	}

public:
	SpaceManager_Impl(SizeT cell_size, SizeT space_size, const boost::function1<std::pair<PositionT,SizeT>, SizeT>& make_new_space);
	SpaceManager_Impl(const boost::function1<std::pair<PositionT,SizeT>, SizeT>& make_new_space);
	~SpaceManager_Impl();

	PositionT alloc(SizeT size);
	PositionT alloc(PositionT hint_pos, SizeT old_size, SizeT min_size, SizeT* max_size);
	void free(PositionT pos, SizeT old_size);
};

template<class PositionT, class SizeT>
SpaceManager<PositionT, SizeT>::SpaceManager(SizeT cell_size,
					 SizeT init_space_size,
					 const boost::function1<std::pair<PositionT,SizeT>, SizeT>& make_new_space)
  : m_impl(new SpaceManager_Impl<PositionT, SizeT>(cell_size, init_space_size, make_new_space))
{
}

template<class PositionT, class SizeT>
SpaceManager<PositionT, SizeT>::SpaceManager(const boost::function1<std::pair<PositionT,SizeT>, SizeT>& make_new_space)
  : m_impl(new SpaceManager_Impl<PositionT, SizeT>(make_new_space))
{
}

template<class PositionT, class SizeT>
SpaceManager<PositionT, SizeT>::~SpaceManager()
{
	delete m_impl;
}

template<class PositionT, class SizeT>
PositionT SpaceManager<PositionT, SizeT>::alloc(SizeT size)
{
	return m_impl->alloc(size);
}
template<class PositionT, class SizeT>
PositionT SpaceManager<PositionT, SizeT>::alloc(PositionT hint_pos,
												SizeT  old_size,
												SizeT  min_size,
												SizeT* max_size)
{
	return m_impl->alloc(hint_pos, old_size, min_size, max_size);
}
template<class PositionT, class SizeT>
void SpaceManager<PositionT, SizeT>::free(PositionT pos, SizeT old_size)
{
	m_impl->free(pos, old_size);
}

//////////////////////////////////////////////////////////////////////////

template<class PositionT, class SizeT>
SpaceManager_Impl<PositionT, SizeT>::SpaceManager_Impl(
								SizeT cell_size,
								SizeT init_space_size,
								const boost::function1<std::pair<PositionT,SizeT>, SizeT>& make_new_space)
 : m_cell_size(cell_size)
 , m_make_new_space(make_new_space)
{
//	FEBIRD_RT_assert(cell_size==m_cell_size, std::runtime_error);
	if (!native_load_from_file(*this, "SpaceManager_Impl.bin", false))
	{
		std::pair<PositionT,SizeT> pos_size = make_new_space_aux(init_space_size);
		m_free_ptr.insert(make_pos_size(pos_size.first, pos_size.second));
		m_spac_tab.insert(make_size_pos(pos_size.second, pos_size.first));
	}
}

template<class PositionT, class SizeT>
SpaceManager_Impl<PositionT, SizeT>::SpaceManager_Impl(
	const boost::function1<std::pair<PositionT,SizeT>, SizeT>& make_new_space)
 : m_make_new_space(make_new_space)
{
	native_load_from_file(*this, "SpaceManager_Impl.bin");
}

template<class PositionT, class SizeT>
SpaceManager_Impl<PositionT, SizeT>::~SpaceManager_Impl()
{
	native_save_to_file(*this, "SpaceManager_Impl.bin");
}

template<class PositionT, class SizeT>
std::pair<PositionT,SizeT> SpaceManager_Impl<PositionT, SizeT>::make_new_space_aux(SizeT new_size)
{
	using namespace std;

	std::pair<PositionT,SizeT> pos_size = m_make_new_space(new_size);
	if (pos_size.second < new_size)
	{
		string_appender<> oss;
		oss << "make_new_space must return sufficient space, or throw exception."
			<< " space[req=" << new_size << ", ret=" << pos_size.second << "]";
		throw std::logic_error(oss.str().c_str());
	}
	typename point_map_t::iterator prev_ptr = m_free_ptr.lower_bound(pos_size.first);
	typename point_map_t::iterator next_ptr = m_free_ptr.lower_bound(pos_size.first + pos_size.second);
	if (m_free_ptr.end() != prev_ptr && pos_size.first < prev_ptr->first + prev_ptr->second ||
		m_free_ptr.end() != next_ptr && next_ptr->first < pos_size.first + pos_size.second)
	{
		// ���صĿռ���Ѵ��ڵĿ��п��ص�����˵�� make_new_space ��ʵ������
		//
		string_appender<> oss;
		oss << "make_new_space return space is already in free list."
			<< " space[pos=" << pos_size.first << ", size=" << pos_size.second << "]";
		throw std::logic_error(oss.str().c_str());
	}
	return pos_size;
}

template<class PositionT, class SizeT>
PositionT SpaceManager_Impl<PositionT, SizeT>::alloc_new(SizeT min_size, SizeT* max_size)
{
	using namespace std;

	assert( min_size % m_cell_size == 0);
	assert(*max_size % m_cell_size == 0);

	PositionT free_pos;
	typename space_tab_t::iterator iter = m_spac_tab.lower_bound(make_size_pos(min_size, PositionT(0)));
	if (m_spac_tab.end() == iter)
	{
		// not found free space with sufficient size
		// make new space
		//
		std::pair<PositionT,SizeT> pos_size = make_new_space_aux(min_size);
		free_pos = pos_size.first;
		if (pos_size.second <= *max_size)
		{
			*max_size = pos_size.second;
		}
		else // has more space, put it to free list
		{
			pos_size.first += *max_size;
			pos_size.second -= *max_size;
			m_free_ptr.insert(make_pos_size(pos_size.first, pos_size.second));
			m_spac_tab.insert(make_size_pos(pos_size.second, pos_size.first));
		}
	}
	else // found sufficient free space
	{
		free_pos = iter->second;
		SizeT free_size = iter->first;

		m_spac_tab.erase(iter);
		m_free_ptr.erase(m_free_ptr.find(free_pos));

		if (free_size > *max_size) {
			PositionT new_free_pos = free_pos + *max_size;
			SizeT new_free_size = free_size - *max_size;
			m_free_ptr.insert(make_pos_size(new_free_pos, new_free_size));
			m_spac_tab.insert(make_size_pos(new_free_size, new_free_pos));
		} else
			*max_size = free_size;
	}
	return free_pos;
}

template<class PositionT, class SizeT>
PositionT SpaceManager_Impl<PositionT, SizeT>::alloc(SizeT size)
{
	SizeT max_size = size = align_up(size, m_cell_size);
	return alloc_new(size, &max_size);
}

/**
 @brief ����ռ�
  
  ��� hint_pos ֮����ӵ��п��пռ䣬�ͷ��� hint_pos
  ���򷵻��·���Ŀ飬hint_pos ���ᱻ�ͷţ�������Ӧ��������ͷ� hint_pos
 */
template<class PositionT, class SizeT>
PositionT SpaceManager_Impl<PositionT, SizeT>::alloc(PositionT hint_pos,
													 SizeT  old_size,
													 SizeT  min_size,
													 SizeT* max_size)
{
	using namespace std;

	old_size = align_up(old_size, m_cell_size);
	min_size = align_up(min_size, m_cell_size);
	*max_size = align_up(*max_size, m_cell_size);

	// ���ҽ����� hint_pos ֮��Ŀ��п�
	PositionT next_free_pos = hint_pos + old_size;
	typename point_map_t::iterator next_ptr = m_free_ptr.find(next_free_pos);
	if (m_free_ptr.end() != next_ptr)
	{
		// (hint_pos, old_size) ���һ�����п�����
		SizeT next_free_size = next_ptr->second;
		typename space_tab_t::iterator next_spc = m_spac_tab.find(make_size_pos(next_ptr->second, next_ptr->first));
		if (m_spac_tab.end() == next_spc)
		{
			string_appender<> oss;
			oss << "fatal: " << __FILE__ << ":" << __LINE__ << ", δ�ҵ���Ӧ�� space ��Ŀ" << BOOST_CURRENT_FUNCTION;
			throw std::runtime_error(oss.str().c_str());
		}
		m_free_ptr.erase(next_ptr);
		m_spac_tab.erase(next_spc);
		if (old_size + next_free_size > *max_size)
		{ // ��������п�ǰ������һ���֣�����Ĳ����Կ���
			next_free_pos = hint_pos + *max_size;
			next_free_size -= (*max_size - old_size);
			m_free_ptr.insert(make_pos_size(next_free_pos, next_free_size));
			m_spac_tab.insert(make_size_pos(next_free_size, next_free_pos));
		} else {
			*max_size = SizeT(old_size + next_free_size);
		}
		return hint_pos;
	} else
		return alloc_new(min_size, max_size);
}

/**
 @brief ����˵��
  
 @param pos  Ҫ�ͷŵĿռ��ַ
 @param size ����ռ�Ĵ�С��������ȷ�ṩ�����С����������Ĵ�С��
 */
template<class PositionT, class SizeT>
void SpaceManager_Impl<PositionT, SizeT>::free(PositionT pos, SizeT size)
{
	using namespace std;

	size = align_up(size, m_cell_size);

	std::pair<PositionT,SizeT> spc = make_pos_size(pos, size);

	typename point_map_t::iterator next_ptr = m_free_ptr.lower_bound(pos);
	if (m_free_ptr.begin() != next_ptr)
	{
		typename point_map_t::iterator prev_ptr = next_ptr;
		--prev_ptr;
		if (prev_ptr->first + prev_ptr->second == pos)
		{
			// ��ǰһ�����п�����
			typename space_tab_t::iterator prev_spc = m_spac_tab.find(make_size_pos(prev_ptr->second, prev_ptr->first));
			if (m_spac_tab.end() == prev_spc)
			{
				string_appender<> oss;
				oss << "fatal: " << __FILE__ << ":" << __LINE__ << ", δ�ҵ���Ӧ�� space ��Ŀ" << BOOST_CURRENT_FUNCTION;
				throw std::runtime_error(oss.str().c_str());
			}
			// �ϲ�ǰһ�����п�
			spc.first = prev_ptr->first;
			spc.second += prev_ptr->second;
			m_free_ptr.erase(prev_ptr);
			m_spac_tab.erase(prev_spc);
		}
		else if (prev_ptr->first + prev_ptr->second > pos)
		{ // Ҫ�ͷŵĿ���ǰһ�����п��ص�
			string_appender<> oss;
			oss << "fatal: " << __FILE__ << ":" << __LINE__ << ", Ҫ�ͷŵĿ���ǰһ�����п��ص�:" << BOOST_CURRENT_FUNCTION;
			throw std::runtime_error(oss.str().c_str());
		}
	}
	if (m_free_ptr.end() != next_ptr)
	{
		if (next_ptr->first == pos)
		{ // Ҫ�ͷŵ� pos ���ڿ��б���
			string_appender<> oss;
			oss << "fatal: " << __FILE__ << ":" << __LINE__ << ", Ҫ�ͷŵ� pos ���ڿ��б���:" << BOOST_CURRENT_FUNCTION;
			throw std::runtime_error(oss.str().c_str());
		}
		else if (next_ptr->first < pos + size)
		{
			string_appender<> oss;
			oss << "fatal: " << __FILE__ << ":" << __LINE__
				<< ", Ҫ�ͷŵ� pos �����Ŀ��пռ��ص�: "
				<< "curr[ptr=" << pos << ", size=" << size << "], "
				<< "next[ptr=" << next_ptr->first << ", size=" << next_ptr->second << "]\n"
				<< "in func:" << BOOST_CURRENT_FUNCTION;
			throw std::runtime_error(oss.str().c_str());
		}
		else if (pos + size == next_ptr->first)
		{	// ���һ�����п�����
			typename space_tab_t::iterator next_spc = m_spac_tab.find(make_size_pos(next_ptr->second, next_ptr->first));
			if (m_spac_tab.end() == next_spc)
			{
				string_appender<> oss;
				oss << "fatal: " << __FILE__ << ":" << __LINE__ << ", δ�ҵ���Ӧ�� space ��Ŀ" << BOOST_CURRENT_FUNCTION;
				throw std::runtime_error(oss.str().c_str());
			}
			// �ϲ���һ�����п�
			spc.second += next_ptr->second;
			m_free_ptr.erase(next_ptr);
			m_spac_tab.erase(next_spc);
		}
	}
	m_free_ptr.insert(spc);
	m_spac_tab.insert(make_size_pos(spc.second, spc.first));
}

template class SpaceManager<stream_position_t, stream_position_t>; // FileSpace
//template SpaceManager<unsigned char*, unsigned int>; // MemSpace

} // namespace febird
