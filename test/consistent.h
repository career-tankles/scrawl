//
//  consistent.h - a consistent hash ring
//  Copyright (C) 2010 Martin Broadhurst 
//  www.martinbroadhurst.com
//

#ifndef CONSISTENT_H
#define CONSISTENT_H

#include <map>
#ifdef __GNUC__
#include <ext/hash_map>
#define HASH_NAMESPACE __gnu_cxx
#else
#include <hash_map>
#define HASH_NAMESPACE std
#endif
#include <string>
#include <sstream>

namespace
{

class StringifyException
{
};

template <class T>
std::string Stringify(const T& t)
{
	std::ostringstream os;
	if (!(os << t))
	{
		throw StringifyException();
	}
	return os.str();
}

template <>
std::string Stringify(const std::string& str)
{
	return str;
}

template <>
std::string Stringify(const char* const& str)
{
	return str;
}

}

namespace Consistent
{

class EmptyRingException
{
};

template <class Node, class Data, class Hash = HASH_NAMESPACE::hash<const char*> >
class HashRing
{
public:
	typedef std::map<size_t, Node> NodeMap;
	typedef std::map<std::string, unsigned int> NodeWeight;

	HashRing(unsigned int replicas)
		: replicas_(replicas), hash_(HASH_NAMESPACE::hash<const char*>())
	{
	}

	HashRing(unsigned int replicas, const Hash& hash)
		: replicas_(replicas), hash_(hash)
	{
	}

	size_t AddNode(const Node& node, unsigned int replicas=0);
	void RemoveNode(const Node& node);
	const Node& GetNode(const Data& data) const;

private:
	NodeMap ring_;
    NodeWeight ring_weight_;
	const unsigned int replicas_;
	Hash hash_;
};

template <class Node, class Data, class Hash>
size_t HashRing<Node, Data, Hash>::AddNode(const Node& node, unsigned int replicas)
{
	size_t hash;
    if(replicas <= 0) replicas = replicas_;
	std::string nodestr = Stringify(node);
    ring_weight_[nodestr] = replicas;
	for (unsigned int r = 0; r < replicas; r++) {
		hash = hash_((nodestr + Stringify(r)).c_str());
		ring_[hash] = node;
	}
	return hash;
}

template <class Node, class Data, class Hash>
void HashRing<Node, Data, Hash>::RemoveNode(const Node& node)
{
	std::string nodestr = Stringify(node);
    unsigned int replicas = ring_weight_[nodestr];
	for (unsigned int r = 0; r < replicas; r++) {
		size_t hash = hash_((nodestr + Stringify(r)).c_str());
		ring_.erase(hash);
	}
}

template <class Node, class Data, class Hash>
const Node& HashRing<Node, Data, Hash>::GetNode(const Data& data) const
{
	if (ring_.empty()) {
		throw EmptyRingException();
	}
	size_t hash = hash_(Stringify(data).c_str());
	typename NodeMap::const_iterator it;
	// Look for the first node >= hash
	it = ring_.lower_bound(hash);
	if (it == ring_.end()) {
		// Wrapped around; get the first node
		it = ring_.begin();
	}
	return it->second;
}

}

#endif // CONSISTENT_H
