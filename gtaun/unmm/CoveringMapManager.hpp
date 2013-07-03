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

#include <algorithm>
#include <vector>

#include <boost/geometry/index/rtree.hpp>

namespace gtaun {
namespace unmm {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class CoveringMapManager
{
public:
	class CoveringDataEntry;

private:
	typedef bg::model::point<size_t, 1, bg::cs::cartesian> point_type;
	typedef bg::model::box<point_type> range_type;
	typedef std::pair<range_type, CoveringDataEntry*> rtree_value_type;
	typedef bgi::rtree<rtree_value_type, bgi::quadratic<16>> rtree_type;

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
		CoveringDataEntry(CoveringMapManager* manager, size_t order, size_t offset, size_t size, const void* data) :
			manager(manager), order(order), offset(offset), size(size), data(data)
		{
		}

		CoveringDataEntry(CoveringMapManager* manager, size_t order, const range_type& range, const void* data) :
			manager(manager), order(order), offset(range.min_corner().get<0>()),
			size(range.max_corner().get<0>() - offset + 1), data(data)
		{
		}

		~CoveringDataEntry()
		{
			if (manager != nullptr) manager->unregisterCoveringBlock(this);
		}

		inline size_t getEnd()
		{
			return offset + size - 1;
		}

		inline range_type getRange()
		{
			return range_type(offset, getEnd());
		}
	};

	struct ReadSequence
	{
	public:
		const CoveringDataEntry* entry;

	private:
		size_t offset;
		size_t size;

	public:
		ReadSequence(const CoveringDataEntry* entry, size_t offset, size_t size) :
			entry(entry), offset(offset), size(size)
		{
		}

		ReadSequence(const CoveringDataEntry* entry, const range_type& range) :
			entry(entry), offset(range.min_corner().get<0>()), size(range.max_corner().get<0>() - offset + 1)
		{
		}

		ReadSequence(const CoveringDataEntry* entry) :
			entry(entry), offset(entry->offset), size(entry->size)
		{
		}

		~ReadSequence()
		{
		}

		bool operator == (const ReadSequence& seq) const
		{
			if (entry != seq.entry) return false;
			if (offset != seq.offset) return false;
			if (size != seq.size) return false;
			return true;
		}

		inline size_t getOffset()
		{
			return offset;
		}

		inline size_t getSize()
		{
			return size;
		}

		inline size_t getEnd() const
		{
			return offset + size - 1;
		}

		inline range_type getRange() const
		{
			return range_type(offset, getEnd());
		}
	};

private:
	template<typename ValueType>
	class out_iterator
	{
	private:
		boost::function<void(const ValueType&)> &out;

	public:
		typedef out_iterator _Myt;
		typedef ValueType _Valty;

		explicit out_iterator(boost::function<void(const ValueType&)> &out) : out(out)
		{
		}

		_Myt& operator=(const _Valty& _Val)
		{
			out(const_cast<_Valty&>(_Val));
			return (*this);
		}

		_Myt& operator*()			{ return (*this); }
		_Myt& operator++()			{ return (*this); }
	};

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
		CoveringDataEntry* entry = new CoveringDataEntry(this, orderCount, offset, size, data);
		tree.insert(std::make_pair(entry->getRange(), entry));

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

	std::vector<ReadSequence> generateReadSequence(size_t offset, size_t size)
	{
		typedef std::pair<range_type, ReadSequence> value_type;
		typedef bgi::rtree<value_type, bgi::quadratic<16>> covered_rtree_type;

		const size_t& start = offset;
		const size_t end = offset + size - 1;

		covered_rtree_type coveredBlocks;

		std::vector<CoveringDataEntry*> entries = querySorted(offset, size);
		for (auto& entry : entries)
		{
			range_type range(max(start, entry->offset), min(end, entry->offset + entry->size - 1));

			std::vector<value_type> queriedCoveredBlocks;
			boost::function<void(const value_type&)> func = [&] (const value_type& v)
			{
				queriedCoveredBlocks.push_back(const_cast<value_type&>(v));
			};
			coveredBlocks.query(bgi::intersects(range), out_iterator<value_type>(func));

			for (auto& v : queriedCoveredBlocks)
			{
				range_type& coveredBlockRange = v.first;
				ReadSequence& coveredBlock = v.second;
				const CoveringDataEntry* coveredBlockData = coveredBlock.entry;

				size_t coveredBlockStart = max(start, coveredBlockRange.min_corner().get<0>());
				size_t coveredBlockEnd = min(end, coveredBlockRange.max_corner().get<0>());

				const size_t& entryStart = range.min_corner().get<0>();
				const size_t& entryEnd = range.max_corner().get<0>();

				if (entryStart > coveredBlockStart)
				{
					range_type newCoveredRange(coveredBlockStart, entryStart - 1);
					coveredBlocks.insert(std::make_pair(newCoveredRange, ReadSequence(coveredBlockData, newCoveredRange)));
				}
				if (entryEnd < coveredBlockEnd)
				{
					range_type newCoveredRange(entryEnd + 1, coveredBlockEnd);
					coveredBlocks.insert(std::make_pair(newCoveredRange, ReadSequence(coveredBlockData, newCoveredRange)));
				}

				coveredBlocks.remove(v);
			}

			coveredBlocks.insert(std::make_pair(range, ReadSequence(entry, range)));
		}

		std::vector<ReadSequence> readSequences;
		boost::function<void(const value_type&)> func = [&] (const value_type& v)
		{
			readSequences.push_back(v.second);
		};
		coveredBlocks.query(bgi::intersects(coveredBlocks.bounds()), out_iterator<value_type>(func));

		struct
		{
			bool operator() (ReadSequence& lhs, ReadSequence& rhs)
			{   
				return lhs.getOffset() < rhs.getOffset();
			}
		} start_less;
		std::sort(readSequences.begin(), readSequences.end(), start_less);

		return readSequences;
	}

private:
	CoveringMapManager(const CoveringMapManager&);
	CoveringMapManager& operator=(const CoveringMapManager&);
	
	template<typename Predicates>
	inline void query(Predicates &predicates, boost::function<void(const rtree_value_type&)> out)
	{
		tree.query(predicates, out_iterator<rtree_value_type>(out));
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
