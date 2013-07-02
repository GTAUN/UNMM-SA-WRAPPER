/**
 * Copyright (C) 2013 MK124
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GTAUN_UNMM_COVERINGMAPMANAGER_HPP
#define GTAUN_UNMM_COVERINGMAPMANAGER_HPP

#include <set>

#include <boost/geometry/index/rtree.hpp>
#include <boost/system/error_code.hpp>

namespace gtaun {
namespace unmm {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class CoveringMapManager
{
public:
	class CoveringDataEntry
	{
	public:
		const size_t order;
		const size_t offset;
		const size_t size;
		const void* data;

	private:
		CoveringMapManager* manager;

	public:
		CoveringDataEntry(CoveringMapManager* manager, size_t orde, size_t offset, size_t sizer, void* data) :
			manager(manager), order(order), offset(offset), size(size), data(data)
		{
		}

		~CoveringDataEntry()
		{
			if (manager != nullptr) manager->unregisterCoveringBlock(this);
		}
	};

private:
	typedef bg::model::point<size_t, 1, bg::cs::cartesian> point_type;
	typedef bg::model::box<point_type> range_type;
	typedef std::pair<range_type, CoveringDataEntry*> rtree_value_type;
	typedef bgi::rtree<rtree_value_type, bgi::quadratic<16>> rtree_type;

public:

private:
	rtree_type tree;
	size_t orderCount;

public:
	CoveringMapManager() : orderCount(0)
	{
	}

	~CoveringMapManager()
	{
		orderCount = -1;
		query(bgi::covered_by(tree.bounds()), [&] (const rtree_value_type& v) { delete v.second; });
	}

	CoveringDataEntry* registerCoveringBlock(size_t offset, size_t size, void* data)
	{
		size_t end = offset + size - 1;

		CoveringDataEntry* entry = new CoveringDataEntry(this, orderCount, offset, size, data);
		range_type range(offset, end);
		tree.insert(std::make_pair(range, entry));

		orderCount++;
		return entry;
	}

	inline void query(size_t offset, size_t size, boost::function<void(CoveringDataEntry*)> out)
	{
		size_t end = offset + size - 1;
		range_type range(offset, end);
		query(bgi::intersects(range), [&] (const rtree_value_type& v) { out(v.second); });
	}

	std::vector<CoveringDataEntry*> querySorted(size_t offset, size_t size)
	{
		std::vector<CoveringDataEntry*> vec;
		query(offset, size, [&] (CoveringDataEntry* e) { vec.push_back(e); });

		struct  
		{
			bool operator() (CoveringDataEntry* lhs, CoveringDataEntry* rhs)
			{   
				return lhs->order < rhs->order;
			}
		} entry_order_less;
		std::sort(vec.begin(), vec.end(), entry_order_less);
		return vec;
	}

private:
	CoveringMapManager(const CoveringMapManager&);
	CoveringMapManager& operator=(const CoveringMapManager&);
	
	template<typename Predicates>
	void query(Predicates &predicates, boost::function<void(const rtree_value_type&)> out)
	{

		class out_iterator
		{
		private:
			boost::function<void(const rtree_value_type&)> &out;

		public:
			typedef out_iterator _Myt;
			typedef rtree_value_type _Valty;

			explicit out_iterator(boost::function<void(const rtree_value_type&)> &out) : out(out)
			{
			}

			_Myt& operator=(const _Valty& _Val)
			{
				out(const_cast<_Valty&>(_Val));
				return (*this);
			}

			_Myt& operator*()			{ return (*this); }
			_Myt& operator++()			{ return (*this); }
		} outIt(out);

		tree.query(predicates, outIt);
	}

	void unregisterCoveringBlock(CoveringDataEntry* entry)
	{
		if (orderCount < 0) return;

		size_t end = entry->offset + entry->size - 1;
		range_type range(entry->offset, end);
		query(bgi::covered_by(range), [&] (const rtree_value_type& v) { if (v.second == entry) tree.remove(v); } );
	}
};

} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_COVERINGMAPMANAGER_HPP
