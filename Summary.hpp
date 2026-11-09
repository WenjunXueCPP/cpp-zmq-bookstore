// 订单统计：按商品 ID 聚合销量与营收
// 改造点：最值改为增量维护，O(n) 全表遍历 → O(1) 读取
// 同时把 std::map 换成 unordered_map：最值不再依赖遍历，哈希查找 O(1) 取代 O(log n)
#ifndef _SUMMARY_HPP
#define _SUMMARY_HPP

#include "cpplang.hpp"
#include "SalesData.hpp"
#include "SpinLock.hpp"

BEGIN_NAMESPACE(cpp_study)

class Summary final
{
public:
    using this_type           = Summary;
public:
    using sales_type          = SalesData;
    using lock_type           = SpinLock;
    using lock_guard_type     = SpinLockGuard;

    using string_type         = std::string;
    using uint_type           = sales_type::uint_type;
    using map_type            = std::unordered_map<string_type, sales_type>;
    using minmax_sales_type   = std::pair<string_type, string_type>;
public:
    Summary() = default;
   ~Summary() = default;

    Summary(const this_type&) = delete;
    Summary& operator=(const this_type&) = delete;

private:
    mutable lock_type   m_lock;
    map_type            m_sales;

    // 增量维护的最值，避免每 5 秒上报时持锁遍历整表
    string_type         m_min_id   {};
    string_type         m_max_id   {};
    uint_type           m_min_sold {~0u};
    uint_type           m_max_sold {0};
public:
    void add_sales(const sales_type& s)
    {
        lock_guard_type guard(m_lock);

        const auto& id = s.id();
        auto it = m_sales.find(id);

        if (it == m_sales.end()) {
            // 新商品入库：已有商品的销量只增不减，所以只有新商品可能刷新最小值
            it = m_sales.emplace(id, s).first;

            if (s.sold() < m_min_sold) {
                m_min_sold = s.sold();
                m_min_id   = id;
            }
        }
        else {
            it->second.inc_sold(s.sold());
            it->second.inc_revenue(s.revenue());
        }

        // 同样因为只增不减，只有"当前最大值所属商品"可能继续刷新最大值
        if (it->second.sold() > m_max_sold) {
            m_max_sold = it->second.sold();
            m_max_id   = it->second.id();
        }
    }

    minmax_sales_type minmax_sales() const
    {
        lock_guard_type guard(m_lock);

        if (m_sales.empty()) {
            return minmax_sales_type();
        }

        return {m_min_id, m_max_id};
    }

    std::size_t size() const
    {
        lock_guard_type guard(m_lock);
        return m_sales.size();
    }
};

END_NAMESPACE(cpp_study)

#endif  //_SUMMARY_HPP
