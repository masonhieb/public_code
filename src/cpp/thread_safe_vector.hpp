/*

BSD 3-Clause License

Copyright (c) 2021, Mason Hieb
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#pragma once
#include<vector>
#include<mutex>
#include <random>
#include<condition_variable>

// A thread safe vector designed in the context of a use case involving lots of threads pushing and a small number of threads popping

// Anything that involves iterating needs to be its own method in this class since you
// don't really don't want multiple threads iterating and modifying the vector at the same time
template <typename T>
class thread_safe_vector {
public:
	thread_safe_vector () :
		generator_(rd_())
	{}
	std::atomic_bool empty() {
		std::lock_guard<std::mutex> lk(m_);
		return vec_.empty();
	}
	std::atomic<std::size_t> size() {
		std::lock_guard<std::mutex> lk(m_);
		return vec_.size();
	}
	T pop(std::size_t idx) {
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]() {return !vec_.empty(); });
		return pop_non_thread_safe(idx);
	}
	T pop_random() {
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]() {return !vec_.empty(); });
		std::uniform_int_distribution<std::size_t> distribution(0, vec_.size()-1);
		return pop_non_thread_safe(distribution(generator_));
	}
	template<bool do_notify = true>
	void push_back(T val) {
		std::lock_guard<std::mutex> lk(m_);
		vec_.push_back(val);
		if (do_notify) {
			notify();
		}
	}
	void erase_index(std::size_t idx) {
		std::lock_guard<std::mutex> lk(m_);
		vec_.erase(vec_.begin()+idx);
	}
	void erase_range(std::size_t idx1, std::size_t idx2) {
		if (idx1 < idx2) {
			std::lock_guard<std::mutex> lk(m_);
			vec_.erase(vec_.begin() + idx1, vec_.begin() + idx2);
		}
	}
	void reserve(std::size_t n) {
		std::lock_guard<std::mutex> lk(m_);
		vec_.reserve(n);
	}
	void clear(T val) {
		std::lock_guard<std::mutex> lk(m_);
		vec_.clear();
	}
	template <class... valty>
	void emplace_back(valty&&... val) {
		std::lock_guard<std::mutex> lk(m_);
		vec_.emplace_back(std::forward<valty>(val)...);
		notify();
	}
	void notify() {
		cv_.notify_all();
	}
	T& at(std::size_t n) {
		std::lock_guard<std::mutex> lk(m_);
		return vec_.at(n);
	}
	const T& at(std::size_t n) const {
		std::lock_guard<std::mutex> lk(m_);
		return vec_.at(n);
	}
	T& operator[](std::size_t idx) { 
		std::lock_guard<std::mutex> lk(m_);
		return vec_[idx];
	}
	const T& operator[](std::size_t idx) const {
		std::lock_guard<std::mutex> lk(m_);
		return vec_[idx];
	}
private:
	std::vector<T> vec_;
	std::mutex m_;
	std::condition_variable cv_;
	std::random_device rd_;
	std::mt19937 generator_;
	T pop_non_thread_safe(std::size_t idx) {
		T out = vec_.at(idx);
		vec_.erase(vec_.begin() + idx);
		return out;
	}

};