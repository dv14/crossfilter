/*This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2018 Dmitry Vinokurov */

#include <map>
template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::init_slots() {
  auto filter_slot = [this](std::size_t filter_offset, int filter_bit_num,
                           const std::vector<std::size_t> &added,
                           const std::vector<std::size_t> &removed, bool not_filter) {
    update(filter_offset, filter_bit_num, added, removed, not_filter);
  };

  auto add_slot = [this](const value_vec_t &new_data,
                        const std::vector<std::size_t> &new_indexes,
                        std::size_t old_data_size, std::size_t new_data_size) {
    add(new_data, new_indexes, old_data_size, new_data_size);
  };

  auto remove_slot = [this](const std::vector<int64_t> &removed) {
    remove(removed);
  };
  connection_filter = dimension->connect_filter_slot(filter_slot);
  connection_add = dimension->connect_add_slot(add_slot);
  connection_remove =  dimension->connect_remove_slot(remove_slot);
  connection_dimension = dimension->connect_dispose_slot([this]() { dispose();});
}

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::remove(const std::vector<int64_t> &removed) {
  if(isGroupAll)
    return;

  std::vector<int> seen_groups(groups.size(),0);
  std::size_t group_num = 0;
  std::size_t j = 0;
  std::size_t remove_counter = 0;
  std::size_t sz = group_index.size();
  
  for(std::size_t i = 0; i < sz; i++) {
    if(removed[i] != REMOVED_INDEX) {
      group_index[j] = group_index[i];
      group_index.for_each(j,[&seen_groups](auto k) { seen_groups[k] = 1;});
      group_num += group_index.size(j);
      // groupIndex[j] = groupIndex[i];
      // if constexpr(Dimension::getIsIterable()) {
      //     std::size_t sz1 = groupIndex[j].size();
      //     for(std::size_t i0 = 0; i0 < sz1 ; i0++) {
      //       seenGroups[groupIndex[j][i0]] = 1;
      //       groupNum++;
      //     }
      //   } else {
      //   seenGroups[groupIndex[j]] = 1;
      //   groupNum++;
      // }

      j++;

    } else {
      remove_counter++;
    }
  }
  std::vector<group_type_t> old_groups;
  std::swap(old_groups,groups);
  groups.reserve(group_num);
  sz = seen_groups.size();

  for(std::size_t i = 0, k = 0; i < sz; i++) {
    if(seen_groups[i]) {
      seen_groups[i] = k++;
      groups.push_back(old_groups[i]);
    }
  }
  for(std::size_t i = 0; i < j; i++) {
    group_index.set(i,[&seen_groups](auto k) {
        return seen_groups[k];
      });

    // if constexpr(Dimension::getIsIterable()) {
    //     auto sz1 = groupIndex[i].size();
    //     for(std::size_t i0 = 0; i0 < sz1; i0++) {
    //       groupIndex[i][i0] = seenGroups[groupIndex[i][i0]];
    //     }
    //   } else {
    //   groupIndex[i] = seenGroups[groupIndex[i]];
    // }
  }
  group_index.resize(group_index.size() - remove_counter);
}


template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::update_one(std::size_t filter_offset, int filter_bit_index,
                                                              const std::vector<std::size_t> &added,
                                                              const std::vector<std::size_t> &removed,
                                                              bool not_filter) {
  if ((filter_bit_index > 0 && dimension->dimension_offset == filter_offset &&
       dimension->dimension_bit_index == filter_bit_index) ||
      reset_needed)
    return;

  std::for_each(added.begin(), added.end(),
                [this](auto i) {
                  if(dimension->zero_except(i))
                    groups[0].second = add_func(groups[0].second,dimension->get_raw(i),false);
                });

  std::for_each(removed.begin(), removed.end(),
                [this, filter_offset, filter_bit_index, not_filter] (auto i) {
                  if(filter_bit_index < 0) {
                    if(dimension->zero_except(i))
                      groups[0].second = remove_func(groups[0].second,dimension->get_raw(i),not_filter);
                  } else {
                    if(dimension->only_except(i,filter_offset, filter_bit_index))
                      groups[0].second = remove_func(groups[0].second,dimension->get_raw(i),not_filter);
                  }
                });
}

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::update(std::size_t filter_offset, int filter_bit_index,
                                                           const std::vector<std::size_t> &added,
                                                           const std::vector<std::size_t> &removed,
                                                           bool not_filter) {
  if (isGroupAll && isFlatIndex) {
    update_one(filter_offset, filter_bit_index, added, removed, not_filter);
    return;
  }

  if ((dimension->dimension_offset == filter_offset &&
       dimension->dimension_bit_index == filter_bit_index) ||
      reset_needed)
    return;

  for(auto i : added) {
    if(dimension->zero_except(i)) {
      auto & elem = dimension->get_raw(i);
      group_index.for_each(i, [this,elem](auto j) {
          auto & g = groups[j];
          g.second = add_func(g.second, elem, false);
        });
      // if constexpr(Dimension::getIsIterable()) {
      // for(auto j : groupIndex[i] ){
      //   auto & g = groups[j];
      //   g.second = add_func(g.second, dimension->getRaw(i), false);
      // }
      //   } else {
      //   auto & g = groups[groupIndex[i]];
      //   g.second = add_func(g.second, dimension->getRaw(i), false);
      // }
    }
  }

  for(auto i : removed) {
    if(filter_bit_index < 0) {
      if(!dimension->zero_except(i))
        continue;
    } else {
      if(!dimension->only_except(i, filter_offset, filter_bit_index))
        continue;
    }
    auto & elem = dimension->get_raw(i);
    group_index.for_each(i, [this,elem,not_filter](auto j) {
        auto & g = groups[j];
        g.second = remove_func(g.second, elem, not_filter);
      });

    // if constexpr(Dimension::getIsIterable()) {
    //     for(auto j : groupIndex[i]) {
    //       auto & g = groups[j];
    //       g.second = remove_func(g.second, dimension->getRaw(i), notFilter);
    //     }
    //   } else {
    //   auto & g = groups[groupIndex[i]];
    //   g.second = remove_func(g.second, dimension->getRaw(i), notFilter);
    // }
  }
}

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::add(const value_vec_t &new_data,
                                                   const std::vector<std::size_t> &new_data_index,
                                                   std::size_t old_data_size, std::size_t new_data_size) {
  if (new_data.empty()) {
    group_index.resize(group_index.size() + new_data_size);
    return;
  }
  data_size = new_data.size() + old_data_size;
  std::vector<group_type_t> new_groups;
  //  std::vector<group_index_t> newGroupIndex(groupIndex.size() + newDataSize);
  GroupIndex<isFlatIndex> new_group_index(group_index.size() + new_data_size);
  std::map<key_type_t, std::vector<std::size_t>>  old_indexes;

  auto sz = group_index.size();
  for (std::size_t i = 0; i < sz; i++) {
    group_index.for_each(i,[this, i,&old_indexes](auto k) {
        auto key = groups[k].first;
        old_indexes[key].push_back(i);
      });

    // if constexpr(Dimension::getIsIterable()) {
    //     for(auto gIndex : groupIndex[i]) {
    //       auto key = groups[gIndex].first;
    //       oldIndexes[key].push_back(i);
    //     }
    //   } else {
    //   auto key = groups[groupIndex[i]].first;
    //   oldIndexes[key].push_back(i);
    // }
  }

  std::size_t i = 0;
  std::size_t i1 = 0;
  std::size_t i2 = 0;
  key_type_t last_key =
      (groups.empty()) ? key(new_data[0]) : groups[0].first;
  auto reduce_value_f = [this](auto & g, std::size_t i) {
    g.second = add_func(g.second, dimension->get_raw(i), true);
    if (!dimension->zero_except(i))
      g.second = remove_func(g.second, dimension->get_raw(i), false);
  };
  auto group_size = groups.size();
  auto new_values_size = new_data.size();

  for (; i1 < group_size && i2 < new_values_size;) {
    auto new_key = key(new_data[i2]);

    if (new_key < groups[i1].first) {
      if (new_groups.empty() || new_groups.back().first < new_key) {
        auto ngroup = std::make_pair(new_key, reduce_type_t());
        new_groups.push_back(ngroup);
        last_key = new_key;
      }
      reduce_value_f(new_groups.back(), new_data_index[i2]);
      new_group_index.setIndex(new_data_index[i2],new_groups.size()-1);

      // if constexpr (Dimension::getIsIterable()) {
      //     newGroupIndex[newDataIndex[i2]].push_back(newGroups.size()-1);
      //   } else {
      //   newGroupIndex[newDataIndex[i2]] = newGroups.size()-1;
      // }
      i2++;
    } else if (new_key > groups[i1].first) {
      new_groups.push_back(groups[i1]);
      auto v = new_groups.size()-1;
      for (auto &j : old_indexes[groups[i1].first]) {
        new_group_index.setIndex(j,v);
        // if constexpr(Dimension::getIsIterable()) {
        //     newGroupIndex[j].push_back(newGroups.size() - 1);
        //   } else {
        //   newGroupIndex[j] = newGroups.size() - 1;
        // }
      }
      last_key = groups[i1].first;
      i1++;
    } else {
      //        skip new key
      last_key = groups[i1].first;
      new_groups.push_back(groups[i1]);
      auto gindex = new_groups.size() - 1;;

      for (auto j : old_indexes[groups[i1].first]) {
        new_group_index.setIndex(j,gindex);
      }
      reduce_value_f(new_groups.back(), new_data_index[i2]);
      new_group_index.setIndex(new_data_index[i2],gindex);
      i1++;
      i2++;
    }
  }
  for (; i1 < group_size; i1++, i++) {
    new_groups.push_back(groups[i1]);
    auto gindex = new_groups.size() - 1;

    for (auto &j : old_indexes[groups[i1].first]) {
      new_group_index.setIndex(j,gindex);
    }
  }
  for (; i2 < new_values_size; i2++, i++) {
    auto new_key = key(new_data[i2]);
    if (new_groups.empty() || new_key > last_key) {
      new_groups.push_back(std::make_pair(new_key, reduce_type_t()));
      last_key = new_key;
    }
    new_group_index.setIndex(new_data_index[i2],new_groups.size() - 1);
    auto & g = new_groups[new_groups.size()-1];
    reduce_value_f(g, new_data_index[i2]);
  }
  std::swap(groups, new_groups);
  std::swap(group_index, new_group_index);
}

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
std::vector<typename feature_impl<Key, Reduce, Dimension, isGroupAll>::group_type_t>
feature_impl<Key, Reduce, Dimension, isGroupAll>::top(std::size_t k) {
  auto &t = all();
  auto top = select_func(t, 0, groups.size(), k);
  return sort_func(top, 0, top.size());
}

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::reset_one() {
  if (groups.empty())
      groups.push_back(group_type_t());

  group_type_t &g = groups[0];
  g.second = initial_func();
  auto group_index_size = group_index.size();
  for (std::size_t i = 0; i < group_index_size; ++i) {
    g.second = add_func(g.second, dimension->get_raw(i), true);
  }
  for (std::size_t i = 0; i < group_index_size; ++i) {
    if (!dimension->zero_except(i)) {
      g.second =
          remove_func(g.second, dimension->get_raw(i), false);
    }
  }
}

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
void feature_impl<Key, Reduce, Dimension, isGroupAll>::reset() {
  if (isGroupAll && isFlatIndex) {
    reset_one();
    return;
  }
  // Reset all group values.
  for(auto &g: groups) {
    g.second = initial_func();
  }
  auto group_index_size = group_index.size();
  for (std::size_t i = 0; i < group_index_size; ++i) {
    auto & elem = dimension->get_raw(i);
    bool zero_except = dimension->zero_except(i);
    group_index.for_each(i,[this,elem,zero_except](auto k) {
        auto &g = groups[k];
        g.second = add_func(g.second, elem, true);
        if (!zero_except) {
          g.second =  remove_func(g.second, elem, false);
        }
      });
  }
}


template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
inline
std::vector<typename feature_impl<Key, Reduce, Dimension, isGroupAll>::group_type_t> &
feature_impl<Key, Reduce, Dimension, isGroupAll>::all() {
  if (reset_needed) {
    reset();
    reset_needed = false;
  }
  return groups;
}

template <typename K, typename R, typename D,
          bool b>
inline
typename feature_impl<K, R, D, b>::reduce_type_t feature_impl<K, R, D, b>::value() {
  if (reset_needed) {
    reset();
    reset_needed = false;
  }
  if (groups.empty())
    return initial_func();

  return groups[0].second;
}

// template <typename Key, typename Reduce, typename Dimension,
//           bool isGroupAll>
// inline
// feature_impl<Key, Reduce, Dimension, isGroupAll> &feature_impl<Key, Reduce, Dimension, isGroupAll>::reduce(
//     std::function<reduce_type_t(reduce_type_t &, const record_type_t &, bool)> add_,
//     std::function<reduce_type_t(reduce_type_t &, const record_type_t &, bool)> remove_,
//     Reduce initial_) {
//   add_func = add_;
//   remove_func = remove_;
//   initial_func = [initial_]() { return reduce_type_t(initial_); };
//   reset_needed = true;
//   return *this;
// }

// template <typename Key, typename Reduce, typename Dimension,
//           bool isGroupAll>
// inline
// feature_impl<Key, Reduce, Dimension, isGroupAll> &feature_impl<Key, Reduce, Dimension, isGroupAll>::reduceCount() {
//   return reduce([]( reduce_type_t &v, const record_type_t &, bool) { return v + 1; },
//                 []( reduce_type_t &v, const record_type_t &, bool) { return v - 1; },
//                 reduce_type_t());
// }

// template <typename Key, typename Reduce, typename Dimension,
//           bool isGroupAll>
// inline
// feature_impl<Key, Reduce, Dimension, isGroupAll> &
// feature_impl<Key, Reduce, Dimension, isGroupAll>::reduce_sum(std::function<reduce_type_t(const record_type_t &)> value) {
//   return reduce(
//       [value](reduce_type_t &v, const record_type_t &t, bool) {
//         return v + value(t);
//       },
//       [value](reduce_type_t &v, const record_type_t &t, bool) {
//         return v - value(t);
//       },
//       Reduce());
// }

template <typename Key, typename Reduce, typename Dimension,
          bool isGroupAll>
template<typename OrderFunc>
inline
feature_impl<Key, Reduce, Dimension, isGroupAll> &
feature_impl<Key, Reduce, Dimension, isGroupAll>::order(OrderFunc value) {
  //  using ph = std::placeholders;
  using T = typename std::decay< decltype(value(std::declval<reduce_type_t>())) >::type;
  select_func = std::bind(Heap<group_type_t, T>::select,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          std::placeholders::_3,
                          std::placeholders::_4,
                          [&value](const group_type_t& g) { return value(g.second);});

  sort_func = std::bind(Heap<group_type_t, T>::sort, std::placeholders::_1,
                        std::placeholders::_2,
                        std::placeholders::_3,
                        [&value](const group_type_t& g) { return value(g.second);});
  return *this;
}
